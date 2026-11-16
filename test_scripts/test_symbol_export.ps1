# ============================================================================
#  P12-1 —— `-s` 符号表导出器验收测试（六种格式）
#
#  设计前提（用户 2026-11-16 裁决，见 docs/p12_plan.md §3.3）：
#    `-s` 被**重新设计**，不再追求与旧实现逐字节一致。理由：
#      * 旧 symbolId 形如 name_vidN，N 来自 utils::Object 的全局自增计数器，
#        取决于构造顺序，从干净 AST 遍历无法复现；
#      * 旧 JSON 键序来自 unordered_map 迭代顺序，本身就不稳定。
#    新设计目标：多格式导出、便于其他程序消费、其中必须有供人查阅的可视化格式。
#
#  获批的六种格式：text / json / html / ctags / dot / markdown
#
#  本脚本校验的是**结构性质**（每种格式该有的东西在不在、对不对），
#  不是逐字节比对旧输出 —— 那在旧实现内部都不成立。
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_symbol_export.ps1
# ============================================================================

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$rcc  = Join-Path $repo 'release\RCC.exe'
$work = Join-Path $repo 'build\p12_symbol_export'

if (-not (Test-Path $rcc)) {
    Write-Host "找不到编译器：$rcc" -ForegroundColor Red
    Write-Host "请先构建：cmake --build build" -ForegroundColor Yellow
    exit 1
}

if (Test-Path $work) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$pass = 0
$fail = 0
$failures = @()

function Check {
    param([string]$Name, [bool]$Ok, [string]$Detail = '')
    if ($Ok) {
        $script:pass++
        Write-Host "  [通过] $Name" -ForegroundColor Green
    } else {
        $script:fail++
        $script:failures += $Name
        Write-Host "  [失败] $Name" -ForegroundColor Red
        if ($Detail) { Write-Host "         $Detail" -ForegroundColor DarkYellow }
    }
}

function Write-Source {
    param([string]$Name, [string]$Body)
    $path = Join-Path $work "$Name.rio"
    [System.IO.File]::WriteAllText($path, $Body, $utf8NoBom)
    return $path
}

function Invoke-Export {
    param([string]$Path, [string]$Format = 'text', [string[]]$Extra = @())
    $argv = @('-s', '--p', $Path, '--fmt', $Format) + $Extra
    # 预期会失败的命令（如 --ss 找不到符号）不能因 ErrorActionPreference=Stop 中断脚本，
    # 这里临时降级到 Continue，只取退出码与输出。
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & $rcc @argv 2>&1 | Out-String
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
    return @{ Text = $out; Code = $code }
}

# ---------------------------------------------------------------- 测试输入
# 覆盖：继承、方法、泛型方法、构造函数、重载、顶层函数、全局变量、调用关系
$rich = Write-Source 'rich' @'
class Base {
    var id: int = 0
    fun get(): int { ret id }
}

class Box: Base {
    var x: int = 0
    ctor(n: int) { x = n }
    fun get[T](v: T): T { ret v }
}

fun top(a: int, *rest, k: str = "z"): bool { ret true }
fun top(a: str): bool { ret false }
fun g(): void { }
'@

# 覆盖：import 别名 + export 语句（export 会在模块内联时被丢弃，必须前置扫描）
$calls = Write-Source 'calls' @'
import(io="io")
export(helper, Shape, Circle)

class Shape {
    var n: int = 0
}

class Circle: Shape {
    var r: int = 0
}

fun helper(s: Shape): int { ret s.n }

fun caller(): int {
    var c: Circle = Circle()
    ret helper(c)
}

fun main(): void {
    var x: int = caller()
    io.sout(x)
}
'@

Write-Host ""
Write-Host "==== 1. 六种格式都能产出非空输出 ====" -ForegroundColor Cyan

$expected = @{
    text     = 'Symbol table:'
    json     = '"schemaVersion"'
    html     = '<!DOCTYPE html>'
    ctags    = '!_TAG_FILE_FORMAT'
    dot      = 'digraph'
    markdown = '# Symbol table'
}

foreach ($fmt in @('text','json','html','ctags','dot','markdown')) {
    $r = Invoke-Export -Path $rich -Format $fmt
    Check "格式 $fmt 输出正常" ($r.Code -eq 0 -and $r.Text.Contains($expected[$fmt])) `
        "exit=$($r.Code) 期望包含 '$($expected[$fmt])'"
}

Write-Host ""
Write-Host "==== 2. text 格式内容 ====" -ForegroundColor Cyan

$txt = (Invoke-Export -Path $rich -Format 'text').Text

Check 'text 报出类'            ($txt -match 'Box')
Check 'text 报出继承关系'       ($txt -match 'Box:\s*Base')
Check 'text 报出重载函数 top'   ($txt -match 'top\(')
Check 'text 报出方法 Box.get'   ($txt -match 'Box\.get')
Check 'text 报出构造函数'       ($txt -match 'Box\(n: int\)')
Check 'text 报出汇编符号名'     ($txt -match 'asm=Box\.__init__')
Check 'text 带可变参数标记'     ($txt -match '\*rest')
Check 'text 带默认值'           ($txt -match 'k: str = "z"')

Write-Host ""
Write-Host "==== 3. json 格式结构 ====" -ForegroundColor Cyan

$jsonRaw = (Invoke-Export -Path $rich -Format 'json').Text
$jsonOk = $true
$jsonObj = $null
try { $jsonObj = $jsonRaw | ConvertFrom-Json } catch { $jsonOk = $false }

Check 'json 可被解析' $jsonOk
if ($jsonOk) {
    Check 'json 有 schemaVersion'   ($jsonObj.schemaVersion -eq 2)
    Check 'json 有 symbols 数组'    ($null -ne $jsonObj.symbols -and $jsonObj.symbols.Count -gt 0)
    Check 'json 记录主文件名'       ($null -ne $jsonObj.mainFile)

    $box = $jsonObj.symbols | Where-Object { $_.name -eq 'Box' } | Select-Object -First 1
    Check 'json 含 Box 类'          ($null -ne $box)
    if ($box) {
        Check 'json Box 有基类'     ($box.baseClass -eq 'Base')
    }

    $ctor = $jsonObj.symbols | Where-Object { $_.kind -eq 'constructor' } | Select-Object -First 1
    Check 'json 含构造函数条目'     ($null -ne $ctor)
    if ($ctor) {
        Check 'json 构造函数 asmSymbol 正确' ($ctor.asmSymbol -eq 'Box.__init__')
    }

    # 关键设计点：重载函数算不出 _R 编码，asmSymbol 必须留空而不是编一个假名
    $over = $jsonObj.symbols | Where-Object { $_.name -eq 'top' } | Select-Object -First 1
    Check 'json 含重载函数 top'     ($null -ne $over)
    if ($over) {
        Check 'json 重载函数 asmSymbol 留空' ([string]::IsNullOrEmpty($over.asmSymbol))
    }

    # 参数结构
    $withParams = $jsonObj.symbols | Where-Object { $_.params -and $_.params.Count -gt 0 } | Select-Object -First 1
    Check 'json 参数带结构' ($null -ne $withParams)
}

Write-Host ""
Write-Host "==== 4. html 格式（可视化）====" -ForegroundColor Cyan

$html = (Invoke-Export -Path $rich -Format 'html').Text

Check 'html 是完整文档'       ($html -match '<!DOCTYPE html>' -and $html -match '</html>')
Check 'html 自包含 CSS'       ($html -match '<style')
Check 'html 自包含 JS'        ($html -match '<script')
Check 'html 有搜索框'         ($html -match 'input')
Check 'html 含符号数据'       ($html -match 'Box')
Check 'html 不引用外部资源'   (-not ($html -match '<link[^>]+href="http'))

Write-Host ""
Write-Host "==== 5. ctags 格式 ====" -ForegroundColor Cyan

$ctags = (Invoke-Export -Path $rich -Format 'ctags').Text
$ctagsLines = $ctags -split "`n" | Where-Object { $_ -notmatch '^!_' -and $_.Trim() -ne '' }

Check 'ctags 有扩展格式头'    ($ctags -match '!_TAG_FILE_FORMAT\s+2\s+/extended format/')
Check 'ctags 有非头部条目'    ($ctagsLines.Count -gt 0)
Check 'ctags 条目含文件字段'  ($ctagsLines[0] -match 'rich\.rio')
Check 'ctags 构造函数标签带类名' ($ctags -match 'Box\.__init__')
Check 'ctags 构造函数搜索 ctor'  ($ctags -match 'Box\.__init__.*?/\^\[\s\\t\]\*ctor/')

Write-Host ""
Write-Host "==== 6. dot 格式 ====" -ForegroundColor Cyan

$dot = (Invoke-Export -Path $calls -Format 'dot').Text

Check 'dot 是 digraph'          ($dot -match '^digraph' -or $dot -match 'digraph\s')
Check 'dot 有继承子图'          ($dot -match 'Inheritance')
Check 'dot 有调用子图'          ($dot -match 'Calls')
Check 'dot 含继承边 Shape->Circle' ($dot -match '"class:Shape"\s*->\s*"class:Circle"')
Check 'dot 含调用边'            ($dot -match '"fn:caller"\s*->\s*"fn:helper"')
Check 'dot 只画主文件（标注说明）' ($dot -match 'main file only')

Write-Host ""
Write-Host "==== 7. markdown 格式 ====" -ForegroundColor Cyan

$md = (Invoke-Export -Path $rich -Format 'markdown').Text

Check 'markdown 有标题'         ($md -match '^#\s')
Check 'markdown 有 Classes 表'  ($md -match '##\s+Classes')
Check 'markdown 有 Functions 表' ($md -match '##\s+Functions')
Check 'markdown 是表格'         ($md -match '\|\s*-+')
Check 'markdown 复数不是 Classs' (-not ($md -match 'Classs'))

Write-Host ""
Write-Host "==== 8. export / 过滤 / 错误路径 ====" -ForegroundColor Cyan

$expOnly = (Invoke-Export -Path $calls -Format 'text' -Extra @('-exp')).Text
Check '-exp 只导出 export(...) 的符号' ($expOnly -match 'Shape' -and $expOnly -match 'helper')
Check '-exp 排除未导出的 caller'       (-not ($expOnly -match 'caller'))

$spec = (Invoke-Export -Path $rich -Format 'text' -Extra @('--ss', 'top')).Text
Check '--ss 按名过滤' ($spec -match 'top' -and -not ($spec -match 'Box\(n'))

$missing = Invoke-Export -Path $rich -Format 'text' -Extra @('--ss', 'nope')
Check '--ss 找不到时非零退出' ($missing.Code -ne 0)
Check '--ss 找不到时给出诊断' ($missing.Text -match 'not found')

$badFmt = Invoke-Export -Path $rich -Format 'xml'
Check '未知格式非零退出' ($badFmt.Code -ne 0)
Check '未知格式列出可用格式' ($badFmt.Text -match 'text' -and $badFmt.Text -match 'markdown')

Write-Host ""
Write-Host "==== 9. 格式名大小写不敏感 ====" -ForegroundColor Cyan

$upper = Invoke-Export -Path $rich -Format 'JSON'
Check '大写格式名可用' ($upper.Code -eq 0 -and $upper.Text.Contains('"schemaVersion"'))

Write-Host ""
Write-Host "==== 10. 内置库导出（模块内联）====" -ForegroundColor Cyan

$builtin = & $rcc -s -b --ext ds --fmt text 2>&1 | Out-String
Check '内置库导出非空' ($builtin -match 'Symbol table')
Check '内置库报出模块名' ($builtin -match 'Modules:')

Write-Host ""
Write-Host "==== 11. --output 写文件 ====" -ForegroundColor Cyan

$outFile = Join-Path $work 'out.html'
$null = & $rcc -s --p $rich --fmt html --o $outFile 2>&1
Check '--output 生成文件' (Test-Path $outFile)
if (Test-Path $outFile) {
    $written = [System.IO.File]::ReadAllText($outFile)
    Check '--output 内容正确' ($written -match '<!DOCTYPE html>')
}

# ---------------------------------------------------------------- 汇总
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "结果：通过 $pass，失败 $fail" -ForegroundColor $(if ($fail -eq 0) { 'Green' } else { 'Red' })

if ($fail -gt 0) {
    Write-Host "失败用例：$($failures -join ', ')" -ForegroundColor Red
    exit 1
}

Write-Host "P12-1 符号表导出器验收通过" -ForegroundColor Green
exit 0
