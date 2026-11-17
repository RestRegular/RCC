# P13-2 审阅缺陷修复验证（A-J）
# 用法：Invoke-Expression (Get-Content -Raw .\tests\_probe\verify_fixes.ps1)
$ErrorActionPreference = 'Continue'
$rcc = Join-Path $PWD 'release\RCC.exe'
$dir = Join-Path $PWD 'tests\_probe\fix'
if (!(Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }

$player = @'
class Player {
 var name: str
 var hp: int
 ctor(n: str) { this.name = n this.hp = 10 }
 fun greet(): str { ret this.name }
}
'@

# name, source, expect: ERR / OK
$cases = @(
  # ===== A 事实表跨函数泄漏 =====
  @{ n='A1 子引用不跨函数（a 无 ret，b.clear 应通过）'; e='OK';
     s=$player + @'
fun a(): void { var p: Player = Player("x") var r = &p.name }
fun b(p: Player): void { p.clear() }
fun main(): void { }
'@ }
  @{ n='A1ctl 单函数内子引用阻止 clear（对照）'; e='ERR';
     s=$player + @'
fun b(p: Player): void { var r = &p.name p.clear() }
fun main(): void { }
'@ }
  @{ n='A2 出生地不跨函数（release 应报错）'; e='ERR';
     s=$player + @'
fun a(): void { var pl: Pool = Pool.fixed(4) var q: Player pl = Player("x") }
fun b(q: Player, pl: Pool): void { pl.release(q) }
fun main(): void { }
'@ }
  @{ n='A3 移动状态不跨函数（fill 需 E，应报错）'; e='ERR';
     s=$player + @'
fun a(): void { var p: Player = Player("x") p.clear() }
fun b(p: Player): void { p.fill(Player("y")) }
fun main(): void { }
'@ }
  @{ n='A3ctl 单函数 clear 后 fill（对照）'; e='OK';
     s=$player + @'
fun main(): void { var p: Player = Player("x") p.clear() p.fill(Player("y")) }
'@ }

  # ===== B take 状态前置 =====
  @{ n='B1 take 自 Empty'; e='ERR';
     s=$player + @'
fun main(): void { var p: Player = Player("x") p.clear() var c = p.take() }
'@ }
  @{ n='B2 take 两次（跨语句）'; e='ERR';
     s=$player + @'
fun main(): void { var p: Player = Player("x") var a = p.take() var b = p.take() }
'@ }
  @{ n='B3 take 自 Full（对照）'; e='OK';
     s=$player + @'
fun main(): void { var p: Player = Player("x") var c = p.take() }
'@ }

  # ===== C pool.new 类型绑定 =====
  @{ n='C1 pool.new[T] 与声明类型不符'; e='ERR';
     s=$player + @'
class Enemy { var hp: int ctor(h: int) { this.hp = h } }
fun main(): void { var pl: Pool = Pool.fixed(4) var a: Enemy = pl.new[Player]("x") }
'@ }
  @{ n='C2 非 Pool 接收者'; e='ERR';
     s=$player + @'
fun main(): void { var n: Player = Player("n") var a: Player = n.new[Player]("y") }
'@ }
  @{ n='C3 pool.new 类型一致（对照）'; e='OK';
     s=$player + @'
fun main(): void { var pl: Pool = Pool.fixed(4) var a: Player = pl.new[Player]("x") }
'@ }

  # ===== D ensures[+a] =====
  @{ n='D1 ensures[+p] 非 Take 形参'; e='ERR';
     s=$player + @'
fun f(p: Player): int ensures[+p] { ret 1 }
fun main(): void { }
'@ }
  @{ n='D2 ensures[+zzz] 未知形参'; e='ERR';
     s=$player + @'
fun f(p: Player): int ensures[+zzz] { ret 1 }
fun main(): void { }
'@ }
  @{ n='D3 ensures[+t] 未消费（Take 入口态 ?，应报错）'; e='ERR';
     s=$player + @'
fun f(t: Take[Player]): int ensures[+t] { ret 1 }
fun main(): void { }
'@ }
  @{ n='D4 ensures[!t] 未消费（对照）'; e='ERR';
     s=$player + @'
fun f(t: Take[Player]): int ensures[!t] { ret 1 }
fun main(): void { }
'@ }

  # ===== F get_mut NLL =====
  @{ n='F1 get_mut 同下标但前次已最后使用'; e='OK';
     s=@'
fun main(): void {
 var xs = [1,2,3]
 var a = xs.get_mut(0)
 var t = 5
 var b = xs.get_mut(0)
}
'@ }
  @{ n='F2 get_mut 同语句仍冲突（对照）'; e='ERR';
     s=@'
fun main(): void {
 var xs = [1,2,3]
 var a = xs.get_mut(0)
 var b = xs.get_mut(0)
 sout(a)
 sout(b)
}
'@ }
  @{ n='F3 get_mut 不同下标（对照）'; e='OK';
     s=@'
fun main(): void {
 var xs = [1,2,3]
 var a = xs.get_mut(0)
 var b = xs.get_mut(1)
}
'@ }

  # ===== G writes / from 校验 =====
  @{ n='G1 writes[q.name] q 非形参'; e='ERR';
     s=$player + @'
fun f(p: Player): void writes[q.name] { }
fun main(): void { }
'@ }
  @{ n='G2 from[zz.name] zz 不存在'; e='ERR';
     s=$player + @'
fun f(p: Player): Ref[Player] from[zz.name] { ret &p }
fun main(): void { }
'@ }
  @{ n='G3 from[p] 却 ret &q'; e='ERR';
     s=$player + @'
fun f(p: Player, q: Player): Ref[Player] from[p] { ret &q }
fun main(): void { }
'@ }
  @{ n='G4 from[p] 且 ret &p（对照）'; e='OK';
     s=$player + @'
fun f(p: Player, q: Player): Ref[Player] from[p] { ret &p }
fun main(): void { }
'@ }
  @{ n='G5 writes[p.name] 合法（对照）'; e='OK';
     s=$player + @'
fun f(p: Player): void writes[p.name] { p.name = "z" }
fun main(): void { }
'@ }

  # ===== H 出生地标签唯一 =====
  @{ n='H1 两个出生地标签'; e='ERR';
     s=$player + @'
fun main(): void { var p1: Pool = Pool.fixed(4) var p2: Pool = Pool.fixed(4) var a: Player p1 p2 = Player("x") }
'@ }
  @{ n='H2 单个出生地标签（对照）'; e='OK';
     s=$player + @'
fun main(): void { var p1: Pool = Pool.fixed(4) var a: Player p1 = Player("x") }
'@ }

  # ===== I §4.2 writes 调用点效果系统 =====
  @{ n='I1 调用 writes[p.name] 时 &p.name 活跃'; e='ERR';
     s=$player + @'
fun w(p: Player): void writes[p.name] { p.name = "y" }
fun main(): void { var p: Player = Player("a") var r = &p.name w(p) sout(r) }
'@ }
  @{ n='I2 同上但 r 已最后使用（NLL 放行）'; e='OK';
     s=$player + @'
fun w(p: Player): void writes[p.name] { p.name = "y" }
fun main(): void { var p: Player = Player("a") var r = &p.name sout(r) w(p) }
'@ }
  @{ n='I3 writes[p] 整参数 vs 活跃 &p.name'; e='ERR';
     s=$player + @'
fun w(p: Player): void writes[p] { p.name = "y" }
fun main(): void { var p: Player = Player("a") var r = &p.name w(p) sout(r) }
'@ }
  @{ n='I4 writes[p.hp] 与 &p.name 不相交（对照）'; e='OK';
     s=$player + @'
fun w(p: Player): void writes[p.hp] { p.hp = 3 }
fun main(): void { var p: Player = Player("a") var r = &p.name w(p) sout(r) }
'@ }
  @{ n='I5 无 writes 标注不误报（对照）'; e='OK';
     s=$player + @'
fun w(p: Player): void { p.name = "y" }
fun main(): void { var p: Player = Player("a") var r = &p.name w(p) sout(r) }
'@ }
  @{ n='I6 本地字段赋值 vs 活跃子引用'; e='ERR';
     s=$player + @'
fun main(): void { var p: Player = Player("a") var r = &p.name p.name = "y" sout(r) }
'@ }
  @{ n='I7 不相交字段赋值放行（对照）'; e='OK';
     s=$player + @'
fun main(): void { var p: Player = Player("a") var r = &p.name p.hp = 5 sout(r) }
'@ }
  @{ n='I8 take vs 活跃子引用（§4.2 根替换）'; e='ERR';
     s=$player + @'
fun main(): void { var p: Player = Player("a") var r = &p.name var c = p.take() }
'@ }
  @{ n='I9 mut() vs 活跃子引用'; e='ERR';
     s=$player + @'
fun main(): void { var p: Player = Player("a") var r = &p.name var m = p.mut() sout(r) }
'@ }
  @{ n='I10 路径前缀按段比较（p.n 不是 p.name 前缀）'; e='OK';
     s=$player + @'
fun main(): void { var p: Player = Player("a") var r = &p.name var q: Player = Player("b") q.name = "z" sout(r) }
'@ }

  # ===== J 内存模型形参的字段/方法访问（classNameOf 穿透）=====
  @{ n='J1 Take 形参调方法'; e='OK';
     s=$player + @'
fun f(t: Take[Player]): str { ret t.greet() }
fun main(): void { }
'@ }
  @{ n='J2 Take 形参读字段'; e='OK';
     s=$player + @'
fun f(t: Take[Player]): str { ret t.name }
fun main(): void { }
'@ }
  @{ n='J3 Ref 形参读字段'; e='OK';
     s=$player + @'
fun f(r: Ref[Player]): str { ret r.name }
fun main(): void { }
'@ }
  @{ n='J4 Mut 形参写字段'; e='OK';
     s=$player + @'
fun f(m: Mut[Player]): void { m.name = "z" }
fun main(): void { }
'@ }
  @{ n='J5 Content 形参调方法'; e='OK';
     s=$player + @'
fun f(c: Content[Player]): str { ret c.greet() }
fun main(): void { }
'@ }
  @{ n='J6 Body 形参 clear'; e='OK';
     s=$player + @'
fun f(b: Body[Player]): void { b.clear() }
fun main(): void { }
'@ }
  @{ n='J7 return t.take() 落定满足 ensures[!t]'; e='OK';
     s=$player + @'
fun f(t: Take[Player]): Content[Player] ensures[!t] { return t.take() }
fun main(): void { }
'@ }
  @{ n='J8 var c = t.take() 落定满足 ensures[!t]（对照）'; e='OK';
     s=$player + @'
fun f(t: Take[Player]): int ensures[!t] { var c = t.take() ret 1 }
fun main(): void { }
'@ }
)

$pass = 0; $fail = 0
foreach ($c in $cases) {
  $file = Join-Path $dir (([guid]::NewGuid().ToString('N').Substring(0,8)) + '.rio')
  [IO.File]::WriteAllText($file, $c.s, [Text.UTF8Encoding]::new($false))
  $out = & $rcc build --p $file 2>&1 | Out-String
  $code = $LASTEXITCODE
  $actual = if ($code -eq 0) { 'OK' } else { 'ERR' }
  $firstErr = @($out -split "`n" | Where-Object { $_ -match 'Error|L3:|L[0-9]-' })[0]
  if ($actual -eq $c.e) { $pass++; Write-Host ("  [PASS] " + $c.n) }
  else {
    $fail++
    Write-Host ("  [FAIL] " + $c.n + "  期望=" + $c.e + " 实际=" + $actual)
    if ($firstErr) { Write-Host ("         " + ($firstErr -replace '\s+',' ').Trim()) }
  }
  Remove-Item $file -ErrorAction SilentlyContinue
}
Write-Host ""
Write-Host ("=== 汇总：PASS=$pass FAIL=$fail ===")
