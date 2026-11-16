# P12 —— 收尾（删旧 RA 路径 / 补语言缺口 / 重写标准库 / 脚本与发版）

> 目标：把 P0–P11 的成果收成一个**仓库干净、能发版**的编译器。
> 本阶段不再新增语言机制（除了标准库真正需要的那几项），重点是**删、补、重写、发版**。
>
> 验收：`test_p0`–`test_p11` 全绿 + legacy 15/17（`test_14`/`test_15` 因 G6 延期，见 §10.2）
> \+ `lib/std` **三层**验收全绿（§5.6）+ `-s` 六种格式全部可用（§3.3）+ 仓库无 LLVM 残留
> \+ §7.1 的 Breaking 表填实。
>
> **总纲见 §11「总体执行原则」**；任何小节与本计划冲突时以 §11 为准。
> **待设计项**：G1b（argv 传递，§4.2）、G6（闭包写回，§4.3）—— P12 不实现，见 §10.2。

## 0. 前置：先冻结基线，再动手

P0–P11 的改动目前**全部未提交**（`git status` 里 12 个 `M`、几十个 `??`，
HEAD 还停在 `93eaa2e`）。P12 要删掉约 8000 行 legacy 代码，如果和前面 11 个阶段
的改动混在一个 diff 里，review 和回滚都会失控。

所以 P12-0 是纯机械步骤，**没有任何代码改动**：

```powershell
# 1) 确认基线：全绿 + legacy 15/17（test_14/15 属本阶段）
powershell -File test_scripts/test_legacy_all.ps1
foreach ($p in 0,1,2,5,6,7,8,9,10,11) {
    powershell -File "test_scripts/test_p${p}_*.ps1"
}

# 2) 提交 P0–P11
git add -A
git commit -m "feat(backend): rewrite RCC on x86-64 GAS backend (P0-P11)"
```

之后的每个子阶段各自一个提交，P12 的 diff 才读得懂。

## 1. 实测缺口清单（P12 的设计输入）

下面每一条都在 `release/RCC.exe` 上**实测过**（探针放在 `build/p12probe/`），
不是推断。列这张表的目的：P12 只补「标准库真的需要」的，其余明确写成不支持。

### 1.1 已经能用（不用动）

| 能力 | 实测 |
|---|---|
| `if / elif / else`、`while`、`until`、`for ({var i=0; i<n; i++})` | ✅ |
| `break` / `continue`（含 `for` 循环里 `continue`） | ✅ |
| `ret`（裸 `ret`、`ret expr`）、提前 `ret` | ✅ 见下方 1.3 的解析坑 |
| 具名实参 `g(1, b=9)`、缺省实参 `fun g(a: int, b: int = 5)` | ✅ |
| 函数重载（`fun f(x: int)` / `fun f(x: str)`），**含跨模块** | ✅ `m.f(1)` / `m.f("a")` 各自决议正确 |
| 泛型函数 / 泛型类（`first[T](a: list[T]): T`、`Box[int](5)`） | ✅ |
| 类：`ctor`、字段、方法、`this.x`、`this.field[i]`、`obj.field = v` | ✅ |
| 匿名函数、具名函数当函数值传参、`funi[int][int]` 形参 + `funi[any, int, int, .*][any]` | ✅ |
| 无捕获闭包、有捕获闭包（按值） | ✅ 读得到捕获值 |
| `core_str_len(x)` 对 **list / dict / str** 都返回正确长度 | ✅ 3 / 4 / 2 |
| `sout` 多实参混类型、`end=` 关键字实参、`sout()` 特化 | ✅ |
| `ptr` 形参接收 list/str（族内互通）、`core_str_len(ptr)` | ✅ |
| `null`、`var a = 1, b = 2`（多声明）、`var x: int const = 3` | ✅ |
| 模块内联 + `mod.fn(...)` 调用（`import(m="mod1")`） | ✅ |

### 1.2 当前不支持的能力（分成「补缺口」与「待审批提案」两类）

**A. 属于「实现没跟上原仓库已有设计」的缺口 —— P12-2 直接补**

| # | 缺口 | 实测报错 / 现象 | 原仓库依据 | P12 决定 |
|---|---|---|---|---|
| G1a | 顶层 `var` 在函数体里不可见 | `Undefined variable: g` | 原仓库有 `ScopeType::GLOBAL` / `enterGlobalScope()`；`math.rio` 的 `var pi: float const` 就是顶层 var | **必补**：全局静态槽（详见 §4.1） |
| G1b | 程序入口与运行参数传递 | 无 argv 通道（见 §4.2 实测） | **原始设计里没有 argv 传递** —— 详见 §4.2 的核实结论 | ⚠️ **待设计**：需先提交提案 |
| G3 | `str + str` | 编译成整数加法：`cannot assign a value of type int to a target of type str` | 原仓库 `lowlevel.rio:53` `var riString: str = i + ": "`、`str.rio:111` `[""] + result + ""` 都在用 | **必补**：`rt_str_concat` |
| G4 | `list + list` | `Index access on a non-container value`（推断成 int） | 原仓库 `str.rio:111` 用 `[""] + result + ""` 做列表拼接 | **必补**：`rt_list_concat` |
| G5 | 方法重载（类内同名不同签名） | `Argument 1 of 'A.m': cannot assign a value of type int ...` | P11 已在全局函数上实现，类方法漏做 | **必补**：复用 P11 mangling |
| G6 | 闭包捕获**写回** | `sum1` 保持 0 | 原仓库 `ds.iter` / `ds.forInRange` 的契约就是「回调里改外层变量」 | ⚠️ **待设计**：须先对齐内存模型（详见 §4.3） |
| G7 | `{ ret }` —— `ret` 与 `}` 同一行 | ParserError `TOKEN_RBRACE` 找不到前缀构造 | 原仓库裸 `ret` 是合法语法，`ds.rio` 里大量使用 | **必补**：parser 一行小修 |
| G9 | `str` 相等比较按**指针**比 | `"x" == "x"` → `false`；`var a="ab" var c="ab"` → `a != c` 成立 | 原仓库 `str.rio:105` `if del == ""`、`datatype.rio:92` `typeOfArg1 == arg2[i]` 都依赖**内容**比较；legacy `ITER_GET`/字典查找也按内容比 | **必补**：`emitBinary` 在 str 分支改调 `core_mem_eq` |
| G10 | `str` 下标 `s[i]` | `Index access on a non-container value` | 原仓库 v1.21.2 **实测支持读取**（`sout(s[1])` → `e`）；但**写入是静默失效的 bug**（`s[1]="X"` 输出仍 `hello`，因 VM 的 `String::getDataAt` 返回副本） | **必补（只读不写）**：`emitIndexExpression` 加 TK_STR 分支返回 `char`；`s[i]=c` **编译期报错**。详见 §G10 |
| G11 | 类字段初始化器 `var x: int = 42` 不生效 | 读出来是 `0`（只有 ctor 里赋值才有效） | 原仓库 `datatype.rio:50` `var TP_INT: str static = type(0)`、`ds.rio:492` `compareHandler: funi[...] private = null` 都是字段初始化器 | **必补**：`registerClassBody` 记录初值，构造时写入 |
| G12 | `ptr` 返回值标注不可用（无 cast） | `fun f(): ptr { var p = core_alloc(...) ret p }` → `cannot assign a value of type int to a target of type ptr` | 原仓库没有 `ptr` 类型（`builtInTypes` 里无 `ptr`），这是**新架构自己引入**的类型；但 `core.rio` 的 `alloc`/`realloc` 必须能返回指针，否则 std 无法分配内存 | **必补**：`ptr` 与 `int` 视为同族（`sameValueFamily` 放行），或允许 `ptr` 形参接受 `alloc` 结果 |
| G13 | `any` 作为**显式标注**不认 | `fun f(a: any): int` → `Unknown type 'any'`；但**省略标注**完全正常 | 原仓库 `builtInTypes` 里**有** `any`（`rcc_symbol.cpp:650`），`ds.rio:32` `fun copy(arg): any`、`math.rio:26` `fun pow(a, times: int): any` 都在用 | **必补**（低优先级）：`parseTypeName` 加 `any` → `getUnknownType()`（等于「不标注」），语义正好等价 |

**B. 属于「新增语言特性」的提案 —— 全部已否决，不改实现（详见 §1.4）**

| # | 提案 | 实测现象 | 状态 |
|---|---|---|---|
| A4 / G8 | `ptr` 索引 `p[i]` | `Index access on a non-container value` | ❌ **否决** |
| A5 / G2 | 索引赋值 `a[i] = v`、`d[k] = v` | `Only simple variable assignment is supported` | ✅ **2026-11-16 用户改为要求支持**（详见 §G2） |
| A6 | list 自动扩容 | 无此语义 | ❌ **否决** |

### 1.3 明确**不做**（写进文档，不再当作缺陷）

| 项 | 理由 |
|---|---|
| `type` / `setType` / `checkType` / `isIter` 运行时自省 | 新架构是静态的；这些是 RA 时代给动态语义兜底的。标准库不再依赖 |
| 静态类字段 `C.X` | 用顶层 `var`（G1a 修好后就是天然的全局常量）替代，少一套机制 |
| `encapsulated` 在用户文件里当桩 | 只在 rt/std 内部有意义；用户写了就是链接错误，行为正确 |

> **本节只允许出现「原仓库本来就没有、且不需要新增语法就能绕开」的项。**
> 任何「新增语法 / 新增语言特性 / 删除或重命名原仓库已有 API」一律进 §1.4，等审批。

#### 1.3.1 兼容性定位（2026-11-16 用户评审第 4 条）

> **兼容性不是 P12 的最高目标；正确、统一、可维护的新架构才是。**

| 不做什么 | 说明 |
|---|---|
| ❌ 不为兼容旧 RA 架构保留 legacy 实现 | 18 个文件 ~8.6k 行全删（§3） |
| ❌ 不为没有必要的旧接口增加实现成本 | 原理上做不了的就删，不写空壳 |
| ❌ 不为无法实现的旧 API 制造替代实现 | 恒等函数、永远返回 `null` 的桩都是反模式 |
| ✅ 对不再需要的功能，在明确记录变更的前提下删除 | 记录进 §7.1 的 Breaking 表 |
| ⚠️ 涉及语言设计 / 公开 API 的删改，**仍须审批** | 这条不变，见 §1.4 |

**判断口径**：一个旧 API 该不该留，只看「新架构下它是否有意义、是否能正确实现」，
**不看**「删了会不会让老代码编不过」。老代码编不过是可以接受的，写进迁移说明即可。

### 1.4 我提的提案 —— **全部未获批准，不得写进实现**

下面这些**不是**从原仓库源码里读出来的，是我在写本计划时自己提出的。
按 2026-11-16 的约定（用户明确要求）：**语言设计层面的任何变更都必须先提出并获批**，
未获批之前只作为「提案」记录，不进入 P12 的实现范围。

| # | 提案 | 来源核实 | 状态 |
|---|---|---|---|
| A1 | 新增 `for x in a`（for-in 语法） | 原仓库 parser **没有**这个语法（`KEYWORDS` 里无 `in`，`Parser` 无对应 builder）；17 个 legacy 测试里**没有任何一处**使用 for-in 语法，`test_15_forinrange` 名字里有 forin 但实际是调 `ds.forInRange()` 函数 | ❌ **我编的，撤回** |
| A2 | 新增 `switch` 语句 | 原仓库 AST 里**没有** `SwitchNode`，parser 无对应分支 | ❌ **我编的，撤回** |
| A3 | 新增 `0..3` range 字面量 | 原仓库只有 `ParenRangerNode` / `BlockRangerNode` / `BracketExpressionNode`（括号/块/方括号的语法包装），**不是**区间语法 | ❌ **我编的，撤回**（`docs/p1_plan.md:42` 写错过，已改） |
| A4 | `ptr` 索引 `p[i]`（G8） | 原仓库 `.rio` 代码里**没有任何 `p[i]` 用法**，`core.rio`/`rt.rio` 全靠 `p + 8` 裸指针算术 | ❌ **否决**（2026-11-16） |
| A5 | 索引赋值 `a[i] = v` / `d[k] = v`（G2） | ✅ **原仓库 v1.21.2 实测支持**（用户实测反证）。机制：RA VM 的 `ITER_GET` 返回 `shared_ptr`，改临时变量即写回容器 | ⚠️ **第三轮误判为「否决」，第五轮已纠正**：这是**新架构的功能回归**，应恢复。详见 §G2 |
| A6 | `list` 自动扩容 | 原仓库靠 `listAppend` 内置函数（RA 层）实现，没有「`a[len] = v` 自动增长」的语义 | ❌ **否决**（2026-11-16）；越界一律 panic |
| A7 | 重设计 `str` 模块 API（`len`/`at`/`sub`/`upper`/`lower`/`contains`/`indexOf`/`startsWith`/`endsWith`/`toInt`/`fromInt`） | 原仓库真实 API 是 `toStr`/`toChar`/`subStr`/`find`/`split`/`chunk`/`replace`/`toLowerCase`/`toUpperCase`/`charToInt`/`intToChar`/`trim` | ⬜ **不采纳**（见 S7：名字无明显问题，改名只是风格偏好） |
| A8 | 重设计 `math` 模块（删 `opp`，加 `floor`/`ceil`/`sign`） | 原仓库真实 API 是 `pi`/`e`/`pow`/`max`/`min`/`opp`/`abs` | ⬜ **不采纳**（见 S8：`opp` 语义明确，改名破坏兼容） |
| A9 | 重设计 `prog` 模块（`exit`/`abort`/`panic`/`timeNs` 取代 `entry`/`breakpoint`） | 原仓库真实 API 是 `entry`/`breakpoint` | 部分采纳：`entry` 删（见 S5），`breakpoint` 保留；`exit`/`abort`/`panic`/`timeNs` 属新增，不采纳 |
| A10 | 重设计 `lowlevel` 模块（`memCopy`/`memMove`/`memSet`/`memEq`/`alloc`/`free`/`realloc` 取代 `id`/`rasm`/`ri`/`bindDllExt`/`unbindDllExt`） | 原仓库真实 API 是 `id`/`rasm`/`ri`/`bindDllExt`/`unbindDllExt` | 部分采纳：删 `id`/`rasm`/`bindDllExt`/`unbindDllExt`（见 S1），保留 `ri`（S2）；**不新增** `memCopy` 等（用户代码不该直接碰内存） |
| A11 | 删 `ds` 的 `unpack`/`top`/`iterContains`/`iterRemove`/`dictRemove`/`repeat` | 原仓库这 6 个都是公开 API | 部分采纳：只删 `unpack`/`iterContains`/`iterRemove`（见 S4，原理上无法实现）；`top`/`dictRemove`/`repeat` **保留** |
| A12 | 重设计 `ops` 模块内容 | 原仓库真实 API 是 `ternary`/`ternaryLazy`/`orVal`/`orLazy` | ⬜ **不采纳**（见 S9：4 个都没问题） |

**处理原则**：A1–A3 直接撤回（本来就不存在，不该出现在计划里）。
A4–A12 在获得批准前，P12 的标准库按**原仓库 API 面**重写实现（实现方式可以新写，
但函数名、参数、返回值保持不变），这样即使提案全部被否，P12 也照样能交付。

### 1.5 审批结果（2026-11-16，用户答复）

| 问题 | 用户决定 | 对 P12 的影响 |
|---|---|---|
| 是否批准 A4/A5/A6 三个语言特性提案 | **全部不批准**，备注「我先看看了来，不着急」 | A4/A6 标 ❌ **否决**，不进入实现。<br>**A5 在第四轮被用户改判为要求支持**（`list[...] = x`），见 §G2 与 §1.5.1 |
| 标准库 API 面是否允许我提改名/增删建议 | **允许，逐条审批** | A7–A12 进入逐条审批流程，见 §5.4 的逐条清单；未批前按原 API 面重写 |
| 新架构无法实现的旧函数怎么处理 | **先逐个核实，再列清单请你定** | 已完成核实，见 §5.5「无法实现的旧函数清单」 |

### 1.5.1 第四轮审批结果（2026-11-16，本轮）

| # | 问题 | 用户决定 | 对 P12 的影响 |
|---|---|---|---|
| 1 | S1–S4、S6 删除清单 | ✅ **批准** | 删 `lowlevel.id/rasm/bindDllExt/unbindDllExt`、整个 `datatype` 模块、`ds.unpack/iterContains/iterRemove` |
| 2 | 是否新增 `listSet`/`dictSet` | ✅ **不新增**，改用 `list[...] = x` 索引赋值 | ✅ 用户判断正确：原仓库 v1.21.2 实测**支持**索引赋值，这是新架构的**功能回归**。详见 §G2 |
| 3 | `prog.entry` 如何处置 | ✅ **P12 不动，登记待设计** | 既不实现 argv 通道，也不删除。见 §4.2.3、§10.2 |
| 4 | G6 是否延期 | ✅ **同意延期到 P13** | P12 不写 boxed cell，见 §4.3 |
| 5 | 内存模型冲突（规范禁引用计数 vs 架构计划建议引用计数） | ✅ **以规范为准，改架构计划** | `architecture_plan.md §8.1` 需改写，`rc` 字段标注废弃，见 §4.3.1 |
| 6 | 内存模型规范的定位 | 用户说明：「**这份设计是在脱离 Rio 的情况下设计出来的，里面的示例代码都是伪代码**，需要根据 Rio 的语法和语言特色进行修改」 | 规范不能直接作为实现依据；**Rio 映射本身是设计工作**，见 §4.3.1 冲突 B |

## 2. 子阶段划分

| 子阶段 | 内容 | 产出验收 |
|---|---|---|
| **P12-0** 冻结基线 | 跑全套测试；提交 P0–P11 | 一次干净提交，工作树干净 |
| **P12-1** 删旧 RA 路径 | 依赖审计（§3.2 A1–A7）→ 重新设计 `-s`（§3.3）→ 删 `RCC_LEGACY_SOURCES` 全部 + visitor/interface 骨架 | 审计七项有结论；构建通过；`-s` 六种格式可用；行数 −8000；**独立提交** |
| **P12-2** 补语言缺口 | ✅ 做：G1a / G2 / G3 / G4 / G5 / G7 / G9 / G10 / G11 / G12<br>⏳ 待设计：G1b（§4.2）、G6（§4.3）<br>❌ 不做：G8（已否决） | 每条实现的缺口配一个回归用例；待设计项**不写代码** |
| **P12-3** 重写标准库 | `lib/std/*.rio` 全新实现，替换 `release/Lib/builtin/` | **三层验收**（§5.6）：编译 + API + 行为全绿 |
| **P12-4** 脚本重写 | `run.bat` / `test.bat` 去 LLVM；新增统一测试入口 | `scripts\test.bat` 一条命令跑完全部 |
| **P12-5** 文档与发版 | `rio_doc.md` 重写、版本号、更新日志、README | `RCC -v` = 1.22.0；文档无占位；§7.1 Breaking 表六项填实 |

## 3. P12-1：删旧 RA 路径

### 3.1 现状

`RCC_LEGACY_SOURCES` 一共 18 个文件、约 8.6k 行，全部是 LLVM 时代的 RA 发射器：

| 文件 | 行数 | 用途 | 处置 |
|---|---|---|---|
| `visitors/rcc_compile_visitor.cpp` | 3922 | RA 发射主路径（`-ir` 时代） | **删** |
| `components/symbol/rcc_symbol.cpp` | 2692 | 旧符号表 | **删** |
| `visitors/tools/rcc_function_call_compiler.cpp` | 656 | 旧调用编译 | **删** |
| `builtin/rcc_builtin_core.cpp` | 342 | 内置函数注册（RA 指令） | **删** |
| `components/ri/rcc_ri.cpp` | 273 | RA 指令集定义 | **删** |
| `builtin/functions/rcc_builtin_import.cpp` | 246 | 旧 import（P8 已由 module_loader 取代） | **删** |
| `builtin/rcc_builtin.cpp` | 143 | 内置函数管理器 | **删** |
| `visitors/rcc_collect_symbol_visitor.cpp` | 131 | 旧符号收集 | **删** |
| `builtin/dll_extension_manager/*.cpp` | 130 | DLL 扩展加载（LLVM 时代） | **删** |
| `builtin/functions/rcc_builtin_{ds,datatype,export,io,lowlevel,prog,super}_functions.cpp` | 各 27–120 | 各模块内置实现 | **删** |
| `visitors/rcc_print_visitor.cpp` | 141 | 旧 AST 打印（`-s` 用） | **删** |
| `visitors/rcc_json_visitor.cpp` | 482 | 旧 AST→JSON（`-s --format json` 用） | **删** |

### 3.2 删除前的依赖审计（**必须先完成，全部有结论才能动手删**）

审计分七步，每步都要留下可核对的记录（写进 `docs/p12_legacy_audit.md`）：

| # | 审计项 | 命令 / 方法 | 通过标准 |
|---|---|---|---|
| A1 | 被删类/函数/头文件的所有引用 | `git grep -n "CompileVisitor\|SymbolTable\|CollectSymbolVisitor\|PrintVisitor\|JsonVisitor\|rcc_ri\|BuiltinManager"` | 剩余引用全部指向新实现或被同步删除 |
| A2 | 头文件前置声明与接口依赖 | 查 `rcc_ast_dec.h` / `rcc_builtin_interface.h` / `rcc_visitors.h` 里的 forward decl | 无悬空声明（构建期就能发现，但先人工过一遍） |
| A3 | CMake 源列表与链接依赖 | `CMakeLists.txt` 的 `RCC_LEGACY_SOURCES`、`target_link_libraries`、`add_executable` | 源列表整块删除后链接无 undefined symbol |
| A4 | `rcc_main.cpp` 对旧实现的引用 | 查 `-s` 用到的静态成员 `__symbol_flag__` / `__symbol_flag_export__` / `__symbol_option_format__`，以及 `-ir` 相关分支 | 全部改接到新 `SymbolExporter`，`-ir` 分支删除或改报错 |
| A5 | CLI 参数与脚本对旧实现的依赖 | 查 `run.bat` / `test.bat` / `test_scripts/*.ps1` 里是否用 `-ir` / `-s` / `.ll` / `.obj` | 脚本改为 `-build`；`-s` 保留但走新实现 |
| A6 | 测试脚本是否调用旧命令/旧产物 | 跑一遍 `test_legacy_all.ps1`，记录哪些用例依赖 `.ll`/`.obj` | 依赖旧产物的用例标记为「P12 后删除」或改写 |
| A7 | 最终 exe 是否仍依赖旧对象文件 | `nm release/RCC.exe \| grep -i "ri::\|CompileVisitor"`；对比删除前后 `dumpbin /dependents` | 无旧符号残留 |

### 3.3 `-s` 的重新设计（**已获批，取代逐字节 golden 方案**）

> **决策（用户 2026-11-16 裁决，推翻本节的原始「逐字节 golden」方案）**
>
> 原文要求新 `-s` 与旧输出**逐字节一致**，并把旧输出存成 golden 入库。
> 用户明确否掉了这个方向：*「既然你要新实现 -s 就没有必要保证和原实现一模一样啊，
> 而且旧的 json 的格式也不是很好，而且也不一定非要使用 json 格式」*。
>
> 理由不只是「没必要」——旧的两种格式客观上都不适合作为对外接口：
>
> - 旧 `symbolId` 形如 `name_vidN`，`N` 来自 `utils::Object` 的全局自增计数器，
>   取值取决于**构造顺序**（含被删掉的 visitor 内部临时对象）。从干净的 AST 遍历
>   出发根本无法复现，强行对齐只能靠给导出器人为制造遍历副作用。
> - 旧 JSON 的键序来自 `unordered_map` 的迭代顺序，本身就是不稳定输出，
>   「逐字节一致」在旧实现内部都不成立。
>
> 因此改为**重新设计**：`-s` 的职责就是「导出源码中的符号表」，重写后按
> 「多种导出格式、便于其他程序消费、其中必须有供人查阅的可视化格式」重新定义。

**获批的格式集（用户选定：text + json + html + ctags + dot + markdown）**

| 格式 | 用途 | 主要消费者 |
|---|---|---|
| `text` | 人读的紧凑清单，`-s` 默认 | 终端 / 日志 |
| `json` | 结构化全量数据（含参数、调用关系、位置） | 编辑器、依赖分析、性能分析工具 |
| `html` | **自包含可视化页面**：搜索框 + 种类筛选 + 点击展开详情 | 开发者人工查阅 |
| `ctags` | 扩展 ctags 格式，可直接喂 `vim` / `VSCode` / `ctags` 生态 | 编辑器跳转 |
| `dot` | Graphviz 图：继承图 + 调用图 | 架构分析、可视化 |
| `markdown` | 按种类分表，便于贴进文档 / PR / wiki | 文档生成 |

**可扩展性**：格式在 `rcc_symbol_export.cpp` 里由一张 `kFormats[]` 注册表驱动
（`name` / `extension` / `renderer`），`parseSymbolFormat`、`symbolFormatList`、
`symbolFormatExtension`、帮助文本和错误信息全部从这张表派生。新增一种格式 =
加一个 `render*` 函数 + 加一行注册项，不改任何调用点。

**命名抽取**：mangling 从 `backend::CodeGen` 抽成公共工具
（`code/include/backend/rcc_symbol_naming.h`），`CodeGen` 改为转发、行为零变化；
导出器因此能输出**真实的汇编符号名**（`asmSymbol`）。对重载函数和泛型函数，
导出器**留空** `asmSymbol` 而不是编一个看似合理的名字——它没有类型系统，
算不出 `_R<len><name><形参编码>`，错误的名字会误导 `objdump` / `perf` 等对齐工具。

**验收**：`test_scripts/test_symbol_export.ps1` 校验六种格式各自的**结构性质**
（不是逐字节比对旧输出）——见 §3.5。

### 3.4 `-s` 的实现（已完成）

`-s` 原本**只**通过 `ast::CompileVisitor` 实现，`rcc_main.cpp` 直接引用它的静态成员
（`__symbol_flag__`、`__symbol_flag_export__`、`__symbol_option_format__`）。
删掉 `CompileVisitor` 就必须给 `-s` 一个新实现。

| 方案 | 代价 | 结论 |
|---|---|---|
| A. 保留 CompileVisitor 只为 `-s` | 保留 3922 行 + 整套旧符号表 | ✗ P12 就是来删这个的 |
| B. 去掉 `-s` | 破坏 CLI 兼容；README 里承诺过 | ✗ |
| C. 用新 AST 写独立导出器 | 一次实现，永久摆脱旧路径 | ✅ **已采用** |

实现要点（`code/src/analyzer/rcc_symbol_export.cpp`）：

- 输入：`ast::ProgramNode`，**先**在合并前的 AST 上扫 `export(...)`
  （`collectExportNames`），**再** `loadProgramWithModules` 内联模块后收集符号。
  顺序不能反：`ModuleLoader::expandProgram()` 会把 `export` 语句整条丢弃。
- 六种格式由 `kFormats[]` 注册表驱动，`--format` 大小写不敏感，可流式输出到
  stdout 或 `--output`（目录或无扩展名时自动补 `symbolFormatExtension`）。
- 只读 AST，不做类型解析 —— 标注按原文打印。
- `--export` / `--extension` / `--spec-symbol` 三个开关语义照抄旧实现。
- **dot 图默认只画主文件**：模块内联会把整个标准库拉进来（一个 hello-world
  就有 90+ 符号），两张图都会不可读；模块内部符号用 `-s --ext <mod>` 查看。

### 3.5 顺带清掉（**已完成**）

已删除：

- `CMakeLists.txt` 的 `RCC_LEGACY_SOURCES` 整块（18 个 `.cpp`，约 8.6k 行）及文件本身
- `code/include/visitors/` 整个目录 —— 不只是四个 visitor，连 `rcc_base_visitor.h`
  这个 51 个纯虚函数的基类、以及 AST 里 47 处 `acceptVisitor` 声明/实现一并删除
  （它们只为已删除的 visitor 存在）
- `code/include/components/symbol/`、`code/include/components/ri/`、`code/include/builtin/`
- `code/declarations/builtin/**`、`code/declarations/components/**`
- `code/include/interfaces/` 整个目录 + `code/include/internal/`
  —— 这套 `IRCCObjectInterface` / `IRCCPosInterface` / `IRCCCompileInterface` /
  `IRCCSymbolInterface` C ABI 是给旧 DLL 扩展（`lowlevel.bindDllExt`）传对象用的，
  扩展机制随 S1 删除后已零调用点；其中 `const char* ToString()` 这类重载返回的是
  临时 `std::string` 的 `c_str()`，本身就是悬垂指针
- `code/declarations/analyzer/rcc_ast_dec.h`（其 `getNodeTypeName` 声明移入 `rcc_ast.h`，
  其余前置声明已无对象）

**验收结果**：

1. ✅ `mingw32-make -C build -j 10` 零错误
2. ✅ 六种格式在 `rich.rio` / `calls.rio` 上全部实测通过；`-exp`、`--ss`、
   `-b --ext ds`、`--fmt XML` 报错路径均符合预期
3. ✅ `test_p0`–`test_p11` 全绿（**147 例，0 失败**）
4. ✅ legacy 套件保持已知基线 15 通过 / 2 失败（`test_14_iter`、`test_15_forinrange`，
   属 G6，已延期到 P13，与本次删除无关）
5. ✅ A1–A7 七项审计全部有结论（见 `docs/p12_legacy_audit.md`）

## 4. P12-2：补语言缺口

按 §1.2 的 A 组逐条做，每条都先加回归用例再加实现。
§1.2 的 B 组（A4 / A5 / A6）与 §1.4 的 A7–A12 是**提案**，未获批不动。
实现全部集中在 `code/src/backend/rcc_codegen.cpp`（后端没有独立 sema pass）。

**本节分三类，执行时必须区分**：

| 类别 | 缺口 | 处置 |
|---|---|---|
| ✅ 可直接实现 | G1a / G3 / G4 / G5 / G7 / G9 / G10 / G11 / G12 | 按 §4.1 及各小节实现，每条配回归 |
| ⚠️ 待设计 | G1b（入口与运行参数）、G6（闭包写回） | **先提交提案获批再动手**，未获批只记录不实现（§4.2 / §4.3） |
| ❌ 已否决 | G2 / G8（A4 / A5 / A6） | 不实现 |

### 4.1 G1a 顶层 `var` 变成全局静态槽

- 顶层 `var` 现在被当作 `__rio_main` 的局部变量发射，函数体里自然看不到。
- 改法：`emitProgram` 先扫一遍顶层 `var`，为每个分配 `.bss` 槽（`_g_<name>`），
  登记进 `_globals`；`lookupVar` 未命中局部作用域时回退查 `_globals`；
  `emitAssignment` / `emitCompoundAssign` 同样回退。
- **初始化顺序**（与 §4.2 的入口设计强相关，必须一起定）：
  顶层 `var` 的初始化语句仍在 `__rio_main` 里按**源码顺序**执行，全局槽只是它的存储位置。
  因此全局变量在「用户代码第一句」之前就已初始化完毕 —— 但**在入口回调之前还是之后**，
  取决于 §4.2 最终采用哪种入口模型，见 §4.2.4 的顺序表。
- 副作用：`math.rio` 的 `var pi: float const = 3.141593` 从此可用。

### 4.2 G1b 程序入口与运行参数传递 —— **⚠️ 待设计，先出提案**

#### 4.2.1 现状核实（已完成，2026-11-16）

| 环节 | 实际情况 | 证据 |
|---|---|---|
| 进程入口 | 后端发射 `main`，只做三件事：建帧、`call __rio_main`、`core_proc_exit(0)` | `rcc_codegen.cpp:1350` `emitMainWrapper` |
| argc/argv | **`__rio_main` 不带参数**，`main` 也从不读 `%rcx`/`%rdx` | 同上；legacy 的 `test_10_main/test.ll` 里 `define i32 @main(i32 %0, ptr %1)` 收下 argc/argv 后直接 `call ptr @__rio_main()`，**argv 丢弃** |
| 用户 `fun main()` | **不会自动执行**，必须显式写 `main()` 才跑（legacy `test_10_main` 就是这样） | `emitMainWrapper` 的注释与实现 |
| `prog.entry(arg, *args, **kwargs)` | 编译器内建（`rcc_entry`），语义是：**若当前文件 == 程序入口文件**，则把 `entry(...)` 调用点上**除第一个实参外**的其余实参转发给回调并调用；否则不调用（有返回值时推 `null`） | `rcc_builtin_prog_functions.cpp:10-56` |
| 回调签名约束 | 回调必须**恰好两个参数**，且依次是 `PARAM_VAR_LEN_POSITIONAL`（`*args`）与 `PARAM_VAR_LEN_KEYWORD`（`**kwargs`），否则编译期报错 | 同上 L29-34 |
| 仓库内唯一调用点 | `release/Lib/builtin/math.rio:130` `prog.entry((*args, **kwargs) -> { io.sout(abs(false)) })` —— **零个**位置参数、**零个**关键字参数 | 全仓库 grep |

#### 4.2.2 定性：**设计意图明确，实现从未落地**（用户 2026-11-16 确认）

你的原话：**「原始设计是这样的但是并未实现」**。

所以 `prog.entry` 的定性修正为：

| 维度 | 结论 |
|---|---|
| 设计意图 | ✅ **明确**：进程运行参数（argv）应作为 `*args` / `**kwargs` 传给第一个 `arg` 回调 |
| 实现状态 | ❌ **从未实现**。当前 `rcc_entry` 转发的是**调用点写死的实参**，与 argv 无关 |
| 缺陷性质 | 不是「原理上做不到」，而是**实现缺失** |
| §5.5-A 的旧结论 | ⚠️ **必须修正**：原写「`prog.entry` 原理上无法实现」，实际是「设计意图存在但未实现」—— 编译期路径比较在新架构里完全做得到（`-build` 时已知主文件路径），argv 传递也不是不可能，只是没做 |

`rcc_entry` 的当前实现只做到「编译期判断当前文件是不是入口文件」这一半，
argv → 回调这一半**从来没有代码**：

```cpp
// rcc_builtin_prog_functions.cpp:35-43（现状）
const std::vector<std::string> &args = {callInfos.processedArgs.begin() + 1,
                                        callInfos.processedArgs.end()};   // ← 调用点实参
if (utils::checkPathEqual(visitor.getProgramEntryFilePath(),
                          visitor.getCurrentProcessingFilePath())) {
    return ri::CALL(callInfos.processedArgs[0], args).toRACode();          // ← 转发这些实参
}
```

`release/Lib/builtin/math.rio:130` 是全仓库唯一调用点，写的正是
`prog.entry((*args, **kwargs) -> { io.sout(abs(false)) })` —— 零实参，
所以「转发调用点实参」和「转发 argv」在现有代码上**看起来**都能跑，缺陷被掩盖了。

#### 4.2.3 处置决定（用户 2026-11-16 已裁决：**P12 不动，登记待设计**）

我在本轮给出了三种方案的对比与推荐，用户选择「**P12 不动，登记待设计**」。
以下保留分析供 P13 决策，**P12 不实施任何一条**。

**三个方案对比**：

| 方案 | 做法 | 优点 | 缺点 |
|---|---|---|---|
| A. 保留原样补齐 | 按原设计实现 argv → `*args`/`**kwargs` | API 不变、向后兼容 | 背上 D1–D4 全部缺陷；强制 `**kwargs` 是负担；需新增 `--key=value` 语法（需审批） |
| B. 替换为显式接口 | 删 `prog.entry`，改 `prog.args(): list`、`prog.argCount(): int`；`main` 仍由用户显式调用 | 无 `**kwargs` 伪概念；按需取用；符合内存模型「无隐式行为」原则 | 破坏公开 API（需审批）；丢失回调式写法 |
| C. 两者并存 | 保留 `prog.entry`，另加 `prog.args()` | 兼容 | 两套机制做同一件事，违反「统一」原则 |

**原始设计的四个结构性缺陷**（无论选哪个方案都要面对）：

| # | 缺陷 | 说明 |
|---|---|---|
| D1 | `**kwargs` 无来源 | argv 天然没有键值概念。造 kwargs 必须约定 `--key=value` 语法（**新增语言设计**）。而原实现强制回调签名必须是 `(*args, **kwargs)`，等于强迫每个程序接受一个可能永远为空的 `**kwargs` |
| D2 | 入口唯一但回调可变 | 一个程序只能有一个入口，但 `prog.entry` 可调用任意次、任意位置。原实现靠「当前文件==入口文件」的**编译期**判断；模块内联（P8）后该判据在新架构里无运行时对应物 |
| D3 | 字符串传递强制分配 | argv 每个参数都要变成 Rio `str`（`{i64 len}+bytes`），启动时就要 `core_alloc` 一串堆对象。对「只想知道有没有 `--help`」的程序也是全量开销 |
| D4 | 错误处理缺失 | 回调签名不符是编译期错误，但 argv 内容非法（编码/超长/缺值）在运行期，内存模型规范 §9 没有对应机制 |

**我的推荐（供 P13 参考）**：方案 B。理由是内存模型规范自身的原则：

> §1 目标 2：**没有隐式的运行时检查，没有隐式的终止。所有动态行为都必须在源码里显式出现。**
> §9：**语言里没有异常。可恢复失败用 `Result` / `Maybe` 显式表达。**

`prog.entry` 的回调模型是「编译器在幕后决定何时调你的函数」——正是规范要消灭的**隐式控制流**；
而 `prog.args()` 是显式取值，契合规范。

**但方案 B 有一个规范未覆盖的空白**：argv → `list` 需要分配，而规范的「出生地」机制要求
每次创建都显式写 `in`。**启动时的 argv 属于谁？** 这在 `memory-model-spec-full.md §12`
未决问题里**没有列出**，是一个新的待设计点。这也是「P12 不动」的技术理由之一。

#### 4.2.4 待提案的设计问题（P13 用，不自行假设）

| # | 待定问题 | 备注 |
|---|---|---|
| Q1 | argv 如何变成 Rio 的 `str` | 需 `rt` 提供「C 字符串 → Rio str」构造器 |
| Q2 | 「关键字参数」从哪来 | 约定 `--key=value` → kwargs，其余 → `*args`；**这是新语法，须批准** |
| Q3 | 回调签名是否仍强制 `(*args, **kwargs)` | 若走方案 B 则此问题消失 |
| Q4 | `prog.entry` 公开 API 是否变 | 用户已定「P12 不改」，P13 再议 |
| Q5 | 没有运行参数时 | 空 list / 空 dict，回调照常调用一次 |
| Q6 | 参数含空格/引号/特殊字符 | 交给 Windows 命令行规则，Rio 侧不做二次解析 |
| Q7 | 解析失败 | 原样当位置参数，不 panic（最不意外） |
| Q8 | 是否需要 `main(argc, argv)` | **不需要** —— 约束明确「不引入 `main` 作为 RCC 程序入口」 |
| **Q9** | **启动期 argv 的出生地归属** | **规范空白**，方案 B 的前置；需在内存模型里新增一节 |

#### 4.2.4 入口 / 全局初始化 / 用户代码的执行顺序（待 Q 系列定案后填实）

当前实现的顺序是确定的：

```
进程启动 → main（C 运行时）
  → __rio_main
      → 顶层 var 初始化（按源码顺序）        ← G1a 修好后仍在这里
      → 顶层语句（含用户显式写的 main() 调用）
      → prog.entry(...) 若当前文件是入口文件，则在此处调回调
  → core_proc_exit(0)
```

**冲突点**：若走「可能二」（argv 进回调），那么 argv 必须在**进程启动时**就从
`main` 的 `%rcx`/`%rdx` 取出并保存到全局槽，**早于任何用户代码**；
而顶层 `var` 初始化可能访问这些槽 —— 必须保证「argv 槽先于顶层 var 初始化」。
这条顺序约束要写进实现，否则会出现「全局初始化读到未初始化的 argv」。

#### 4.2.5 测试清单（对应你列的 7 项）

先建 `test_scripts/test_p12_entry.ps1`，用例目录 `build/p12entry/`。
**在 Q 系列定案前，只写「不依赖 argv」的 3 项**，其余 4 项等设计确认后补：

| # | 用例 | 现在能否写 | 说明 |
|---|---|---|---|
| T1 | 无运行参数时的入口行为 | ✅ 能 | `prog.entry` 零实参 → 回调照常被调用一次 |
| T2 | 多个位置参数的传递 | ⚠️ 视 Q2 | 若走可能一，测「调用点传 3 个实参」；若可能二，测 argv |
| T3 | 多个关键字参数的传递 | ⚠️ 视 Q2 | 同上 |
| T4 | 位置 + 关键字混合 | ⚠️ 视 Q2 | 同上 |
| T5 | 参数含空格 / 特殊字符 | ⚠️ 视 Q1 | 仅 argv 模型下有意义（Windows 命令行引号规则） |
| T6 | 传递失败 / 非法参数 | ⚠️ 视 Q6 | panic 还是降级，取决于定案 |
| T7 | 全局变量初始化与入口回调的顺序 | ✅ 能 | 全局 `var g = 1`，回调里读 `g` 应为 `1` |

### 4.3 G6 闭包捕获写回 —— **⚠️ 待设计，先对齐内存模型**

#### 4.3.1 内存模型的真实状态（2026-11-16 更新）

**上一轮的结论已过时，此处更正。**

`docs/memory-model-spec-full.md` 现在存在（805 行，v0.3 合订版），是一份**完整的内存管理模型规范**：

| 维度 | 内容 |
|---|---|
| 规模 | 12 章 + 3 附录，15 节 |
| 核心机制 | Body（壳子）/ Content（内容）二分；**出生地** `in local` / `in result`；能力类型 `Ref<T>` / `Mut<T>` / `Take<T>`；`Pool` 稳定地址容器；`Link` 成对引用；控制流状态窄化；结构化并发 |
| 明确非目标（§1） | **垃圾回收、引用计数、运行时版本号** |
| 明确无异常（§9） | 没有展开；不可恢复失败直接 `abort`，不运行销毁 |
| 状态 | 标题即写「**设计草案**」，3 处【待确认】（§9 无展开、§12 未决问题 14 条） |

**但这份规范不能直接作为实现依据**，有两个必须先解决的问题：

**冲突 A：规范与架构计划互斥**

| 来源 | 对引用计数的态度 |
|---|---|
| `memory-model-spec-full.md` §1 非目标 | ❌ **明确排除**「垃圾回收、引用计数」 |
| `architecture_plan.md §8.1` | ✅ 「**建议：先 B（引用计数，非原子）**」，容器 header 已预留 `rc` 字段（当前恒为 0） |

两者**不能同时成立**。用户 2026-11-16 已裁决：**以规范为准，改架构计划**。
因此 `architecture_plan.md §8.1` 需改写，`ListHeader`/`DictHeader` 的 `rc` 字段需标注废弃
（这影响布局，是 ABI 级决定，须在 P13 内存模型落地时一并处理）。

**冲突 B：规范是伪代码，映射到 Rio 是设计工作**

用户明确说明：「这份设计是在脱离 Rio 的情况下设计出来的，里面的示例代码都是伪代码，
需要根据 Rio 的语法和语言特色进行修改。」核实语法差异：

| 规范（伪代码） | Rio 实际语法 | 差距性质 |
|---|---|---|
| `let x = ...` | `var x = ...` | 直接映射 |
| `fn f(...)` | `fun f(...): T` | 需补返回类型 |
| `struct X { }` | `class X { }` + `ctor` | **语义不同**，需重设计 |
| `Ref<T>` / `Mut<T>` / `Take<T>` | 无；泛型写作 `class Box[T]` / `fun f[T]` | **能力类型系统需全新设计** |
| `in local` / `in result` | 无 | **出生地语法需全新设计** |
| `Pool` / `Link` | 无 | 需设计为标准库或内建 |
| `fn main()` 是入口 | `fun main()` **不自动执行**（须显式 `main()`） | **入口语义冲突**，与 §4.2 相关 |

`Ref<T>`/`Mut<T>`/`Take<T>` 是一整套**能力类型系统**，`in local` 是**新语法**，
`Pool`/`Link` 是**新容器原语**。把它们落地等于给 Rio 加一层新的类型系统与所有权模型——
这不是「改改示例代码」，而是**重大语言设计工作**，每项都需逐条审批。

**结论**：内存模型已有完整草案，但**尚未完成 Rio 映射**，且与架构计划有未落地的裁决。
按 §11 原则 6（未完成设计必须标注），**G6 仍不实施**，但延期的理由更新为：
「内存模型规范已存在（v0.3），但需先完成 Rio 语法映射、落实『以规范为准』的架构计划修订，
才能作为 G6 的实现依据。」

#### 4.3.2 G6 与内存模型的耦合点（哪些必须先定）

按规范的实际机制重新梳理（上一轮基于「引用计数」的耦合分析已失效）：

| 耦合点 | 规范里的相关规则 | 对 G6 的影响 |
|---|---|---|
| 捕获变量的存储位置 | §2.2 出生地决定销毁点；§2.1 Body 固定位置永不搬家 | 闭包捕获的变量若要写回，必须让「变量」成为**可被多个闭包共享的 Body**，而不是栈槽 |
| 多闭包共享同一变量 | §3.1 引用指向 Body（槽位），可跨 Full→Empty→Full 存活 | 共享 = 多个闭包持有同一 Body 的引用；写回 = 通过 `Mut<T>` 修改 |
| 闭包自身的生命周期 | §8.8 循环中 `in local` 每轮新建销毁；逃逸须显式指定外层出生地 | 闭包 env 也要有出生地；当前实现**从不释放** env（`core_alloc` 后不 free），与规范冲突 |
| 变量写回与内容转移 | §3.5 `move_into` 目标必须确定 Empty；§4.1 原地改值 vs 替换/重分配 | 写回是「原地改值」还是「替换」决定是否与活跃子引用冲突 |
| **逃逸闭包** | §3.3 `Take` **不能被逃逸的闭包捕获**；§8.3 闭包不带走窄化事实 | 若闭包捕获 `Take` 则非法；捕获 `Ref` 按 `?` 处理 |

#### 4.3.3 处置决定

- **P12 不实施 G6**（用户第 4 条已同意延期）。不写 boxed cell，不改 `allocateSlot` /
  `emitAssignment` / 9 处 `stackOffset`。
- **不引入任何与规范冲突的临时方案**（明确排除：给 cell 加裸指针、给 env 加手工 `rc`——
  后者还与规范的「非目标：引用计数」直接冲突）。
- **延期记录**：见 §10.2，写清原因、依赖、后续计划。
- **测试预期不得修改来掩盖问题**：`test_14_iter` / `test_15_forinrange` 的现有
  `expected.txt` 与 `test.rio` **原样保留不动**，只标注「当前已知失败，原因 G6 延期」。
  不允许把 `expected.txt` 改成「不写回」的结果来制造通过。
- **标准库不得依赖写回**：`ds.iter` / `ds.forInRange` 在 P12 里实现为
  「返回累计值」的纯函数式契约（这本来就是新写，不是改旧契约），
  并在文档里写明「回调写回需等 G6」。
- **架构计划需修订**（用户已裁决「以规范为准」）：`architecture_plan.md §8.1` 的三方案表
  改为指向 `memory-model-spec-full.md`，并注明 `rc` 字段已废弃。**P12 内完成此项文档修订**，
  因为它是文档工作，不涉及代码。

#### 4.3.4 保留的验证目标（等内存模型完成 Rio 映射后启用）

以下四条作为 G6 的**验收定义**先写进测试脚本骨架（`test_scripts/test_p13_closure.ps1`，
P12 阶段只建骨架不跑），实现时直接填：

| # | 目标 |
|---|---|
| C1 | 闭包能够修改外层变量 |
| C2 | 多个闭包捕获同一个变量时能正确共享状态 |
| C3 | 嵌套闭包的捕获行为正确 |
| C4 | 闭包生命周期与变量生命周期符合最终内存模型 |

### G2 索引赋值 `a[i] = v` / `d[k] = v` —— **✅ 原仓库确实支持（我此前核实错误，已纠正）**

#### G2.1 结论纠正（2026-11-16，用户实测反证）

**我此前的结论是错的。** 我在克隆版 `D:\DeepseekHarnessProjects\RCC`（v1.21.0）上核实，
得出「原仓库从未支持元素赋值」。用户用**真实原仓库**（`D:\ClionProjects\Rio\Rio\RCC\RCC.exe`，
**v1.21.2**，Build Time Dec 24 2025）实测反证：

```rio
fun main(): void {
    var li: list = [1, 2, 3]
    li[0] = 0        // ✅ 生效
    li[1] += 1       // ✅ 生效
    li += [4]        // ✅ 生效
}
```

运行输出 `[0, 2, 3]` / `[0, 3, 3]` / `[0, 3, 3, 4]` —— **元素赋值、复合赋值全部可用**。
所以用户的判断「可能是 LLVM 改造的时候导致不支持了」是**正确的**：
这是新后端（P0–P11 重写）的**功能回归**，不是从未有过的能力。

#### G2.2 我错在哪（教训，必须记下）

| 我的核实 | 为什么错 |
|---|---|
| 只在 `D:\DeepseekHarnessProjects\RCC`（v1.21.0 克隆版）上查 | **没有对照真实原仓库** `D:\ClionProjects\Rio\Rio\RCC`（v1.21.2）与 `_rcc-merge-v1212` 源码树 |
| 看到 `visitIndexExpressionNode` 只有 `ITER_GET` 就断定「只读」 | **`ITER_GET` 在 RA 虚拟机里是「引用绑定」语义**，不是拷贝。看指令名想当然 |
| 看到 `visitAssignmentNode` 只认 `Identifier`/字段 就断定不支持 | 没意识到 `li[0] = 0` 走的是「先 `ITER_GET` 绑定临时变量，再 `COPY` 到它」的路径 |
| 搜 `ITER_SET` 零命中就断定无写能力 | 根本不存在独立的「写」指令，写是**通过共享指针间接完成**的 |

**方法论教训**：核实「原仓库某能力是否存在」时，必须用**用户指定的真实仓库版本**，
并且**以实际运行结果为准**，不能只看指令名或 visitor 分支。

#### G2.3 真实机制（v1.21.2 源码 + VM 实现）

编译器侧（`_rcc-merge-v1212\code\src\visitors\rcc_compile_visitor.cpp`）：
`li[0] = 0` 被解析成 `AssignmentNode(IndexExpressionNode, 0)`，
`visitAssignmentNode`（:4176）先 `identNode->acceptVisitor(*this)` →
`visitIndexExpressionNode`（:3948）发出 `ITER_GET`，把结果绑到临时变量，
然后 `visitAssignmentNode` 对这个临时变量发 `COPY`/`PUT`。

实际生成的 RA（实测）：

```
ALLOT: li_v1092
ITER_APND: 1, 2, 3, t388_v1093
PUT: t388_v1093, li_v1092
ITER_GET: li_v1092, 0, t389_v1094   ← 取出，但 t389 与 li[0] 是同一对象
COPY: 99, t389_v1094                ← 改 t389 就是改 li[0]
```

VM 侧（`D:\ClionProjects\RioKit\code\rvm\src\rvm_ris.cpp:3209` `ri_iter_get`）：

```cpp
data = iter->getDataAt(index->getValue());          // 返回 shared_ptr<RVM_Data>
data_space_pool.updateDataByArgOrName(targetArg, targetArg.getValue(), data);
```

而 `getDataAt` 的签名（`ra_core.h:369`）是
`virtual std::shared_ptr<RVM_Data> getDataAt(int index) = 0;`
—— **返回的是指向容器内元素的共享指针，不是副本**。

**所以索引赋值的语义基础是「元素是引用，不是值」。** 这与新架构的
`ListHeader` 布局（数据区直接存值，`elem_size` 字节）**根本不同**：

| | 原仓库（RA VM） | 新架构（RCC x86-64） |
|---|---|---|
| 容器元素 | `shared_ptr<RVM_Data>` 装箱对象 | 数据区内联的**裸值**（8 字节或 `elem_size`） |
| `a[i]` 读出 | 得到指向元素的**引用** | 得到值的**副本**（`mov 32+i*8(%rax), %rax`） |
| `a[i] = v` | `COPY` 到引用 → 自动写回 | **没有引用可写**，必须显式计算地址并 `mov` |

**这就是回归的真正原因**：新架构去掉了装箱，元素变成内联值，
「改临时变量就等于改容器」这条隐含通路随之消失。**要恢复它，必须显式实现索引赋值。**

#### G2.4 若要在新架构恢复（需要你批准）

失败点只有一处（`code/src/backend/rcc_codegen.cpp:1988-1992`）：

```cpp
const auto id = std::dynamic_pointer_cast<ast::IdentifierNode>(lhs);
if (!id) { fail("Only simple variable assignment is supported: ..."); return; }
```

需要新增 `IndexExpressionNode` 左值分支，生成「计算元素地址 + 存值」：

```
# a[i] = v  (list, elem_size 8)
<求 a>            → %rax = 数据区基址 + 32
<求 i>            → %rcx
<求 v>            → %rdx
mov %rdx, (%rax, %rcx, 8)
```

对 `dict` 需要先查键、不存在则插入（涉及扩容）。

**语义定案（2026-11-16 用户裁决「支持读取不支持修改」+ 我按原仓库实测补齐）**：

| # | 问题 | 定案 |
|---|---|---|
| Q1 | 越界怎么办 | 静态可算的报错；动态越界返回 `0`（与现有 list 读分支 `emitIndexExpression` L2714 的 `movImm(0,%rax)` 一致）。<br>**注**：原仓库 VM 抛 `RangeError`，但新架构当前 list 读已定为返回 0，保持一致 |
| Q2 | `str` 是否支持 `s[i] = c` | ❌ **不支持，编译期报错**。原仓库「支持」实为**静默失效的 bug**（`String::getDataAt` 返回副本），详见 §G10.1。<br>`s[i]` **读取**则支持，见 G10 |
| Q3 | `dict` 键不存在时 `d[k] = v` | **新增键值对**（原仓库 `Dict` 有 `setDataAt`，行为是新增） |
| Q4 | 元素类型必须匹配 `elem_size` 吗 | 必须，走 `checkAssignable` |
| Q5 | 支持 `a[i] += v` 吗 | ✅ **支持**，原仓库实测可用（`li[1] += 1` → `[0, 3, 3]`） |
| Q6 | `a += [4]`（列表拼接赋值） | ✅ **支持**，原仓库实测可用；归入 G4（`list + list`），不是索引赋值 |
| Q7 | 与内存模型的关系 | 元素赋值是**原地改值**（`memory-model-spec §4.1`），与活跃子引用不冲突 |

#### G2.5 三条落地口径汇总（P12-2 实现清单）

| 语法 | 状态 | 依据 |
|---|---|---|
| `list[i] = v` | ✅ **实现**（真回归，必须恢复） | 原仓库 v1.21.2 实测可用 |
| `dict[k] = v` | ✅ **实现**（键不存在则新增） | 原仓库 v1.21.2 实测可用 |
| `s[i] = c` | ❌ **编译期报错**（`str is immutable`） | 原仓库是静默失效 bug，不得复制（§G10.1） |
| `s[i]` 读取 | ✅ **实现**（G10） | 原仓库实测输出 `e` |
| `a[i] += v` | ✅ **实现**（复合赋值） | 原仓库实测可用 |
| `a += [v]` | ✅ **实现**（G4 列表拼接） | 原仓库实测可用 |

#### G2.6 标准库怎么办

若索引赋值恢复，标准库可直接用 `a[i] = v`；在它落地前，
内部写元素仍用 `core_mem_set(p + offset, byte, n)`（探针 `w1`/`hh3`/`qq1` 已验证）。
**不新增** `listSet`/`dictSet` 公开函数（用户第四轮决策）。

### G3 / G4 字符串与列表拼接

- `emitBinary` 里 `TOKEN_PLUS` 目前无条件走整数加法，这是 `str + str` 打出
  指针值的根因（两侧类型明明是 `str`，结果按 int 解释）。
- 改法：`+` 先看两侧静态类型族：
  - 两侧都是 `str` → `call rt_str_concat`（新函数，裸名，写在 `lib/rt/rt.rio`）
  - 两侧都是 `list` → `call rt_list_concat`（新函数）
  - 否则维持整数加法（含 int/float 混合，已由 `emitFloatBinary` 处理）
- `rt_str_concat(a, b)`：读两个长度头，`core_alloc(8 + len_a + len_b, 8)`，
  写长度头 + 两段字节，返回新 str。
- `rt_list_concat(a, b)`：读两个 ListHeader 的 `len` / `elem_size`，
  分配 `32 + (len_a+len_b)*8`，复制两段元素，返回新 list。
- 这两个函数是**裸名**（唯一名字），与 P11 的「单定义保裸名」规则一致，
  `rt.o` 预编译进链接，不需要改 ABI。

### G5 方法重载

- 现状：`registerClassBody` 一个方法名只留一份定义，`A.m(x: int)` 与
  `A.m(x: str)` 后者覆盖前者。
- 改法：把 P11 在**全局函数**上的做法平移到方法上 ——
  - 方法符号用 `_R<len>Class.m<形参编码>`（带类名，避免两个类的方法互相撞）
  - `registerOverloadSignatures` 的遍历范围从顶层 `fun` 扩到 `class` 的
    `fun`（构造器不参与重载）
  - `emitMethodCall` 先查「这个类 + 这个方法名」是不是重载集，是则决议
  - 单定义的方法**保持裸名**（`A.m` 不变），与全局函数同一判据
- 注意：P11 踩过的坑要复用 —— 同签名重复定义会让 `_signatures` 丢数据
  （`emplace` 只留第一份），方法表也要用「按 mangled 名索引」而不是按名字。

### G6 闭包捕获写回 —— **⚠️ 已移至 §4.3（待设计，P12 不实施）**

> 本小节原有的 boxed cell 方案**降级为候选**，不再作为 P12 的实现依据。
> 原因：内存模型（`architecture_plan.md §8.1`）仍是待定问题，boxed cell 与
> A/B/C 三种方案的耦合方式不同，现在写会与未来定案冲突。
> **完整分析见 §4.3；延期登记见 §10.2。**

### G7 `{ ret }` 解析坑

- `fun f(): void { ret }` 在同一行时 parser 报 `TOKEN_RBRACE` 找不到前缀构造；
  把 `}` 换到下一行就好了。
- 这是 parser 的 prefix builder 把 `ret` 当成「必须跟一个表达式」处理导致的。
- 修法：`ret` 的后缀判断里，`}` / `TOKEN_STREAM_END` 也视为「无返回值」。
- 影响面极小但收益大：手写 Rio 代码几乎一定会踩。

### G8 `ptr` 索引 —— **提案 A4，已否决**

> ❌ 2026-11-16 用户否决。原仓库 `.rio` 代码里也没有任何 `p[i]` 用法，
> `core.rio`/`rt.rio` 全靠 `p + 8` 裸指针算术。**不实现。**
> 需要读写裸内存时用 `core_mem_set(p + off, v, n)` / `core_mem_copy`，
> 读单个字节用 `core_str_len(p + off) % 256`（已实测可行）。

- `emitIndexExpression` 只处理 `TK_LIST` / `TK_DICT`，其余一律报错 —— 保持现状。

### G9 `str` 相等比较按内容 —— 必补

**现象**（探针 `ff3` / `ff6`）：

```
var a = "ab"
var c = "ab"
if a == c { sout("eq") } else { sout("ne") }   // 输出 ne —— 错
```

`emitBinary` 的 `TOKEN_EQUAL` 走 `cmpq` + `sete`，比的是**两个指针**。
字符串字面量即使内容相同，地址也不同，所以永远 `ne`。

**原仓库依据**：`str.rio:105` `if del == ""`、`datatype.rio:92` `typeOfArg1 == arg2[i]`、
`ds.rio` 里大量 `if xxx == "..."` —— 全部依赖内容比较。legacy 的 `ITER_GET` 与字典查找
也按内容比（新后端 `emitDictLookup` 已用 `core_mem_eq`，只有 `==` 运算符漏了）。

**修法**：`emitBinary` 在 `TOKEN_EQUAL`/`TOKEN_NOT_EQUAL` 处，若两侧推断类型
同为 `TK_STR`，改为调用 `rt_str_eq(a, b): bool`（`rt.rio` 新增，用 `core_str_len` + `core_mem_eq`）。
探针 `gg1` 已验证该实现正确：`("ab","ab")=true`、`("ab","ac")=false`、`("ab","abc")=false`。

**风险**：`rt.rio` 自举期不能依赖这个（`rt.o` 在构建时编译，若 `rt_str_eq` 本身用 `==` 会递归）。
`rt_str_eq` 内部只用 `core_*`，不碰 `==`，安全。

### G10 `str` 下标 `s[i]` —— 必补（**只读不写**，2026-11-16 用户定案）

**现象**（探针 `hh5` / `x3`）：`var s = "abc"  sout(s[0])` → `Index access on a non-container value`。

**原仓库依据**：`visitIndexExpressionNode` 明确把 `str` 归入 iterable
（`targetTypeLabel->isIterable()` → 发 `ITER_GET`）；`str.rio:24` `ret arg[0]`、
`str.rio:52` `result += toStr(content[startIndex])` 都在用 `s[i]`。
实测原仓库 v1.21.2：`var s: str = "hello"` + `io.sout(s[1])` → 输出 `e`。✅

**修法（读）**：`emitIndexExpression` 增加 `TK_STR` 分支，返回 `char`（即 `int` 族，值域 0–255）：

```
mov  (%rsi), %rax           # StrHeader.len（第一个 8 字节）
cmp  %rax, %rcx
jae  out_of_range
movzbl 8(%rsi,%rcx), %eax   # 基址 +8（len 占 8 字节）
```

`inferIndexResultType` 对应返回 `getCharType()`。越界返回 `0`
（与现有 list 分支 `emitIndexExpression` L2714 的 `movImm(0,%rax)` 一致）。

**写入：明确不支持，必须编译期报错**（2026-11-16 用户定案「支持读取不支持修改」）

#### G10.1 为什么 str 写入必须禁止（实测发现的原仓库 bug）

原仓库 `s[1] = "X"` **能编译通过，但静默失效**——实测输出仍是 `hello` 而非 `hXllo`。
根因在 RA 虚拟机（`RioKit` 的 `ra_core.h` 附近）：

```cpp
// List::getDataAt —— 返回容器内元素本身
return dataList[index];                       // ✅ shared_ptr 共享，改它就是改容器

// String::getDataAt —— 新建副本
return std::make_shared<Char>(value[index]);  // ❌ 副本，改它没有任何效果
```

生成的 RA 是 `ITER_GET: s, 1, t388` + `COPY: "X", t388`——改的是临时副本，随即丢弃。
**所以原仓库的 str 写入不是特性，是 bug。** 而 list 索引赋值能用，正是因为它
`getDataAt` 返回的是共享指针（详见 §G2.3）。

> 补充：VM 里 `String::setDataAt` 是存在且正确的（写 `value[index]`），
> 只是 RA 指令集没有「写索引」指令去调用它。将来若要支持可变字符串有技术基础，
> 但那需要重新设计 str 的不可变语义，不属于 P12 范围。

#### G10.2 三条落地口径

| 项 | 处置 |
|---|---|
| `s[i]` 读取 | ✅ **P12-2 实现**（本条 G10），返回 `char`，越界返回 `0` |
| `s[i] = c` 写入 | ❌ **编译期报错**，如 `Cannot assign to string element: str is immutable`。<br>**不得复制原仓库的静默失效行为** |
| 理由 | str 是**不可变字节序列**：`var b = a` 与 `a` 共享同一 `StrHeader`，允许写会别名污染所有持有者；改长度更需重分配，而持有者已缓存指针 |

**注意 UTF-8**：`s[i]` 按**字节**索引，多字节字符会被拆开——这与原仓库行为一致，
不是缺陷，但需在 `rio_doc.md` 里写明。

**注意**：这是**新语义**（`ptr` 索引 A4 被否，但 `str` 索引有原仓库依据，属缺口不属提案）。
区别在于：`str` 在原仓库是 iterable（有 `ITER_GET` 语义），`ptr` 在原仓库根本不存在这个类型。

### G11 类字段初始化器 —— 必补

**现象**（探针 `hh1` / `ii2`）：`class A { var x: int = 42 }` → `A().x` 读出来是 `0`。

**原仓库依据**：`datatype.rio:50` `var TP_INT: str static = type(0)`、
`ds.rio:492` `compareHandler: funi[...] private = null`、`ds.rio:493` 同 —— 都是字段初始化器。

**修法**：`registerClassBody` 在收集字段时，把 `def->getValueNode()` 一并存进
`ClassInfo::ownFieldInits`；`emitConstructor`（或对象创建处）在 ctor 之前按声明顺序
写入这些初值，然后才执行用户 ctor。

**注意**：`static` 字段（`C.X`）目前整体不支持（`Cannot determine the object type of attribute access`），
按 §1.3 用顶层 `var`（G1）替代。G11 只处理**实例字段**的初始化器。

### G12 `ptr` 返回值标注 —— 必补

**现象**（探针 `oo2` / `qq3` / `rr2`）：

```
fun cp(arg: list): ptr {
  var p = core_alloc(32, 8)     // core_alloc 推断为 int（无签名信息）
  ret p                          // cannot assign a value of type int to a target of type ptr
}
```

**根因**：`core_*` 函数没有类型签名（`core.rio` 里的 `extern fun` 不参与新后端的类型表），
`emitCallExpression` 对未知函数一律推断返回 `int`。而 `ptr` 是**新架构自己引入**的类型
（原仓库 `builtInTypes` 里没有 `ptr`），`sameValueFamily` 没把 `ptr` 和 `int` 归为一族。

**修法**（两选一，取简单者）：
1. **`sameValueFamily` 放行 `ptr` ↔ `int`** —— `ptr` 本质就是 64 位整数，这样 `ret p` 合法。
   风险：失去「把整数误当指针」的静态检查。但新架构本来就没有 `ptr` 的强类型传统。
2. 给 `core.rio` 的 `extern fun` 建立签名表，让 `core_alloc` 的返回类型已知为 `ptr`。
   更正确，但要改 `emitCallExpression` 的符号解析路径。

**推荐 1**：改动最小，且与「数值族互通」的既有原则一致（P9 已确立）。

**探针验证**：`oo3`（在 `main` 里直接 `core_alloc` 后 `core_mem_copy`）已通过；
`rr1`/`rr2`（试图用 `var r: list = p` 桥接）失败，说明**必须**走方案 1 或 2，
没有纯代码层的绕法。

## 5. P12-3：重写标准库

### 5.1 为什么必须重写，而不是修

`release/Lib/builtin/*.rio`（9 个文件、1.9k 行）是 LLVM/RA 时代的产物，它们的
实现方式依赖 P12 明确不做的东西：

- `datatype.rio`：`type` / `setType` / `checkType` / `isIter` + **静态类字段**
  `DT.TP_LIST`，直接用 `dt.setType(arg, "series")` 做运行时类型改标
- `ds.rio`：`fun size(arg): int { encapsulated }`（靠内置函数注册表提供实现），
  `iter` / `forInRange` 的契约是「回调里改外层变量」（依赖 G6）
- `io.rio` / `lowlevel.rio` / `prog.rio`：同样靠 `encapsulated` 指向内置函数

这些内置函数注册表（`rcc_builtin_*_functions.cpp`）在 P12-1 里会被删掉，
所以旧库不是「有问题」，而是**整体失去实现**。重写是唯一出路。

### 5.2 新标准库的目录与安装

```
lib/std/            ← 源（进仓库，唯一真源）
  io.rio  ds.rio  str.rio  math.rio  prog.rio  ops.rio  lowlevel.rio
release/Lib/builtin/  ← CMake 构建期从 lib/std 复制（.gitignore）
```

- 模块解析路径**不改**（`RCC_BUILTIN_LIB_DIR = ./Lib/builtin/`），零编译期风险。
- `release/Lib/builtin/*.rio` 从 git 里删掉（`git rm`），改由 CMake 复制，
  与 `lib/rt/rt.rio → release/lib/rt/rt.o` 同一个思路。
- 删除 `datatype.rio`、`_pure_builtin.rio`；保留 `ds io math str prog ops lowlevel`
  七个模块名，用户代码里的 `import ds` 不受影响。

### 5.3 关键实现技巧：`size` 不需要任何魔法

三种容器的长度字段**都在偏移 0**：

| 类型 | 布局 | 长度位置 |
|---|---|---|
| `str` | `{i64 len} + bytes` | `+0` |
| `list` | `ListHeader {i64 len, cap, rc, elem_size} + data@+32` | `+0` |
| `dict` | `DictHeader {i64 len, cap, rc, keys, vals}` | `+0` |

所以 `ds.size` 的实现只需要读偏移 0，**不需要内置函数注册表**：

```rio
// 签名照原仓库：fun size(arg): int
fun size(arg): int {
    ret core_str_len(arg)
}
```

实测 `size([1,2,3])` = 3、`size("abcd")` = 4、`size({"a":1,"b":2})` = 2。
一行搞定，不需要内置函数注册表，也不需要重载（`ptr` 家族全互通）。

标准库可以**直接调 `core_*`**（它们在链接期由 `core_win64.o` 提供），
但用户代码不应该这么做 —— 这一点写进文档。

### 5.4 模块清单：**以原仓库 API 面为基线，不为兼容而保留无意义项**

> **基调修正（2026-11-16 用户评审第 4 条）**：
> 兼容性**不是 P12 的最高目标**，正确、统一、可维护的新架构才是。
> 因此：
> - **默认**以原仓库 API 面为基线（因为那是「已批准的设计」，不是历史包袱）
> - 但**不为兼容而保留**：原理上无法实现的、与静态架构冲突的，直接删并记录
> - **不为无法实现的旧 API 制造替代实现**（写空壳/恒等函数都是反模式）
> - 涉及语言设计或公开 API 的删改，**仍然必须审批**（这条不变）

| 模块 | 原仓库真实 API（基线） | 处置 |
|---|---|---|
| `io` | `sout(*args, end="\n")`、`print(*args, end="\n", sep=" ")`、`sin(arg="")`、`input(prompt="")` | 全部实现（`sout` 走编译器特化） |
| `ds` | `size`、`copy`、`unpack`、`isEmpty`、`top`、`repStr`、`repList`、`repeat`、`join`、`joins`、`listAppend`、`listRemove`、`dictRemove`、`iterRemove`、`dictKeys`、`dictValues`、`iterContains`、`sort`、`forInRange`、`iter` | 17 个实现 + 3 个删（`unpack`/`iterContains`/`iterRemove`，见 S4） |
| `str` | `toStr`、`toChar`、`subStr`、`find`、`split`、`chunk`、`replace`、`toLowerCase`、`toUpperCase`、`charToInt`、`intToChar`、`trim` | 12 个全部实现 |
| `math` | `pi`、`e`（顶层 `var`，依赖 G1a）、`pow`、`max`、`min`、`opp`、`abs` | 7 个全部实现 |
| `prog` | `entry`、`breakpoint` | 2 个都实现；`entry` 的 argv 部分**P12 不动**（§4.2.3 已裁决登记待设计） |
| `ops` | `ternary`、`ternaryLazy`、`orVal`、`orLazy` | 4 个全部实现 |
| `lowlevel` | `id`、`rasm`、`ri`、`bindDllExt`、`unbindDllExt` | 只实现 `ri`；其余 4 个删（见 S1） |
| `datatype` | `type`、`setType`、`checkType`、`isIter`、`DT` 类 | **整模块删**（见 S3），与静态架构冲突 |

> ⚠️ **原仓库签名里的 `any` 分两种情况，已实测区分**：
> - `funi[any, int, int, .*][any]` —— 函数类型**内部**的 `any` **能正常解析**
>   （探针 `chk_funi1`/`chk_funi2`/`chk_funi3` 全部通过）。`ds.iter`/`ds.sort`/`ds.repeat`
>   的签名照原样抄即可，**不依赖 G13**。
> - 裸的形参/返回值标注 `fun copy(arg: any): any` —— **不认**（G13）。
>   改用省略标注 `fun copy(arg) { ret arg }` 即可，语义等价（探针 `g13a`/`r1`）。
>
> 结论：**G13 不是 P12-3 的硬前置**，只是让签名能逐字照抄的便利项。

#### 5.4.1 逐条 API 建议（等你逐条审批；未批一律按基线实现）

用户已批准「允许提改名/增删建议，逐条审批」。下表是**建议**，每条独立生效。
**默认全部不采纳** —— 你不回复 = 全部按原 API 面重写。

| 编号 | 模块 | 建议 | 理由 | 影响面 | 建议采纳? |
|---|---|---|---|---|---|
| S1 | `lowlevel` | **删** `id`、`rasm`、`bindDllExt`、`unbindDllExt` | RA 已删除，这四个原理上无法实现（见 §5.5-A） | 仅 `lowlevel` 模块；`release/Lib/builtin` 里无人调用 | ✅ 建议采纳 |
| S2 | `lowlevel` | **保留** `ri`，但**删掉 `import(prog="prog")`** | `ri` 只是字符串格式化，可实现；它 import prog 只为 `entry`，`entry` 要删 | 无 | ✅ 建议采纳 |
| S3 | `datatype` | **整个模块删除**（`type`/`setType`/`checkType`/`isIter`/`DT`） | 静态类型下运行时类型自省不成立（见 §5.5-A） | `ds.rio` 里 `dt.isIter` 被 `ds.iter` 用到 → 改写成直接判断 | ✅ 建议采纳 |
| S4 | `ds` | **删** `unpack`（依赖 `setType`）、**删** `iterContains`/`iterRemove`（依赖 RA 迭代器） | 原理上无法实现 | 无 | ✅ 建议采纳 |
| S5 | `prog` | ~~**删** `entry`，保留 `breakpoint`~~ → **改为：`entry` 保留并补实现** | 用户 2026-11-16 确认「原始设计是这样的但是并未实现」：argv → 回调是**明确的设计意图**，只是没落地。属「该补」而非「该删」 | 需先过 §4.2.3 的 Q1–Q8 设计审批 | ❌ **原建议作废** |
| S6 | `ds` | **保留 17 个**（20 减 S4 的 3 个） | 原仓库公开 API 且新架构可实现 | 无 | ✅ 建议采纳 |
| S7 | `str` | **保留全部 12 个**，不改名 | 名字没有明显问题，改名只是风格偏好 | 无 | ⬜ 不采纳（维持原样） |
| S8 | `math` | **保留全部 7 个**，`opp` 不改名 | `opp` 是「取相反数」，名字怪但语义明确，改名会破坏兼容 | 无 | ⬜ 不采纳（维持原样） |
| S9 | `ops` | **保留全部 4 个** | 无问题 | 无 | ⬜ 不采纳（维持原样） |
| S10 | `io` | `sin`/`input` 保留原名，但 `sin` 名字与 `math.sin` 混淆风险 → 建议 **`sin` 加注「readLine 的别名」**，不改名 | 改名属破坏性变更 | 无 | ⬜ 不采纳（仅加注释） |
| S11 | `ds` | ~~**新增** `listSet(a, i, v)` / `dictSet(d, k, v)`~~ | 用户第四轮决定：**不新增**，改用 `list[...] = x` 索引赋值语法 | 无新增公开函数 | ✅ **已裁决：不新增**（见 §G2） |
| S12 | `ds` | **新增** `strEq(a, b): bool` | G9 修好后 `==` 就能按内容比，这个不再必要 | 无 | ❌ 不建议（G9 修好后冗余） |

**S11 已裁决（2026-11-16 第四轮）**：用户选择「**不新增 `listSet`/`dictSet`，改用 `list[...] = x`
索引赋值语法**」。这意味着：
- 标准库**不新增**任何公开函数（原路 A 的目标达成）；
- 但「写容器元素」改为依赖**语言级索引赋值**，而不是 `core_mem_set` 内部绕过；
- ⚠️ 索引赋值本身是**新增语言语法**（源码证据见 §G2.1），需单独批准。

在索引赋值落地前，标准库内部写元素**仍走 `core_mem_set` + 指针算术**（这是内部实现手段，
不暴露为公开 API），与「不新增公开函数」一致。

**实现层面**：这些函数原本大多靠 `encapsulated` 指向 RA 内置函数注册表，
注册表在 P12-1 被删后，改为用新后端可表达的能力（`core_*` + 指针算术 + 循环）实现。
如果某个函数**在原理上无法用新架构实现**（例如 `rasm` 内联 RA 指令、`id` 取 RA 标识符、
`bindDllExt` 加载 DLL 扩展），则单独列出来请审批 —— 不自行删除或改名。

### 5.5 无法实现的旧函数清单（已逐个核实，2026-11-16）

核实方法：对每个 `encapsulated` 函数读原仓库实现意图，再在新后端上写探针验证
「能否用 Rio + `core_*` 表达」。结论分三档。

#### A. 原理上无法实现（新架构没有对应概念，无法用任何 Rio 代码替代）

| 模块.函数 | 原仓库语义 | 为什么做不了 | 建议 |
|---|---|---|---|
| `lowlevel.rasm(racode)` | 把字符串当 RVM/RA 指令**注入编译产物** | RA 虚拟机已在 P12-1 整体删除，新后端直接发 x86-64 汇编。运行时不存在「注入指令」这回事 | **删除**。等价的底层能力已经是 `core_*`（`mem_*`/`alloc`/`io_*`），不需要字符串指令 |
| `lowlevel.id(arg)` | 返回编译器给该变量分配的 **RA 代码标识符** | RA 标识符随 RA 删除而不存在；新后端里变量是 `rbp` 负偏移的栈槽，没有可暴露的字符串名字 | **删除** |
| `lowlevel.bindDllExt(dllPath)` | 运行时加载 DLL 并注册其中的 RA 扩展 | 新架构没有 RA 扩展注册表，也没有「运行时把符号接进编译器」的机制 | **删除**；需要 DLL 互操作时应作为独立特性重新设计并审批 |
| `lowlevel.unbindDllExt(ident)` | 卸载上面注册的扩展 | 同上 | **删除** |
| `datatype.type(arg)` | 返回运行时类型名字符串 | 新架构是**静态**的：变量类型在编译期已知，运行时值不携带类型标签。要返回正确字符串必须给每个值加类型标签（改值表示 = 改 ABI） | **删除**（或降级为只支持字面量的编译期宏，但这算新特性，需另行审批） |
| `datatype.setType(arg, t)` | 运行时**改**一个值的类型 | 同上，且更严重：静态类型下「改变量类型」在语义上不成立 | **删除** |
| `datatype.checkType` / `datatype.isIter` | 基于 `type()` 的判断 | 依赖上面两个 | **删除** |
| `datatype.DT` 类 | 一堆 `type()` 结果当常量 | 依赖 `type()` | **删除** |
| `ds.unpack(arg)` | 把值标成 `series` 类型（`dt.setType(arg,"series")`） | 依赖 `setType`。且 `series` 在新架构里与 `list` 同布局（`parseTypeName` 把 `series`/`array` 都映射到 `ListType`），「标成 series」没有可观察效果 | **删除**，或保留为恒等函数 `ret arg`（无意义，建议删） |
| ~~`prog.entry(arg, *args, **kwargs)`~~ | ~~「当前文件是主程序才调用」~~ | **此条结论已推翻**（2026-11-16 用户确认「原始设计是这样的但是并未实现」） | ⚠️ **移出本表**，见下方说明 |
| `io.sin` / `io.input` | 读一行 stdin | 技术上**可以**做（`core_io_read` 探针已验证能读到字节），但需要处理行缓冲/EOF/超长行。不是「原理上不能」，而是**成本高于价值** | **保留但降级**：先实现成「读到 `\n` 或 EOF 为止，超长截断」，或用 `core_io_read` 逐字节读。归入 P12-3 实现 |

> **`prog.entry` 更正说明（2026-11-16）**
>
> 本表原先把它列为「原理上无法实现，建议删除」，**这个结论是错的**，现予推翻：
>
> - **设计意图明确存在**：argv 应作为 `*args` / `**kwargs` 传给入口回调。用户已确认「原始设计是这样的」。
> - **只是从未实现**：`rcc_entry` 只做了「编译期判断当前文件是不是入口文件」这一半，
>   argv → 回调那一半**从来没有代码**（详见 §4.2.2）。
> - **在新架构里做得到**：`-build` 时主文件路径是已知的，路径比较可以在编译期完成；
>   argv 也可以在 `main` 里取到并转成 Rio `list`。
> - **所以它不是「该删」，而是「该补」**，且补的方式必须先过设计审批（§4.2.3 的 Q1–Q8）。
>
> 相应地，§5.4.1 的 **S5（删 `prog.entry`）作废**，`prog` 模块保留 `entry` 与 `breakpoint`。

#### B. 可以实现，但需要先补 §1.2 的缺口

| 函数 | 卡在哪个缺口 |
|---|---|
| `ds.size` | 无（`core_str_len` 对 str/list/dict 都成立，已实测 3/4/2） |
| `ds.copy` / `ds.top` / `ds.repList` | G4（`list + list`）、G12（ptr 返回值） |
| `ds.listAppend` / `ds.listRemove` / `ds.dictRemove` | 追加/删除元素：用 `core_mem_copy` 搬移 + 直接改长度头（**不依赖索引赋值**） |
| `ds.dictKeys` / `ds.dictValues` | 需要读 DictHeader 的 `keys`/`vals` 指针（+24/+32），可用指针算术 |
| `ds.sort` / `ds.join` / `ds.iter` / `ds.forInRange` / `ds.repeat` | G6（闭包写回）、G9（str 比较） |
| `str.toStr` / `str.toChar` / `str.subStr` / `str.find` / `str.split` / `str.chunk` / `str.replace` / `str.trim` | G10（`s[i]`）、G3（`str + str`）、G9（str 比较） |
| `str.toLowerCase` / `str.toUpperCase` / `str.charToInt` / `str.intToChar` | G10（逐字节访问） |
| `ops.ternary` / `ops.ternaryLazy` / `ops.orVal` / `ops.orLazy` | 无（已实测可编译） |
| `math.*` | G1（`pi`/`e` 是顶层 `var`） |
| `lowlevel.ri` | G3（`str + str`） |

#### C. 已实测确认可编译（签名直接沿用原仓库）

`ds.iter` 的 `funi[any, int, int, .*][any]`、`ds.sort` 的 `funi[any, any][int] = null`、
`ds.repeat` 的 `funi[int][any]`、`ops.ternaryLazy` 的 `funi[][any]`、
`lowlevel.ri` 的 `(i: str, *args)`、`io.print` 的 `(*args, end, sep)`、
`prog` 的 `(*args, **kwargs)` —— 这些复杂签名都能过编译（探针 `r3`–`r6`、`kk1`–`kk5`）。

> **注意**：`any` 作为**形参/返回值类型标注**目前不认（`Unknown type 'any'`），
> 但**省略标注**（`fun copy(arg) { ret arg }`）可以正常工作（探针 `r1`）。
> 原仓库大量函数正是省略标注的写法，所以这条不影响重写。
> 若确实需要显式 `any`，那是 §1.2 之外的另一个缺口（G13），需单独决定。

### 5.6 验收：三层分层验收（编译 / API / 行为）

> **编译通过 ≠ 实现正确。** 三层必须全过才算模块完成，任一层失败该模块即判定未完成。

新增 `test_scripts/test_std.ps1`，按三层组织：

#### 第一层：编译测试（`-Layer compile`）

1. 对 `lib/std/*.rio` 逐个 `RCC -build -emit-lib --path X --output build/stdcheck/X.o`
   —— 库模式不发射 `main`，只验证语法与代码生成能过
2. **不依赖旧 RA 内置函数注册表**：用 `git grep` 断言 `lib/std/` 下不出现
   `encapsulated`（除 `io.sout` 这类编译器特化桩）与任何已删的内置函数名
3. **不依赖已删 legacy 路径**：断言 `lib/std/` 不 `import` 已删除的模块
4. 把这一层挂进 CMake（`rcc_std_check` 目标），`make` 就发现回归

#### 第二层：API 测试（`-Layer api`）

对**每个模块的每个公开函数**逐条核对，不抽样：

| 检查项 | 做法 |
|---|---|
| 函数存在且名字正确 | 生成一个 `build/stdcheck/api_<module>.rio`，对每个公开函数**至少写一次调用**（含默认参数省略的写法），编译通过 |
| 参数个数 / 顺序 | 用 `-s` 导出的符号表与 §5.4 的原仓库签名逐条 diff |
| 默认参数 | 省略该参数能编译，且行为等价于显式传默认值 |
| 返回类型 | 赋值给对应类型的 `var` 能通过 `checkAssignable` |
| 模块导入行为 | `import ds` 后能用裸名调用；`import` 不重复定义符号 |
| 已删 API 确实不存在 | 对 §5.5-A 判定删除的函数，写一个**期望编译失败**的用例 |

产出：`build/stdcheck/api_report.txt`，列出「模块 / 函数 / 签名一致 / 默认参数 / 返回类型」，
**缺一个函数就失败**。

#### 第三层：行为测试（`-Layer behavior`）

每模块用例分五类：

| 类别 | 要求 |
|---|---|
| 正常输入 | 每个公开函数的典型用法至少 1 例 |
| 边界输入 | 下标 0 / 末位 / 空容器 / 单元素 |
| 空输入 | 空 `str` `""`、空 `list` `[]`、空 `dict` `{}` |
| 非法输入 | 越界、类型错配、`null` 参与运算 —— 记录「panic」或「返回什么」的实际行为 |
| 内存相关 | `ds.copy` 是深拷还是浅拷、`listAppend` 后原 `list` 是否仍可用、重复 `alloc` 是否泄漏 |

**覆盖度硬要求**：`ds`（20 个）、`str`（12 个）、`math`（7 个）、`ops`（4 个）、
`io`（4 个）、`prog`（2 个）、`lowlevel`（1 个，`ri`）—— **每个函数至少 1 个行为用例**，
不能只挑 `size` / `join` / `subStr` 几个代表性函数通过就宣布模块完成。

#### 汇总

`test_std.ps1` 输出三层各自的结果表，并给出一张总表：

```
模块   编译   API    行为        结论
ds     PASS   18/20  17/20       ✗ 未完成（2 个 API 缺失，3 个行为失败）
str    PASS   12/12  12/12       ✓ 完成
```

任一模块任一层非全绿，`exit 1`。

## 6. P12-4：脚本重写

### 6.1 `scripts/run.bat`

现状是 LLVM 时代的五步流水线，其中 `LLC_EXE` 指向
`D:\repositories\llvm-project\build\bin\llc.exe`（已不存在），
`CMAKE_BUILD_DIR` 指向 `D:\ClionProjects\RCC\cmake-build-debug`（已失效）。

新版本：

| 旧步骤 | 新做法 |
|---|---|
| `RCC -ir --p in.rio --ir-o out.ll` | `RCC -build --path in.rio --output out`（一步到位） |
| `llc -filetype=obj out.ll -o out.obj` | 删除（`as` 由 RCC 自己调） |
| `g++ out.obj -o out.exe` | 删除（`ld`/`g++` 由 RCC 自己调） |
| `cmake --build D:\ClionProjects\...` | `mingw32-make -C build -j 10`，且路径从脚本自身位置推导 |

保留 CLI：`-i <test_dir>`、`--no-run`、`--skip-cmake`、`-h`。

### 6.2 `scripts/test.bat`

现状：遍历 `tests\test_*`，对每个调 `run.bat`，用 `fc /W` 比对 `expected.txt`。

问题：bat 里的中文路径、引号、`fc` 的行尾处理都很脆（`fc /W` 会吞掉空白差异，
和 `test_legacy_all.ps1` 的逐字节比对语义不一致）。

新版本：`test.bat` 退化成薄壳，实际逻辑放进 `test_scripts/test_all.ps1`：

```bat
@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\test_scripts\test_all.ps1" %*
```

`test_all.ps1` 依次跑 `test_p0..p11` + `test_std` + `test_legacy_all`，
汇总成一张表，任一失败就 `exit 1`。这样「一条命令跑完全部」且只有一套判定逻辑。

### 6.3 `tests/test_14_iter` / `tests/test_15_forinrange`

这两个用例的 `expected.txt` 与 `test.rio` 依赖旧 `ds.iter` / `ds.forInRange` 的
「回调里改外层变量」契约（G6）。**G6 在 P12 不做**（§4.3、§10.2）。

因此：

| 项 | 处置 |
|---|---|
| `test.rio` | **原样保留，不动** |
| `expected.txt` | **原样保留，不动** |
| 这两个用例的 P12 状态 | 标记为「⚠️ 已知失败 —— G6 延期导致」，**不计入 P12 验收门槛** |
| `test_legacy_all.ps1` 的判定 | 改为 15/17 通过即算通过，并在输出里明确列出这两个是「G6 延期」而非「回归」 |

> ❌ **明确禁止**：把 `expected.txt` 改成「不写回」的结果、或把 `test.rio` 改写成
> 「`iter` 返回累计值」的新契约来让它们变绿。那是用修改测试预期掩盖真实缺陷，
> 会把问题静默带到下个版本。用户 2026-11-16 明确要求过这一点。

等 G6 与内存模型定稿后，这两个用例应当**原样**通过，作为 G6 完成的最强证明。

## 7. P12-5：文档与发版

| 项 | 动作 |
|---|---|
| `docs/rio_doc.md` | 现在 75 行、全是 `...` 占位。按新语言重写：词法/类型/表达式/控制流/函数/类/异常/模块/泛型/重载 + 标准库参考 |
| `docs/architecture_plan.md` | §9.1 进度行改 `P12 ✅`；§10 只记录**用户已批准**的后续方向，未批准的提案留在 §1.4 不进架构计划 |
| `code/include/rcc_version.h` | `1.21.0` → **`1.22.0`**（P12 是 Feature 级：删旧路径 + 新标准库） |
| `logs/update_logs.md` | 版本总览表加 `v1.22.0` 行 + 详细条目（`**[Feature] [Refactor] [Breaking]**`） |
| `scripts/config/update_config.json` | `validate: true`，填 `version.vs_desc` / `change_type` / `update_log.log_contents` |
| `README.md` | 删 LLVM 依赖说明；补「构建 = `cmake -B build -G "MinGW Makefiles"` + `mingw32-make -C build`」 |
| `.gitignore` | 加 `/release/Lib/builtin/`（CMake 复制产物） |

发版脚本 `scripts/execute.py` 依赖 `scripts/.env`（当前**不存在**，只有
`.env.example`，且里面的路径指向 `D:\ClionProjects\RCC`）。P12 不强行跑发版脚本，
改为：手动打 tag + 手动整理 release 目录，并在文档里写明 `.env` 的字段要求。
（跑脚本前必须先补 `.env`，这是运维动作而非代码动作。）

### 7.1 Breaking Changes 记录表（发版说明必须逐项填实）

版本号 `1.22.0` 不变，但**发版说明必须真实反映实际完成的改动**，
不得把未实现/未验证的功能写成已完成。这张表是发版前的**强制检查项**：

| 检查项 | 必须记录的内容 | 填写状态 |
|---|---|---|
| **语言语法** | 是否有变化？逐条列（P12 预期：**无语法变化**，G7 是修 bug 不是改语法） | ⬜ 待填 |
| **编译器 CLI** | 是否有变化？逐条列（P12 预期：`-ir` 删除、`-s` 实现替换但输出不变、`-build` 不变） | ⬜ 待填 |
| **标准库 API** | 新增 / 删除 / 修改，逐函数列（依据 §5.4.1 的最终审批结果） | ⬜ 待填 |
| **ABI** | 是否有变化？（P12 预期：`core_*` 冻结不动；`rt_*` 新增 `rt_str_concat`/`rt_list_concat`/`rt_str_eq`，属**新增符号**不改既有 ABI） | ⬜ 待填 |
| **旧项目迁移** | 是否需要改源码？给迁移对照表（如 `ds.unpack` → 直接用 list） | ⬜ 待填 |
| **legacy 移除** | 明确列出删掉的 18 个源文件、`-ir` 路径、DLL 扩展机制、`datatype` 模块等 | ⬜ 待填 |

**填写纪律**：

- 每项必须写「无变化」或列出具体变化，**不允许留空或写「见上文」**
- 与 §10.2 的延期项对照：G1b（argv 传递）和 G6（闭包写回）**未完成**，
  发版说明里**不得**把它们描述为已支持；应写明「当前版本 `prog.entry` 仅支持调用点实参，
  argv 传递待后续版本」「闭包捕获为按值捕获，写回待后续版本」
- `logs/update_logs.md` 的详细条目按 `**[Feature] [Refactor] [Breaking]**` 分类，
  每条 Breaking 必须在迁移说明里有对应段落

## 8. 风险清单

| 风险 | 影响 | 缓解 |
|---|---|---|
| G6（闭包写回）改的是槽位寻址抽象 | 9 处 `stackOffset` 直接拼地址，改漏一处就是静默错值 | **P12 不做**（§4.3）。等内存模型定稿后再动，避免与未来方案冲突 |
| **为赶进度做临时方案** | 与最新版架构设计冲突，后续要推倒重做 | 原则 2/6：未定稿的设计一律标「待设计」并延期（§10.2），不写临时实现 |
| **用改测试预期的方式让测试变绿** | 掩盖真实缺陷，问题带到下个版本 | §10.2 明确禁止：`test_14_iter`/`test_15_forinrange` 的 `expected.txt` 与 `test.rio` 原样保留 |
| 未经审批就引入语言设计 | 违反 2026-11-16 的约定 | 任何「新增语法/特性/删改原仓库 API」先写进 §1.4 提案表等批准。**A1–A6 已否决；G1b 的 Q2（`--key=value`）与 S1–S12 未批一律不动** |
| `-s` 重写后输出格式变化 | 依赖旧输出格式的脚本失效 | 已获用户批准重新设计（§3.3）；`test_symbol_export.ps1` 校验六种格式各自的结构性质；`update_logs.md` 记 `[Breaking]` |
| **删 legacy 时漏依赖，留下无法解释的残留** | 构建失败或隐藏的旧路径 | §3.2 七步审计（A1–A7）全部有结论才能动手；P12-1 独立提交 |
| `lib/std` 复制到 `release/Lib/builtin` 的时机 | 首次构建时目录可能为空 → `import ds` 失败 | CMake `add_custom_target(rcc_std ALL)` + `add_dependencies(RCC rcc_std)` + `make_directory` |
| 旧库删除后老用户代码编不过 | Breaking | `update_logs.md` 记 `[Breaking]` + §7.1 迁移对照表 |
| **「编译通过」被当成「实现正确」** | 标准库带着一堆未实现的函数发版 | §5.6 三层验收，API 层要求每个公开函数都有调用用例，缺一个就失败 |
| G9 改 `==` 语义会动到 `rt.rio` 自举 | `rt_str_eq` 误用 `==` 会无限递归；`rt.o` 构建期编译，一挂整个构建失败 | `rt_str_eq` 只用 `core_str_len` + `core_mem_eq`，禁止出现 `==`；先单独编 `rt.rio` 验证 |
| G12 放宽 `ptr`/`int` 同族 | 失去「整数误当指针」的静态检查 | 仅放行 `ptr`↔`int`，不放开 `ptr`↔`str`/`list`；文档写明 `ptr` 是无类型裸地址 |
| **argv 通道与全局初始化顺序冲突** | 全局 `var` 初始化时读到未初始化的 argv | §4.2.4：argv 必须在顶层 `var` 初始化**之前**存入全局槽；该顺序写进实现并加用例 T7 |

## 9. 总验收清单

```powershell
# 1) 构建
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -C build -j 10

# 2) 分阶段套件（全部 0 失败）
foreach ($p in 0,1,2,5,6,7,8,9,10,11) {
    powershell -File "test_scripts/test_p${p}_*.ps1"
}

# 3) 标准库 —— 三层都要全绿（§5.6）
powershell -File test_scripts/test_std.ps1 -Layer compile
powershell -File test_scripts/test_std.ps1 -Layer api       # 每个公开函数都有调用用例
powershell -File test_scripts/test_std.ps1 -Layer behavior  # ds 20 / str 12 / math 7 ...
powershell -File test_scripts/test_std.ps1                  # 三层汇总表，任一非全绿 exit 1

# 4) -s 六种格式校验（§3.3）
powershell -File test_scripts/test_symbol_export.ps1        # 六种格式结构校验

# 5) legacy（已知 test_14/15 因 G6 延期而失败，不算回归）
powershell -File test_scripts/test_legacy_all.ps1

# 6) 一条命令全跑
scripts\test.bat

# 7) 仓库干净度
git grep -il "llvm" -- ":!docs/*" ":!logs/*"           # 无输出
git grep -n "llc\|-ir \|\.obj" -- "scripts/*"          # 无输出
RCC.exe -v                                             # 1.22.0
```

**P12 完成的判定（全部满足）**：

| # | 条件 |
|---|---|
| 1 | 构建零错误；`test_p0`–`test_p11` 全绿 |
| 2 | 标准库**三层**全绿，且 `ds`/`str`/`math`/`ops`/`io`/`prog`/`lowlevel` 每个公开函数都有行为用例 |
| 3 | `-s` 六种格式全部输出正常且结构校验通过 |
| 4 | §3.2 的七步依赖审计全部有结论，无无法解释的残留 |
| 5 | §7.1 的 Breaking 记录表六项**全部填实**，且未把延期项写成已完成 |
| 6 | §10.2 的延期项在文档里有明确登记（G1b、G6） |
| 7 | 未擅自引入任何未获批的语言设计。<br>**特例（已获批）**：`list[i]=v` / `dict[k]=v` / `a[i]+=v` / `a+=[v]` / `s[i]` 读取——这些经 2026-11-16 实测确认是**原仓库已有的能力**（新架构的功能回归），恢复它们不属于「新增设计」 |
| 8 | 索引赋值按 §G2.5 六条口径落地：`list`/`dict` 写入可用、`a[i]+=v` 可用、`a+=[v]` 可用、`s[i]` 可读、**`s[i]=c` 编译期报错**（不得静默失效） |

## 10. 建议的执行顺序（跨会话）

| 会话 | 内容 |
|---|---|
| 0 | ✅ **已完成（2026-11-16）**：A4/A5/A6 → 否决；A7–A12 → 允许逐条提建议；无法实现的函数 → 已核实并列清单（§5.5） |
| 1 | P12-0 冻结基线 + P12-1 删旧 RA 路径（含 `-s` 迁移）。这一步收益最大、风险最低，且**不涉及任何语言设计** |
| 2 | P12-2 的 G2 / G7 / G1a / G3 / G4 / G9 / G10 / G11 / G12（都是实现缺口，每条配回归） |
| 3 | P12-2 的 G5（方法重载）。**G6 不做**，见 §10.2 |
| 4 | P12-3 标准库重写（按 §5.4.1 获批的结论；S5 已作废） |
| 5 | P12-4 脚本 + P12-5 文档发版 + 最终全量回归 |
| — | **G1b（argv→回调）与 G6（闭包写回）不在 P12 排期内**，见 §10.2 |

每会话结束都必须留下**可构建、可测试**的树，不允许半截状态跨会话。

### 10.1 待用户拍板的点

**已裁决（2026-11-16 第四轮）**：

| # | 问题 | 用户决定 |
|---|---|---|
| 1 | S1–S4、S6 删除清单 | ✅ **批准** |
| 2 | 是否新增 `listSet`/`dictSet` | ✅ **不新增**，改用 `list[...] = x` 索引赋值 |
| 3 | `prog.entry` 处置 | ✅ **P12 不动，登记待设计** |
| 4 | G6 是否延期 P13 | ✅ **同意延期** |
| 5 | 内存模型冲突（引用计数） | ✅ **以规范为准，改架构计划** |

**已定案（2026-11-16 第五轮，用户裁决「str 支持读取不支持修改」）**：

| # | 问题 | 定案 |
|---|---|---|
| 6 | **G2 索引赋值的 `str` 支持**（§G2.4 的 Q2） | ✅ **`s[i]` 读取支持，`s[i] = c` 不支持**（编译期报错）。<br>理由：原仓库的 str 写入是**静默失效的 bug**（`String::getDataAt` 返回副本，实测输出 `hello` 而非 `hXllo`），不得复制。见 §G10.1 |
| 7 | **G2 索引赋值的完整语义** | ✅ `list[i]=v` / `dict[k]=v` / `a[i]+=v` / `a+=[v]` 全部实现（原仓库 v1.21.2 实测支持，属**功能回归**）；`s[i]` 读取实现（G10）；`s[i]=c` 编译期报错。见 §G2.4 / §G2.5 / §G10.2 |

**至此 §10.1 无待决项。** 其余 S7–S10、S12 我建议**不采纳**（保持原样），已在表中标注。

### 10.2 延期项登记（明确标注，不用改测试预期掩盖）

| 项 | 状态 | 原因 | 依赖 | 后续计划 |
|---|---|---|---|---|
| **G1b** 程序运行参数传递 | ⏳ **待设计**（P12 不动） | 设计意图明确（argv → `*args`/`**kwargs`）但**从未实现**；规则 Q1–Q9 未定义；且有规范空白（启动期 argv 的出生地归属，Q9） | 内存模型补齐 Q9；用户对方案 A/B/C 的裁决 | P13 处理。P12 只保留「不依赖 argv」的用例 T1/T7 |
| **G6** 闭包捕获写回 | ⏳ **待设计**（P12 不动） | 内存模型规范已存在（`memory-model-spec-full.md` v0.3，805 行），但：①需完成 **Rio 语法映射**（规范是伪代码，`Ref<T>`/`in local` 等需全新设计）②需落实「以规范为准」的架构计划修订 ③规范自身仍有 3 处【待确认】+ 14 条未决问题 | Rio 映射完成 + 架构计划修订 | 完成后按 §4.3.4 的 C1–C4 验收。**P12 不写任何临时方案** |
| `test_14_iter` / `test_15_forinrange` | ⚠️ 已知失败 | 依赖 G6 的回调写回契约 | G6 | **`expected.txt` 与 `test.rio` 原样保留不动**，只标注「G6 延期导致」。禁止改成「不写回」的结果来制造通过 |
| **内存模型 Rio 映射** | ⏳ **待设计** | `memory-model-spec-full.md` 是脱离 Rio 设计的伪代码；`Ref<T>`/`Mut<T>`/`Take<T>` 能力类型、`in local`/`in result` 出生地、`Pool`/`Link` 均需按 Rio 语法重新设计 | 用户逐条审批 | P13 主体工作。P12 只做**文档层**的架构计划修订（§8.1 改指规范、`rc` 标注废弃） |

## 11. 总体执行原则（本计划的总纲，任何小节冲突时以本节为准）

1. **语言设计层面的变化必须经过明确审批。**
   未获批的提案只进 §1.4，不得写进实现。判断依据必须是原仓库源码，不是我的推断。
   **注意**：用户的记忆/口头描述与源码冲突时，**先用用户指定的真实仓库版本实测**再下结论；
   我曾在克隆版 v1.21.0 上误判「索引赋值从未支持」，被用户用 v1.21.2 实测反证（见 §G2.2）。
2. **实现必须与最新版架构设计保持一致。**
   架构基线是 `docs/architecture_plan.md`；内存管理以 `docs/memory-model-spec-full.md` 为准
   （用户 2026-11-16 裁决）。两者冲突处，规范优先，架构计划需修订。
3. **不为历史兼容性制造不必要的技术负担。**
   见 §1.3.1。删旧 API 不需要理由，「新架构下没意义」就是理由。
4. **删除旧代码之前必须完成依赖审计。**
   见 §3.2 的 A1–A7 七步，全部有结论才能动手。
5. **每个子阶段结束时都必须保持可构建、可测试。**
   不允许半截状态跨会话；P12-1 尤其要独立提交。
6. **尚未完成设计的内容必须明确标注，不允许自行假设设计结论。**
   当前有三项：G1b（§4.2）、G6（§4.3）、内存模型 Rio 映射（§10.2）。
   标注为「待设计」的项，**不得**为赶进度写临时实现。
7. **发版时如实记录 Breaking Changes。**
   见 §7.1 的六项检查表。未实现/未验证的功能不得写成已完成。

### 11.1 本次修订的冲突自查（用户要求「检查章节间是否冲突」）

| 冲突点 | 原状 | 修订后 | 结论 |
|---|---|---|---|
| **G1 与 §5.5-A** | §5.5-A 说 `prog.entry` 原理上无法实现、建议删 | 用户确认「原始设计是这样的但未实现」，第四轮定「P12 不动」 | ✅ 已改：§5.5-A 加更正说明，S5 作废，`entry` 保留待设计 |
| **G1 与 G6 的初始化顺序** | G1 只说「顶层 var 在 `__rio_main` 里初始化」 | argv 需在顶层 var 初始化**之前**就位 | ✅ 已加 §4.2.4 顺序约束与风险行 |
| **G6 与标准库重写** | 原计划 `ds.iter`/`forInRange` 依赖 G6 的写回 | G6 延期，标准库改为纯函数式（返回累计值） | ✅ §4.3.3 已明确「标准库不得依赖写回」 |
| **G6 与 test_14/15** | 原计划「G6 没做就改写用例为新契约」 | 用户明确禁止改测试预期掩盖问题 | ✅ §10.2 已改为「`expected.txt` 与 `test.rio` 原样保留」 |
| **标准库验收 vs 「编译通过」** | 只要求模块独立可编译 | 三层验收，编译只是第一层 | ✅ §5.6 已重写，API 层要求每个函数都有用例 |
| **§5.4 基调 vs 兼容性** | 「一律照原仓库 API 面」 | 「以基线为准，但不为兼容保留无意义项」 | ✅ §5.4 与 §1.3.1 已统一口径 |
| **S6 数量** | 「保留全部 20 个（除 S4 的 3 个）」自相矛盾 | 17 个 | ✅ 已修正 |
| **P12-1 的 `-s`** | 原方案要求「与旧输出逐字节一致」，但旧 `symbolId` 依赖全局自增计数器、旧 JSON 键序来自 `unordered_map`，客观上不可复现 | 用户裁决重新设计为六格式导出器；`test_symbol_export.ps1` 校验结构性质 | ✅ §3.3 已改 |
| **§7 发版 vs 延期项** | 只说要写 Breaking | 六项检查表 + 明确禁止把 G1b/G6 写成已支持 | ✅ §7.1 已补 |
| **A5 前后矛盾**（第五轮已纠正） | 我第三轮误判「A5 索引赋值从未支持」并否决，用户实测反证 | §G2 整节重写：记录我错在哪（只查克隆版 v1.21.0、把 `ITER_GET` 当只读、搜 `ITER_SET` 零命中）、真实机制（VM 的 `shared_ptr` 引用语义）、回归根因（新架构去装箱后元素变内联值）、恢复方案 | ✅ **已纠正**：G2 归入 P12-2 必做项 |
| **内存模型 vs 架构计划**（本轮新发现） | `architecture_plan.md §8.1` 建议「引用计数」，`memory-model-spec-full.md §1` 明确排除引用计数 | 用户裁决「以规范为准，改架构计划」 | ⚠️ **已裁决**；架构计划修订与 `rc` 字段废弃**待 P12 内完成文档修订** |
| **§4.3.1 「内存模型零命中」**（本轮新发现） | 上一轮写「全仓库搜索内存模型：零命中 / 不存在最新版设计」 | 规范已存在（805 行 v0.3），但需 Rio 映射 | ✅ 已整节重写，含冲突 A/B 分析 |
| **S11 与 G2**（本轮新发现） | S11 说「新增 `listSet`/`dictSet` 绕开索引赋值」，而用户要求用索引赋值 | 不新增函数，改用索引赋值语法 | ✅ §5.4.1 S11 已改；索引赋值未落地前标准库内部仍用 `core_mem_set` |
| **G2 与内存模型自洽性**（本轮新发现） | 未讨论 | 元素赋值属「原地改值」，按 `memory-model-spec §4.1` 与活跃子引用**不冲突** | ✅ 已写入 §G2.3 Q6 |

**已全部定案，无待决项**：第五轮用户裁决「str 支持读取、不支持修改」，我已按原仓库实测补齐
§G2.4 的 Q1–Q7、§G2.5 的六条落地口径、§G10.2 的三条口径。计划修订到此完成。

### 11.2 已裁决事项归档（2026-11-16 第四轮）

| 事项 | 裁决 | 落地位置 |
|---|---|---|
| S1–S4、S6 删除清单 | 批准 | §5.4.1 |
| `listSet`/`dictSet` | 不新增，改用 `list[...] = x` | §5.4.1 S11、§G2 |
| `prog.entry` | P12 不动，登记待设计 | §4.2.3、§10.2 |
| G6 | 延期 P13 | §4.3、§10.2 |
| 内存模型冲突 | 以规范为准，改架构计划 | §4.3.1、§10.2 |
