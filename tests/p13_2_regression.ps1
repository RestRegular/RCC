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
Test-Case "L3 子引用 &p.name 合法（决议 §4）" "$classP`nfun main(): void { var p: Player = Player(`"bob`"); var r = &p.name }" $true ""
Test-Case "L3 & 字面量报错" 'fun main(): void { var r = &42 }' $false "requires a Body variable"
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
Test-Case "L3 活跃子引用阻止 clear（决议 §4）" "$classP`nfun main(): void { var p: Player = Player(`"bob`"); var r = &p.name; p.clear() }" $false "active sub-references"
Test-Case "L3 NLL：最后使用后 clear 合法" "$classP`nfun main(): void { var p: Player = Player(`"bob`"); var r = &p.name; sout(r); p.clear() }" $true ""
Test-Case "L3 get_mut 不同下标不冲突（i≠j）" "fun pair(a: Mut[int], b: Mut[int]): int { ret 1 }`nfun main(): void { var xs = [1,2,3]; var x = pair(xs.get_mut(0), xs.get_mut(1)) }" $true ""
Test-Case "L3 get_mut 相同下标冲突" "fun pair(a: Mut[int], b: Mut[int]): int { ret 1 }`nfun main(): void { var xs = [1,2,3]; var x = pair(xs.get_mut(0), xs.get_mut(0)) }" $false "conflicting mutable references"
Test-Case "L3 from：不得返回 local 引用（决议 §5.2）" "$classP`nfun bad(): Ref[str] { var p: Player = Player(`"bob`"); ret &p.name }`nfun main(): void { }" $false "cannot return a reference to local"
Test-Case "L3 from：返回形参引用合法" "$classP`nfun good(p: Ref[Player]): Ref[str] { ret &p.name }`nfun main(): void { }" $true ""
Test-Case "L3 ensures 只能列 Take 形参（决议 §9.2）" "fun bad(t: int): int ensures[t] { ret t }`nfun main(): void { var x = bad(1) }" $false "requires a Take"
Test-Case "L3 ensures Take 形参 ?t 合法" "$classP`nfun good(t: Take[Player]): int ensures[?t] { ret 1 }`nfun main(): void { }" $true ""
Test-Case "L3 ensures !t 未消费报错（决议 §9.2）" "$classP`nfun bad(t: Take[Player]): int ensures[!t] { ret 1 }`nfun main(): void { }" $false "unknown state"
Test-Case "L3 Take 形参调用点隐式 take（决议 §7.3）" "$classP`nfun consume(t: Take[Player]): int { ret 1 }`nfun main(): void { var bob: Player = Player(`"bob`"); consume(bob); var x = bob }" $false "Empty state"
Test-Case "L3 泛型定义禁 writes[p.field]（决议 §5.3）" "fun setv[T](b: T, x: T): int writes[b.v] { ret 1 }`nfun main(): void { }" $false "instance-level writes"
Test-Case "L3 泛型定义允许 writes[p]" "fun setv[T](b: T, x: T): int writes[b] { ret 1 }`nfun main(): void { }" $true ""

Write-Host ""
Write-Host "=== L3-G：分支/循环状态合流（决议 §8.1 / §14） ==="
# 互斥分支各 take 一次 → 合流 E，不再假报错（修前 inner 写穿外层，第二个分支误判）。
Test-Case "L3-G 互斥分支各 take 一次合法" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { var x = bob.take() } else { var y = bob.take() } }" $true ""
# 一个分支 take、另一个不动 → 合流 ?，后续直接读被拒（§3.4）。
Test-Case "L3-G 分支只一侧 take 后读报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { var x = bob.take() }; sout(bob.name) }" $false "? state"
Test-Case "L3-G 分支只一侧 take 后 take 报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { var x = bob.take() }; var y = bob.take() }" $false "? state"
# 两侧都 clear → 合流 E，后续 fill 合法（这是最典型的「合流不能退化成 ?」反例）。
Test-Case "L3-G 两侧都 clear 后 fill 合法" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { bob.clear() } else { bob.clear() }; var c: Content[Player] = null; bob.fill(c) }" $true ""
Test-Case "L3-G 嵌套 if 两侧都 clear 后 fill 合法" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { if (2 > 0) { bob.clear() } else { bob.clear() } } else { bob.clear() }; var c: Content[Player] = null; bob.fill(c) }" $true ""
# 三分支（if/elif/else）全 clear → E；缺 else 时保留 fall-through 路径 → ?。
Test-Case "L3-G 三分支全 clear 后 fill 合法" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { bob.clear() } else if (2 > 0) { bob.clear() } else { bob.clear() }; var c: Content[Player] = null; bob.fill(c) }" $true ""
Test-Case "L3-G 缺 else 时合流为 ? 后 fill 报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { bob.clear() } else if (2 > 0) { bob.clear() }; var c: Content[Player] = null; bob.fill(c) }" $false "? state"
Test-Case "L3-G 分支内 take 后 ? 态 clear 报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { var x = bob.take() }; bob.clear() }" $false "? state"
# 分支内声明的同名局部变量不得把状态泄漏到外层同名变量（q6 反例）。
Test-Case "L3-G 分支内同名局部不污染外层" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); if (1 > 0) { var bob: Player = Player(`"z`") }; var c: Content[Player] = null; bob.fill(c) }" $false "not Empty"
# 循环：体内 clear 且回边不恢复 → 不动点 ?（§14），循环后直接读报错。
Test-Case "L3-G 循环体 clear 后读报错" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var i = 0; while i < 2 { i += 1 bob.clear() }; sout(bob.name) }" $false "? state"
# 循环：体内 clear 后 fill，回边回到 F = 入口 F，不动点仍是 F，读合法。
Test-Case "L3-G 循环体 clear+fill 后读合法" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var i = 0; while i < 2 { i += 1 bob.clear() var c: Content[Player] = null bob.fill(c) }; sout(bob.name) }" $true ""
# break/ret 出口参与合流（§14：不动点 = 入口 ⊔ 回边 ⊔ break 出口）。
# 入口 F、出口 E → 合流 ?，报的是「? state」而不是「not Empty」，
# 后者说明 break 出口被丢了（那样只会看到入口的 F）。
Test-Case "L3-G break 出口参与合流（F⊔E=?)" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); var i = 0; while i < 2 { i += 1 bob.clear() ret }; var c: Content[Player] = null; bob.fill(c) }" $false "? state"
# 入口已 E、出口也 E → 合流 E，fill 合法。
Test-Case "L3-G break 出口与入口同为 E" "$classP`nfun main(): void { var bob: Player = Player(`"bob`"); bob.clear(); var i = 0; while i < 2 { i += 1 bob.clear() ret }; var c: Content[Player] = null; bob.fill(c) }" $true ""
# Take 形参入口 ? 不透明：读字段/调方法/take 仍合法（§7.3 / §9.2），不被合流检查误伤。
Test-Case "L3-G Take 形参入口 ? 仍可读字段" "$classP`nfun f(t: Take[Player]): str { ret t.name }`nfun main(): void { }" $true ""
Test-Case "L3-G Take 形参入口 ? 仍可 take" "$classP`nfun f(t: Take[Player]): int ensures[!t] { var c = t.take() ret 1 }`nfun main(): void { }" $true ""

Write-Host ""
Write-Host "=== L3-H：ensures 出口枚举与重复项（决议 §9.2） ==="
# throw 出口不检查 ensures：整个函数体只有 throw 时，没有「走到末尾」这个正常出口。
Test-Case "L3-H 只有 throw 时不校验 ensures" "$classP`nfun f(t: Take[Player]): int ensures[!t] { throw `"x`" }`nfun main(): void { }" $true ""
Test-Case "L3-H 只有 throw 时 +t 也不校验" "$classP`nfun f(t: Take[Player]): int ensures[+t] { throw `"x`" }`nfun main(): void { }" $true ""
# 但 throw 之后还有正常出口时，正常出口仍须满足 ensures（不能因有 throw 就整体豁免）。
Test-Case "L3-H throw 之后仍有 ret 出口须满足 ensures" "$classP`nfun f(t: Take[Player]): int ensures[!t] { if (1 > 0) { throw `"x`" }; ret 1 }`nfun main(): void { }" $false "unknown state"
Test-Case "L3-H throw 之后 ret 前已消费则合法" "$classP`nfun f(t: Take[Player]): int ensures[!t] { if (1 > 0) { throw `"x`" }; var c = t.take(); ret 1 }`nfun main(): void { }" $true ""
# 重复列同一形参：每项必须只能有一个状态（§9.2）。
# 注意 `!` 的优先级低于逗号，`ensures[!t, !t]` 在 AST 里是 `!(t, !t)`，
# 修前整条子句被静默丢弃（exit=0），必须逐项继承前缀标记才认得出来。
Test-Case "L3-H ensures 重复 !t, !t 报错" "$classP`nfun f(t: Take[Player]): int ensures[!t, !t] { var c = t.take() ret 1 }`nfun main(): void { }" $false "more than once"
Test-Case "L3-H ensures 重复 ?t, ?t 报错" "$classP`nfun f(t: Take[Player]): int ensures[?t, ?t] { ret 1 }`nfun main(): void { }" $false "more than once"
Test-Case "L3-H ensures 重复 t, t 报错" "$classP`nfun f(t: Take[Player]): int ensures[t, t] { var c = t.take() ret 1 }`nfun main(): void { }" $false "more than once"
Test-Case "L3-H ensures 重复 !t, +t 报错（不同状态也算重复）" "$classP`nfun f(t: Take[Player]): int ensures[!t, +t] { var c = t.take() ret 1 }`nfun main(): void { }" $false "more than once"
# 单写仍正常：!t 未消费报错、?t 合法。
Test-Case "L3-H ensures 单写 !t 未消费仍报错" "$classP`nfun f(t: Take[Player]): int ensures[!t] { ret 1 }`nfun main(): void { }" $false "unknown state"
Test-Case "L3-H ensures 单写 ?t 仍合法" "$classP`nfun f(t: Take[Player]): int ensures[?t] { ret 1 }`nfun main(): void { }" $true ""

Write-Host ""
Write-Host "=== 汇总：PASS=$script:pass FAIL=$script:fail ==="
Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue
if ($script:fail -gt 0) { exit 1 }
