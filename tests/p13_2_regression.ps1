# p13_2_regression.ps1 —— P13-2 已实施成果回归测试
# 用法（本机执行策略禁止直接跑 .ps1，用 Invoke-Expression）：
#   Invoke-Expression (Get-Content -Raw .\tests\p13_2_regression.ps1)
#
# 每条用例：内嵌 .rio 源码 + 期望「编译成功/失败」。
# 失败用例额外可指定必须出现的诊断关键字（如 L0-2、moved）。
# 只验证编译器行为（exit code + 诊断文本），不运行生成的 exe
# （本机 stdout 捕获有问题，与编译器行为无关）。

$ErrorActionPreference = "Continue"
$RccExe = "D:\DeepseekHarnessProjects\RCC\release\RCC.exe"
$tmpDir = "D:\DeepseekHarnessProjects\RCC\tests\_p13_2_tmp"
New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

$script:pass = 0
$script:fail = 0
$caseNo = 0

function Test-Case {
    param(
        [string]$Name,
        [string]$Source,
        [bool]$ExpectSuccess,
        [string]$MustContain = ""
    )
    $script:caseNo++
    $idx = $script:caseNo
    $file = Join-Path $tmpDir ("case_{0:d2}.rio" -f $idx)
    [System.IO.File]::WriteAllText($file, $Source + "`n", [System.Text.UTF8Encoding]::new($false))
    $exe = [System.IO.Path]::ChangeExtension($file, ".exe")
    foreach ($ext in @(".exe", ".o", ".s")) {
        $p = [System.IO.Path]::ChangeExtension($file, $ext)
        if (Test-Path $p) { Remove-Item $p -Force }
    }
    $out = & $RccExe build --p $file 2>&1 | Out-String
    $code = $LASTEXITCODE
    $ok = ($code -eq 0) -eq $ExpectSuccess
    if ($ok -and $MustContain -ne "" -and -not $out.Contains($MustContain)) {
        $ok = $false
    }
    if ($ok) {
        $script:pass++
        Write-Host ("  [PASS] {0}" -f $Name)
    } else {
        $script:fail++
        Write-Host ("  [FAIL] {0} (exit={1}, expected success={2}, want='{3}')" -f $Name, $code, $ExpectSuccess, $MustContain)
        ($out -split "`r?`n" | Where-Object { $_ -match 'Error:|L[0-9]:|moved|Syntax error' } | Select-Object -First 3) | ForEach-Object { Write-Host ("         " + $_.Trim()) }
    }
}

Write-Host "=== L0：静默错误改报错 ==="
Test-Case "L0-1 未知签名子句" 'fun f(a: int): int garbage[a] { ret a }' $false "L0-1"
Test-Case "L0-1 空方括号子句" 'fun f(a: int): int from[] { ret a }' $false "L0-1"
Test-Case "L0-2 类字段类型拼错" "class C { var f: Bogus }`nfun main(): void { var c = C() }" $false "L0-2"
Test-Case "L0-3 fun ctor" "class C { var v: int`nfun ctor(n: int) { this.v = n }`n}`nfun main(): void { var c = C(7) }" $false "L0-3"
Test-Case "L0-4 多类型标签冲突" 'fun main(): void { var x: str int = "hi"; sout(x) }' $false "L0-4"
Test-Case "L0-5 break value" "fun main(): void {`n var i = 0`n while i < 3 {`n  i += 1`n  if i == 2 { break 42 }`n }`n}" $false "L2:"
Test-Case "L0-6 类名冲突 int" 'class int { var v: int }' $false "L0-6"
Test-Case "L0-6 类名冲突 Pool" 'class Pool { var v: int }' $false "L0-6"
Test-Case "L0-7 重复类定义" "class P { var v: int }`nclass P { var w: int }" $false "L0-7"

Write-Host "=== L1：ret/return 拆分 + 裸块 ==="
Test-Case "L1 ret 合法（退循环）" "fun f(): int {`n var i = 0`n while i < 5 {`n  i += 1`n  if i == 2 { ret 99 }`n }`n ret i`n}`nfun main(): void { var x = f() }" $true ""
Test-Case "L1 return 合法（退函数）" "fun f(): int {`n var i = 0`n while i < 5 {`n  i += 1`n  if i == 2 { return 99 }`n }`n ret i`n}`nfun main(): void { var x = f() }" $true ""
Test-Case "L1 裸块识别" "fun main(): void {`n {`n  sout(1)`n }`n sout(2)`n}" $true ""
Test-Case "L1 裸块内分号" "fun main(): void {`n {`n  var x: int = 5; var y: int = 6; sout(x + y)`n }`n}" $true ""
Test-Case "L1 字典仍合法" "fun main(): void { var d = {`"a`": 1, `"b`": 2}; var e = {} }" $true ""

Write-Host "=== L2：: T 标注 + 值合流 ==="
Test-Case "L2 if:T 合法" 'fun main(): void { var x = if (1 > 0): int { 7 } else { 8 } }' $true ""
Test-Case "L2 if:T 类型不符" 'fun main(): void { var x = if (1 > 0): int { "hi" } else { 8 } }' $false "L2:"
Test-Case "L2 if:T 无值" "fun noValue(): void { }`nfun main(): void { var x = if (1 > 0): int { noValue() } else { 8 } }" $false "L2:"
Test-Case "L2 while:T 合法" "fun main(): void { var i = 0; var x = while (i < 3): int { i += 1 } }" $true ""
Test-Case "L2 break value 合法（标注循环）" "fun main(): void {`n var i = 0`n var x = while (i < 5): int {`n  i += 1`n  if i == 3 { break 42 }`n }`n}" $true ""
Test-Case "L2 break value 无标注报错" "fun main(): void {`n var i = 0`n while i < 5 {`n  i += 1`n  if i == 3 { break 42 }`n }`n}" $false "L2:"
Test-Case "L2 break value 类型不符" "fun main(): void {`n var i = 0`n var x = while (i < 5): int {`n  i += 1`n  if i == 3 { break `"hi`" }`n }`n}" $false "L2:"
Test-Case "L2 if 内 ret value 合法" 'fun main(): void { var x = if (1 > 0): int { ret 42 } else { ret 8 } }' $true ""
Test-Case "L2 if 内 ret value 类型不符" 'fun main(): void { var x = if (1 > 0): int { ret "hi" } else { ret 8 } }' $false "L2:"

Write-Host "=== L3：内存模型类型层 ==="
$classP = "class Player {`n var name: str`n ctor(n: str) { this.name = n }`n}"
Test-Case "L3 五类型声明" "$classP`nfun main(): void {`n var p: Body[Player] = Player(`"bob`")`n var c: Content[Player] = null`n var r: Ref[Player] = null`n var m: Mut[Player] = null`n}" $true ""
Test-Case "L3 Body 别名赋值" "$classP`nfun main(): void { var p: Body[Player] = Player(`"bob`"); var q: Player = p }" $true ""
Test-Case "L3 Content/Body 冲突（take 结果）" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var c: Player = bob.take() }" $false "cannot assign"
Test-Case "L3 Ref/Body 冲突（字面量不可取引用）" "$classP`nfun main(): void { var r: Ref[Player] = 42 }" $false "cannot pass int"
Test-Case "L3 Take 作 var 类型报错" "$classP`nfun main(): void { var t: Take[Player] = null }" $false "L3:"
Test-Case "L3 Take 作返回类型报错" "$classP`nfun f(): Take[Player] { ret null }`nfun main(): void { }" $false "L3:"
Test-Case "L3 Take 作形参合法" "$classP`nfun f(p: Take[Player]): int { ret 1 }`nfun main(): void { }" $true ""
Test-Case "L3 Content 移动" "$classP`nfun main(): void { var c: Content[Player] = null; var d = c; var e = c }" $false "Moved state"
Test-Case "L3 Ref 形参隐式引用" "$classP`nfun observe(r: Ref[Player]): int { ret 1 }`nfun main(): void { var p: Player = Player(`"bob`"); observe(p); var q = p }" $true ""
Test-Case "L3 Ref 内层类型不符" "$classP`nclass Enemy { var hp: int }`nfun observe(r: Ref[Player]): int { ret 1 }`nfun main(): void { var e: Enemy = Enemy(); observe(e) }" $false "cannot pass Enemy"
Test-Case "L3 & 前缀合法" "$classP`nfun main(): void { var p: Player = Player(`"bob`"); var r = &p }" $true ""
Test-Case "L3 & 子引用明确报错" "$classP`nfun main(): void { var p: Player = Player(`"bob`"); var r = &p.name }" $false "sub-references"
Test-Case "L3 ref/mut 方法" "$classP`nfun observe(r: Ref[Player]): int { ret 1 }`nfun modify(m: Mut[Player]): int { ret 1 }`nfun main(): void { var p: Player = Player(`"bob`"); observe(p.ref()); modify(p.mut()); var q = p }" $true ""
Test-Case "L3 take 落定（var）" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var c = bob.take(); var d = bob }" $false "Empty state"
Test-Case "L3 take 回填（未落定）" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); sout(bob.take()); var d = bob }" $true ""
Test-Case "L3 take 落定（Content 形参）" "$classP`nfun consume(c: Content[Player]): int { ret 1 }`nfun main(): void { var bob: Player = Player(`"bob`"); consume(bob.take()); var d = bob }" $false "Empty state"
Test-Case "L3 fill/clear 状态循环" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var c = bob.take(); bob.clear(); bob.fill(c) }" $true ""
Test-Case "L3 fill 前置必须是 E" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var c: Content[Player] = null; bob.fill(c) }" $false "not Empty"
Test-Case "L3 release 合法（Pool 出生地 + E 态）" "$classP`nfun main(): void { var pool: Pool = Pool.fixed(10); var a: Player pool = Player(`"bob`"); a.clear(); pool.release(a) }" $true ""
Test-Case "L3 release 跨池报错" "$classP`nfun main(): void { var p1: Pool = Pool.fixed(10); var p2: Pool = Pool.fixed(10); var a: Player p1 = Player(`"bob`"); a.clear(); p2.release(a) }" $false "cross-pool"
Test-Case "L3 release 默认出生地报错" "$classP`nfun main(): void { var pool: Pool = Pool.fixed(10); var a: Player = Player(`"bob`"); pool.release(a) }" $false "no Pool birth place"
Test-Case "L3 release 非 E 态报错" "$classP`nfun main(): void { var pool: Pool = Pool.fixed(10); var a: Player pool = Player(`"bob`"); pool.release(a) }" $false "not Empty"
Test-Case "L3 同一语句二次 take 报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); sout(bob.take(), bob.take()) }" $false "already taken"
Test-Case "L3 挂起期间 clear 报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); sout(bob.take(), bob.clear()) }" $false "pending take"

Write-Host ""
Write-Host "=== 汇总：PASS=$script:pass FAIL=$script:fail ==="
Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue
if ($script:fail -gt 0) { exit 1 }
