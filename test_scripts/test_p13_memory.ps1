# ============================================================================
#  test_p13_memory.ps1 —— P13-1 内存地基验收（D17 调试钩子 + D18 负向用例）
#
#  为什么需要这个脚本：
#  「作用域末确实释放了内存」这件事**无法从程序输出看出来** —— 泄漏和
#  正确释放在 stdout 上一模一样。所以验收靠 core 层加的调试计数器
#  （core_dbg_allocations / core_dbg_frees，见 lib/core/rio 的说明）：
#  它们只在调试钩子里计数，而「真正落到 HeapFree」才算一次 free
#  （nullptr 与 cap==0 的静默返回不算，否则计数器证明不了任何事）。
#
#  ===== 本脚本为什么**不 import std** =====
#  用户 2026-11-17 裁决：D18 的借用规则**严格报错，不为了能跑而造近似**。
#  这条规则与现有标准库冲突（`fun toStr(arg: str): str { ret arg }` 这类
#  恒等函数、`split` 的 `result += [del]` 都属于「借用逃逸」），而用户同时
#  要求**本阶段不重写标准库** —— 等内存规范（Body/Content + 能力类型）
#  真正落地后再改。
#
#  所以本阶段的标准库会因为这个原因编译不过，这是**已确认接受的状态**，
#  不是回归。验收脚本因此全部用「裸 Rio + core_*」，不 import 任何 std 模块，
#  这样才能单独验证 P13-1 的地基是否工作。
#
#  覆盖 docs/p13_plan.md §8 的验收门槛 ③（七项）与 ⑧（四类负向）：
#    ①作用域末释放确实发生  ②嵌套 list[list[int]] 逐层释放
#    ③空指针安全            ④返回值转移不重复释放
#    ⑤循环内每轮释放        ⑥str 字面量不被释放（D11）
#    ⑦临时值/语句级销毁     ⑧四类负向用例必须编译报错（D18）
#
#  用法：powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts\test_p13_memory.ps1
# ============================================================================

$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
$rcc = Join-Path $root "release\RCC.exe"
$outDir = Join-Path $root "build\test_p13_memory"
$utf8 = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path $rcc)) { throw "RCC.exe not found: $rcc (build first)" }
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir -Force | Out-Null }

$script:pass = 0
$script:fail = 0
$script:failures = @()

function Record($name, $ok, $detail) {
    if ($ok) {
        $script:pass += 1
        Write-Host ("  [通过] {0,-40} {1}" -f $name, $detail) -ForegroundColor Green
    } else {
        $script:fail += 1
        $script:failures += "$name :: $detail"
        Write-Host ("  [失败] {0,-40} {1}" -f $name, $detail) -ForegroundColor Red
    }
}

# 编译 + 运行，比较**完整输出**（逐行，保留空行）
function Invoke-RioCase($name, $src, $expected) {
    $file = Join-Path $outDir ($name + '.rio')
    $exe = Join-Path $outDir ($name + '.exe')
    [System.IO.File]::WriteAllText($file, $src, $utf8)
    if (Test-Path $exe) { Remove-Item $exe -Force }
    $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) {
        Record $name $false ("编译失败: " + (($log -split "`n" | Where-Object { $_ -match 'Error' } |
            Select-Object -First 1) -replace '\s+', ' '))
        return
    }
    $actual = (& $exe 2>&1 | Out-String).Trim() -replace "`r`n", "`n"
    $want = $expected.Trim() -replace "`r`n", "`n"
    Record $name ($actual -eq $want) ("期望=[$want] 实际=[$actual]")
}

# 期望**编译失败**（D18 四类负向用例）
#
# 判据必须是一条**语义诊断**，不能只要求"退出码非 0"。踩过的坑写在这里，
# 免得以后又退回去：
#   * 早先这里只要求「退出码非 0 + 诊断里有 file:line:col + 日志里出现某个
#     片段」。第四条用例写成 `catch e {`（Rio 的正确写法是 `catch (e)`），
#     实际触发的是 **ParserError**；它之所以"通过"，是因为那段断言查的片段
#     是 'try'，而**文件名 neg_move_owner_in_try.rio 里就含 "try"**。用例
#     一直是假绿，而 D13 的规则当时根本不存在。
#   * 所以现在要求诊断是 code generation 阶段的语义错误（不是语法错误），
#     并且必须命中诊断串里**只有编译器才会写**的那部分（括号里的规则名）。
function Invoke-RioFails($name, $src, $expectFragment) {
    $file = Join-Path $outDir ($name + '.rio')
    $exe = Join-Path $outDir ($name + '.exe')
    [System.IO.File]::WriteAllText($file, $src, $utf8)
    if (Test-Path $exe) { Remove-Item $exe -Force }
    $log = & $rcc -build --path $file --output $exe 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0) {
        Record $name $false "本应编译报错，却编译成功了"
        return
    }
    # 必须是语义诊断，不能是语法错误（ParserError）。语法错误说明用例本身
    # 就没写对，测的是解析器而不是被验证的规则。
    if ($log -match 'ParserError|syntax errors') {
        Record $name $false ("用例本身有语法错误，测的不是该规则：" +
            (($log -split "`n" | Where-Object { $_ -match 'Pos  \]' } | Select-Object -First 1) -replace '\s+', ' '))
        return
    }
    if ($expectFragment -and ($log -notmatch [regex]::Escape($expectFragment))) {
        Record $name $false ("报错了但信息不含 '$expectFragment'： " +
            (($log -split "`n" | Where-Object { $_ -match 'Error: Code' } | Select-Object -First 1) -replace '\s+', ' '))
        return
    }
    # 诊断必须带 file:line:col（D18 要求沿用 P9 风格）
    $hasPos = $log -match '\.rio:\d+:\d+'
    Record $name $hasPos ("编译报错且带位置: " + $(if ($hasPos) { '是' } else { '否' }))
}

Write-Host ''
Write-Host '=== 1. 作用域末释放确实发生（D4 / D17）==='

# 函数体内的局部 str：函数返回时它必须被释放。
# 计数器口径：alloc 1 次（"a"+"b" 的拼接），free 1 次（作用域末）。
Invoke-RioCase 'scope_end_release' @'
fun work(): void {
    var s: str = "a" + "b"
    sout(s)
}
work()
sout(core_dbg_frees())
'@ "ab`n1"

# 两个拥有者都要释放（D4：逆序销毁，但两个都必须被销毁）
Invoke-RioCase 'two_vars_both_released' @'
fun work(): void {
    var a: str = "x" + "y"
    var b: str = "p" + "q"
    sout(a)
    sout(b)
}
core_dbg_reset()
work()
sout(core_dbg_frees())
'@ "xy`npq`n2"

Write-Host ''
Write-Host '=== 2. 返回值转移不重复释放（D9 规则 C：ret 即移动）==='

# ret 把拥有者整体交给调用方 —— 被返回的变量**不能**在出口再释放一次。
# 关键断言是 free **不是 2**（那才是双重释放）。这里是 0：拼接出的 str
# 被 ret 移交给 got，而 got 在 __rio_main 作用域末才销毁，读计数器时
# 还没到销毁点 —— 「= 0 而非 2」正是「没在 ret 处提前释放」的证据。
Invoke-RioCase 'ret_moves_not_double_free' @'
fun make(): str {
    var s: str = "a" + "b"
    ret s
}
core_dbg_reset()
var got: str = make()
sout(got)
sout(core_dbg_frees())
'@ "ab`n0"

Write-Host ''
Write-Host '=== 3. 空指针安全（D5）==='

# null 与未初始化的 str：core_free 必须静默返回，不得崩。
Invoke-RioCase 'null_is_safe' @'
var s: str
sout(core_dbg_frees())
fun touch(): void {
    var t: str
    sout(core_str_len(t))
}
touch()
sout(core_dbg_frees())
'@ "0`n0`n0"

Write-Host ''
Write-Host '=== 4. str 字面量不被释放（D11：cap==0 即静态只读头）==='

# 只绑定字面量、不做任何拼接 —— free 必须保持 0，否则就是在 free .rdata。
Invoke-RioCase 'literal_not_freed' @'
core_dbg_reset()
fun work(): void {
    var s: str = "abc"
    var t: str = "def"
    sout(s)
    sout(t)
    sout(core_dbg_frees())
}
work()
'@ "abc`ndef`n0"

Write-Host ''
Write-Host '=== 5. 嵌套 list[list[int]] 逐层释放（D12 布局 + D16 drop glue）==='

# 本用例验证「逐层」释放的**边界**，也就是 D14 明确登记的已知泄漏。
#
# 注意 `[inner]` 只写一次：D9 规则 C 里「拥有者入容器即移动」，
# 写两次 `[inner, inner]` 会让第二个 inner 变成「使用已移动的值」
# （H/X 检查会正确报错）—— 这本身就是对的，不是本用例要测的东西。
#
# 分配 4 块：inner 的 header+data、outer 的 header+data。
# 释放 2 块：只有 outer 的 header+data —— 因为 `inner` 被塞进 outer，
# 按 D9 规则 C 它是**移动**（源变 X，出口不再销毁），生命周期归 outer；
# 而 outer 的 drop glue 按 D14 的口径**不递归释放元素**（元素所有权
# 要等 P13-4 的子引用路径与成员集合追踪），所以 inner 的两块成为已知泄漏。
#
# 这个数字是**刻意锁定的**：如果哪天 drop 开始递归释放元素，这里会变成 4，
# 而那正意味着「dictKeys / 浅拷贝共享元素」那些场景会开始 use-after-free。
Invoke-RioCase 'nested_list_layers' @'
core_dbg_reset()
fun work(): void {
    var inner: list[int] = [1, 2]
    var outer: list[list[int]] = [inner]
    sout(outer[0][0])
}
work()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "1`n4`n2"

Write-Host ''
Write-Host '=== 6. 循环内每轮释放（D4 的 C 条）==='

# 循环体每轮新建一个 str，每轮末销毁 —— free 应等于轮数。
Invoke-RioCase 'loop_per_iteration' @'
core_dbg_reset()
fun work(): void {
    var i: int = 0
    while i < 3 {
        var s: str = "a" + "b"
        sout(s)
        i += 1
    }
}
work()
sout(core_dbg_frees())
'@ "ab`nab`nab`n3"

Write-Host ''
Write-Host '=== 7. 正常路径：移动后的值只要不再使用就合法 ==='

Invoke-RioCase 'moved_value_ok_if_unused' @'
fun make(): list[int] {
    var xs: list[int] = [7, 8]
    ret xs
}
var got = make()
sout(got[0])
sout(got[1])
'@ "7`n8"

Write-Host ''
Write-Host '=== 7b. 依赖 ABI v2 的实测回归（本轮修掉的三类崩溃）==='

# 给拥有者赋值 = 壳子赋值（规范 §11A.1）：两个槽共享同一块内容，只有一方是
# 拥有者。早先两个都是拥有者、指向同一块，作用域末各 drop 一次 => 0xC0000374
# 堆损坏。现在期望 alloc=2（两个拼接）、free=2（a 的旧内容 + 存活的那一块）。
Invoke-RioCase 'assign_owner_becomes_alias' @'
fun work(): void {
    var a: str = "x" + "y"
    var b: str = "p" + "q"
    b = a
    sout(b)
}
core_dbg_reset()
work()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "xy`n2`n2"

# 借用变量被赋值为**新值** => 升格成拥有者，出口必须销毁它。
# 只改 borrowed 不动 moved 的写法会让这里 free=0（漏释放）。
Invoke-RioCase 'borrow_promotes_to_owner' @'
fun work(): void {
    var xs: list[int] = [1, 2]
    var ys: list[int] = xs
    ys = [7, 8]
    sout(ys[0])
}
core_dbg_reset()
work()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "7`n4`n4"

# break 是终结性跳转，不会再走到循环体的词法出口：不在这里发 drop 就每
# break 一次泄漏一批。2 轮里每轮一个拼接 => alloc=2、free=2。
Invoke-RioCase 'break_releases_body_vars' @'
fun work(): void {
    var i: int = 0
    while i < 5 {
        var s: str = "a" + "b"
        sout(s)
        if i == 1 {
            break
        }
        i += 1
    }
}
core_dbg_reset()
work()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "ab`nab`n2`n2"

# continue 同理：4 轮里每轮一个拼接，被 continue 跳过的那轮同样要释放。
Invoke-RioCase 'continue_releases_body_vars' @'
fun work(): void {
    var i: int = 0
    var n: int = 0
    while i < 4 {
        var s: str = "a" + "b"
        i += 1
        if i == 2 {
            continue
        }
        n += 1
    }
    sout(n)
}
core_dbg_reset()
work()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "3`n4`n4"

Write-Host ''
Write-Host '=== 8. D18 负向用例：四类必须编译报错 ==='

# ① 使用已移动的值
Invoke-RioFails 'neg_use_moved_value' @'
fun make(): list[int] {
    var xs: list[int] = [1, 2]
    ret xs
    sout(xs[0])
}
make()
'@ 'moved'

# ② 返回借用的形参（D9 规则 C / D18 第二条）
Invoke-RioFails 'neg_return_borrowed_param' @'
fun take(s: str): str {
    ret s
}
sout(take("abc"))
'@ 'borrowed'

# ③ 存储借用（把形参存进容器）
Invoke-RioFails 'neg_store_borrowed_param' @'
fun stash(s: str): list[str] {
    var r: list[str] = []
    r += [s]
    ret r
}
sout(stash("abc")[0])
'@ 'borrowed'

# ④ 在 try 体内移动外层拥有者（D13 第 2 条）
#
# 注意用例写法：`catch (e)` 带括号才是 Rio 的合法语法。写成 `catch e` 会
# 得到 ParserError，测的就不是这条规则了（这条用例此前正是这么假绿的）。
# 另外"移动"必须是真的移动 —— `var inner: list[int] = outer` 只是**借用**，
# 拦它的是借用规则；D13 要拦的是把外层拥有者交出去（入容器 / ret）。
Invoke-RioFails 'neg_move_owner_in_try' @'
fun f(): void {
    var outer: list[int] = [1]
    try {
        var box: list[list[int]] = [outer]
        throw "x"
    } catch (e) {
        sout("caught")
    }
}
f()
'@ 'D13'

Write-Host ''
Write-Host '=== 9. D10 临时值在语句末销毁 ==='

# `sout("a"+"b")` 里的拼接结果没有任何变量接住它 —— 出生地是当前语句的
# 匿名 local，销毁点是语句末。不做这条，str 的大部分分配都泄漏。
Invoke-RioCase 'temp_in_sout_released' @'
core_dbg_reset()
sout("a" + "b")
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "ab`n1`n1"

# 临时值作实参：调用返回后没人接住它，同样语句末销毁。
Invoke-RioCase 'temp_arg_released' @'
fun take(s: str): void {
    sout(s)
}
core_dbg_reset()
take("a" + "b")
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "ab`n1`n1"

# 同一语句里多个临时值，各释放一次。
Invoke-RioCase 'two_temps_one_statement' @'
core_dbg_reset()
sout("a" + "b", "c" + "d")
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "abcd`n2`n2"

# 借用实参**不是**新值：它是调用方的东西，语句末不能释放。
# 这里 read 之后 keep 必须还能正常用（free=0 且输出 xy）。
Invoke-RioCase 'borrow_arg_not_released' @'
fun take(s: str): void {
    sout(s)
}
fun work(): void {
    var keep: str = "x" + "y"
    core_dbg_reset()
    take(keep)
    sout(core_dbg_allocations())
    sout(core_dbg_frees())
    sout(keep)
}
work()
'@ "xy`n0`n0`nxy"

Write-Host ''
Write-Host '=== 10. D17 重复释放检测与 0xDD 填埋 ==='

# 循环每轮分配+释放：free 等于轮数，**不能**被误判成重复释放。
# 这里锁的是一个真实的实现坑：早期的检测在「最近释放过的指针」环形缓冲里
# 查重，但释放掉的地址会被下一轮 core_alloc 复用，于是每轮的正常释放都被
# 算成重复释放（实测 3 轮报出 1 次）。修法是 core_alloc 成功时把该地址
# 从 ring 里清掉 —— 之后 ring 里只剩「释放过且尚未被重新分配」的地址。
Invoke-RioCase 'no_false_double_free' @'
fun work(): void {
    var i: int = 0
    while i < 3 {
        var s: str = "a" + "b"
        sout(s)
        i += 1
    }
}
core_dbg_reset()
work()
sout(core_dbg_frees())
sout(core_dbg_double_frees())
'@ "ab`nab`nab`n3`n0"

# 真正的重复释放必须被数出来（而不是让进程以 0xC0000374 崩掉）。
# 第一次正常释放 free=1；第二次命中检测 -> 不再真的 HeapFree，只
# double_frees=2（两次调用都命中，因为指针一直留在 ring 里）。
Invoke-RioCase 'double_free_is_counted' @'
fun freeStr(p: str): void {
    core_free(p)
}
fun work(): void {
    var s: str = "x" + "y"
    freeStr(s)
    freeStr(s)
}
core_dbg_reset()
work()
sout(core_dbg_frees())
sout(core_dbg_double_frees())
'@ "1`n2"

# 释放前确实往整块写了 0xDD：poison_bytes 必须 > 0，且至少覆盖
# 一个 str 头（16 字节）。
Invoke-RioCase 'poison_bytes_counted' @'
core_dbg_reset()
fun work(): void {
    var s: str = "a" + "b"
    sout(s)
}
work()
sout(core_dbg_poison_bytes())
'@ "ab`n18"

Write-Host ''
Write-Host '=== 11. 原地追加与 clone（D6 / D12）==='

# 原地追加的核心断言：**连续两次追加都要真的进去**。
# 踩过的坑（2026-11-17 实测）：core_mem_copy / core_alloc / core_free 违反
# MS x64 的 callee-saved 约定（%rsi / %rdi 被覆盖后不恢复），而
# core_list_append 正好把 value 放在 %rsi、newcap 放在 %rdi 里跨调用，
# 于是第一次扩容追加后元素变成 0，输出 1/2/0/4。
# 现在 value 与 newcap 都改放栈槽，且 core_* 全部按 ABI 保存恢复。
Invoke-RioCase 'append_in_place_twice' @'
fun main(): void {
    var xs: list[int] = [1, 2]
    core_list_append(xs, 3)
    core_list_append(xs, 4)
    sout(xs[0])
    sout(xs[1])
    sout(xs[2])
    sout(xs[3])
}
main()
'@ "1`n2`n3`n4"

# header 地址不变（D12 的承诺）：把 xs 借用给另一个槽位，追加后**两边**
# 都看到新长度。若 header 搬了家，ys 会指向旧地址，读到旧 len。
Invoke-RioCase 'append_keeps_header_stable' @'
fun push(xs: list[int], v: int): void {
    core_list_append(xs, v)
}
fun main(): void {
    var xs: list[int] = [1]
    push(xs, 2)
    var ys: list[int] = xs
    push(ys, 3)
    sout(xs[2])
    sout(ys[2])
}
main()
'@ "3`n3"

# 追加元素后作用域末仍只释放一次（header + data 两块），且扩容次数符合
# 「2 倍增长、至少 4」的策略：
#   alloc：字面量 [1,2] 的 header+data = 2，第一次追加扩容（cap 2->4）= 1，
#          第二次追加 len=3 < cap=4 走**原位写**不扩容 => 共 3
#   free ：扩容时释放旧 data = 1，出口释放 header+data = 2 => 共 3
# 这个数字同时锁住两件事：扩容后旧块**确实**被释放（不是泄漏），以及
# 没扩容时**不会**多分配（不是每次都 realloc）。
Invoke-RioCase 'append_free_count' @'
core_dbg_reset()
fun main(): void {
    var xs: list[int] = [1, 2]
    core_list_append(xs, 3)
    core_list_append(xs, 4)
    sout(xs[3])
}
main()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "4`n3`n3"

# clone 出来的容器与源**互相独立**：改 ys 不影响 xs。
Invoke-RioCase 'clone_list_independent' @'
fun main(): void {
    var xs: list[int] = [1, 2]
    var ys: list[int] = core_clone_list(xs)
    core_list_append(ys, 9)
    sout(xs[0])
    sout(ys[2])
}
main()
'@ "1`n9"

# clone_str 得到的是**可释放的**新 str（cap 等于真实容量）：
# alloc = 原拼接 1 + clone 1 = 2，free = 2（两块都在出口销毁）。
Invoke-RioCase 'clone_str_owned' @'
core_dbg_reset()
fun main(): void {
    var s: str = "a" + "b"
    var c: str = core_clone_str(s)
    sout(s)
    sout(c)
}
main()
sout(core_dbg_allocations())
sout(core_dbg_frees())
'@ "ab`nab`n2`n2"

Write-Host ''
Write-Host ('结果：通过 {0}，失败 {1}' -f $script:pass, $script:fail)
if ($script:fail -gt 0) {
    Write-Host '失败清单：' -ForegroundColor Red
    foreach ($f in $script:failures) { Write-Host "  - $f" -ForegroundColor Red }
    exit 1
}
Write-Host 'P13-1 地基验收通过（标准库改写留待内存规范落地后）' -ForegroundColor Green
exit 0
