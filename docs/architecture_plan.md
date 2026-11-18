# RCC 新架构设计方案

> 状态：**提案待确认**，本文件只做设计与分析，未改动任何代码。
> 日期：2026-11-15
> 目标：Compiler / Runtime / Standard Library 三层分离；后端改为直接生成 x86-64 汇编；语言静态化（去 `any`，加泛型与重载）；为自举预留结构。

---

## 一、现有架构分析

### 1.1 代码规模实测

| 模块 | 文件 | 行数 | 结论 |
|---|---|---|---|
| Lexer | `rcc_lexer.cpp` | 232 | 保留 |
| Parser | `rcc_parser.cpp` + 3 个 builder | ~1,170 | 保留，需扩展泛型语法 |
| AST | `rcc_ast.cpp` + `rcc_ast_components.cpp` | 919 | 保留，需新增类型节点 |
| 符号系统 | `rcc_symbol.cpp` + 42KB 头 | 2,671 | **部分复用**，与 RA 耦合需剥离 |
| 语义+RA 生成 | `rcc_compile_visitor.cpp` | 3,921 | **不接新流水线**，暂留给 `-s` |
| LLVM 后端 | `rcc_llvm_visitor.cpp` | 3,978 | **删除** |
| RI 中间码 | `rcc_ri.cpp` | 273 | **删除**（RA 指令集表示） |
| 内置函数 | `rcc_builtin_*.cpp` 共 9 个 | ~600 | **重写**，见 1.3 |
| 工具库 | `rcc_utils.cpp` | 2,472 | 保留 |
| **合计** | | **26,293** | |

### 1.2 关键结构性发现

**发现一：现有「内置函数」是编译器内的 RA 代码生成器，不是运行时。**

以 `sout` 为例（`rcc_builtin_io_functions.cpp`）：

```cpp
BuiltinFuncRetType rcc_sout(ast::CompileVisitor &visitor, const CallInfos& callInfos)
{
    const auto &raCode = ri::SOUT("s-l", {"s-unpack", ...}).toRACode();
    return raCode;   // 返回的是 RA 指令文本
}
```

这 15 个 `encapsulated` 函数的实现全部以 C++ 函数形式硬编码在编译器里，输出 RVM 的 RA 指令。**这是新架构迁移的核心难点**：它们必须被拆解为「少量 core intrinsics（汇编实现）+ 大量用 Rio 自己写的 stdlib 实现」。

清单：

| 现内置函数 | 新归属 |
|---|---|
| `sout` / `sin` | core（`io.write` / `io.read`）+ std 包装 |
| `size` / `copy` / `repeat` | std（纯 Rio） |
| `listAppend` / `listRemove` / `dictRemove` / `dictKeys` / `dictValues` | core（内存操作）+ std |
| `type` / `setType` / `checkType` | **删除**（静态语言不需要运行时类型操作） |
| `entry` / `breakpoint` | core（少量） |
| `id` / `rasm` / `bindDllExt` / `unbindDllExt` | **删除**（RA 时代遗留） |
| `import` / `export` / `super` | 保留，改为编译期语义（不生成运行时代码） |

**发现二：`any` 在库代码里被当作「泛型占位符」滥用。**

`release/Lib/builtin/*.rio` 共 55 处 `any`，绝大多数出现在本应是泛型的位置：

```rio
fun iter(iterData, handler: func[any, int, int, .*][any], reverse: bool = false): bool
fun sort(sortHandler: func[any, any][int] = null, reverse: bool = false): List public
```

这说明**泛型不是新增特性，而是补上现有代码已经在用的洞**。去 `any` 与加泛型必须同批做。

**发现三：现有类型标签语法已经是参数化类型的雏形。**

```rio
func[int, bool, bool][any]     // 函数：参数列表 [返回类型]
list[int]                       // 列表元素类型
```

`LabelNode` 已经支持 `appendLabelDesNode(ListExpressionNode)` 递归嵌套（`rcc_prefix_builder.cpp:250`）。**泛型语法应直接复用 `[]`，不要引入 `<>`** —— 后者与比较运算符冲突，需要 C++ 那样的歧义消解规则，代价高且易错。

**发现四：本机工具链可用，LLVM 路线已确认死路。**

- `D:\soft\Clion\CLion 2026.2.3.1\bin\mingw\bin`：g++ 15.2.0、`as.exe`、`ld.exe` 齐全，target `x86_64-w64-mingw32`
- 实测 `.s → as → .o → g++ → .exe → 运行` 全通
- `D:\soft\llvm` 只有 clang/lld 二进制，**没有 IR 头文件，也没有 LLVMConfig.cmake**，现有 LLVM 后端在本机根本编不了

### 1.3 可以留下什么

| 保留 | 理由 |
|---|---|
| Lexer / Parser / AST | 前端质量可用，是最大的一笔资产 |
| `rcc_utils`（参数解析、字符串、文件、错误） | 与后端无关 |
| `rcc_symbol` 的作用域与符号表**结构** | 类型标签的嵌套表示思路正确 |
| `CompileVisitor` | 暂留给 `-s` 符号导出，新流水线不接它 |

| 删除 / 重写 | 理由 |
|---|---|
| `LLVMCodeGenVisitor` + 全部 `<llvm/*>` 依赖 | 路线已废弃 |
| `ri::` 全部类（RA 指令表示） | 后端不再是 RA |
| `rcc_builtin_*` 中吐 RA 的实现 | 拆成 core + std |
| `type` / `setType` / `rasm` / `id` / `bindDllExt` | 静态语言 + 原生后端下无意义 |
| `CompileVisitor` 的 RA 发射路径 | 等 P12 收尾时清 |

---

## 二、目标架构

### 2.1 三层分离

```
┌─────────────────────────────────────────────────────────┐
│  std       标准库（Rio 源码，泛型，随用户程序单态化）      │
│            Vec<T> / String / HashMap<K,V> / fmt / iter    │
└───────────────────────┬─────────────────────────────────┘
                        │ 只能向下依赖
┌───────────────────────▼─────────────────────────────────┐
│  rt        Runtime（Rio 源码，非泛型，预编译为 .o）        │
│            分配器 / panic / 字符串底层 / IO 封装 / 启动代码 │
└───────────────────────┬─────────────────────────────────┘
                        │ 只能向下依赖
┌───────────────────────▼─────────────────────────────────┐
│  core      编译器内在函数 + 平台接口（汇编 + 极少 C）      │
│            mem.* / io.* / proc.* / panic.raise / eh.*     │
│            ★ 冻结 ABI，是唯一的「种子」                    │
└───────────────────────┬─────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────┐
│  OS      Windows kernel32 / msvcrt（平台边界，到此为止）   │
└─────────────────────────────────────────────────────────┘
```

**铁律：依赖方向只能向下。`core` 不知道 `rt` 存在，`rt` 不知道 `std` 存在。**
编译器只依赖 `core` 的 ABI 声明，不依赖 `rt`/`std` 的实现。

### 2.2 与 Rust 的对应关系

| Rust | 本项目 | 说明 |
|---|---|---|
| `core` crate | `lib/core` | 无分配、无 OS 依赖的那一层 |
| `alloc` crate | `lib/rt` | 分配器 + 集合的底层 |
| `std` crate | `lib/std` | 面向用户的标准库 |
| `extern "C"` + `#[lang]` | `core` 的 `extern` 声明 + 编译器内在函数 | ABI 接缝 |
| monomorphization | 单态化 | 泛型实现方式 |
| trait / `dyn` | **待定**，见 §8.3 | 泛型打印需要某种约束机制 |
| `rustc` | `rcc` | 编译器 |
| `.rlib` | `.o` + `.rio` 接口 | 预编译产物 + 源码 |

---

## 三、模块划分

### 3.1 仓库目录

```
RCC/
├── compiler/                    ← C++ 实现（现 code/ 迁入）
│   ├── frontend/
│   │   ├── lexer/               ← 复用
│   │   ├── parser/              ← 复用 + 泛型语法
│   │   └── ast/                 ← 复用 + 类型节点
│   ├── sema/                    ← 新增
│   │   ├── types/               类型表示、类型相等/相容判定
│   │   ├── resolve/             名字解析、作用域链、模块导入
│   │   ├── check/               类型检查、类型推导
│   │   ├── overload/            重载解析
│   │   ├── mono/                泛型单态化
│   │   └── typed_ast/           带类型的 AST（sema 的输出）
│   ├── backend/
│   │   ├── abi/                 调用约定、布局、名字修饰
│   │   ├── asm/                 x86-64 GAS 发射
│   │   └── toolchain/           调用 as / ld，缓存
│   ├── driver/                  CLI、编译单元管理、诊断
│   └── legacy/                  旧 RA 路径（仅 -s 用，P12 删）
│
├── lib/
│   ├── core/                    ← 种子层
│   │   ├── core.rio             声明（extern，编译器解析此文件）
│   │   ├── core_win64.s         汇编实现
│   │   └── core_win64.c         少量 C 实现（便于维护的部分）
│   ├── rt/                      ← 用 Rio 写
│   │   ├── rt.rio               非泛型，预编译为 rt.o
│   │   └── (alloc / panic / io / start)
│   └── std/                     ← 用 Rio 写
│       ├── string.rio
│       ├── vec.rio
│       ├── hashmap.rio
│       ├── fmt.rio
│       └── iter.rio
│
├── tests/                       新测试体系：cases / agent / user 三区
├── docs/
└── scripts/
```

### 3.2 编译器内部数据流

```
.rio 源文件
   │ lexer
   ▼
Token 流
   │ parser
   ▼
AST（无类型）
   │ sema::resolve        ← 载入 core.rio / 其他模块接口
   │ sema::check          ← 类型推导与检查
   │ sema::overload       ← 调用点重载决议
   │ sema::mono           ← 泛型实例化（生成实例清单）
   ▼
TypedAST（每个表达式带 TypeRef，每个调用带确定的目标符号）
   │ backend::asm
   ▼
.s 汇编文本
   │ backend::toolchain
   ▼
.o  →  (链接 rt.o + core.o + 平台库)  →  .exe
```

**是否要引入中间 IR？** 建议**阶段一不引入**，TypedAST 直接下降为汇编。理由：语言规模小，引入 IR 会显著拉长 P2–P6 的工期；但 backend 内部要把「栈槽分配」「寄存器分配」抽成独立对象，未来需要优化时可在 TypedAST 与 asm 之间插入 IR 而不动前端。

---

## 四、ABI 设计

ABI 是三层之间唯一的契约，必须冻结并版本化。

### 4.1 调用约定（Windows x64，已实测确认）

用 GCC 反汇编实测（`g++ -O1 -S`），确认 MinGW 走标准 MS x64 ABI：

| 规则 | 内容 |
|---|---|
| 整型/指针参数 | 按**参数位置**依次用 `RCX, RDX, R8, R9` |
| 浮点参数 | 按**参数位置**用 `XMM0..XMM3`（位置 1 的 float 用 `XMM1`，实测确认，不是「第几个 float 用第几个 XMM」） |
| 超出 4 个的参数 | 压栈，调用者负责清理 |
| 栈参数偏移 | `40(%rsp)` 起（8 字节返回地址 + 32 字节 shadow） |
| Shadow space | 调用者**必须**预留 32 字节 |
| 栈对齐 | 调用前 `rsp` 必须 16 字节对齐 |
| 返回值（整数） | `RAX` |
| 返回值（浮点） | `XMM0` |
| 返回值（> 8 字节结构） | 调用者分配空间，指针作隐藏首参（`RCX`），后续参数右移 |
| 参数（9–64 字节结构） | **传引用**（指针），实测确认 |

**ABI 抽象位置**：`backend/abi/AbiInfo`，把所有平台差异收在这一处。将来加 Linux/SysV 只改这里 + 新增 `core_sysv.s`。

### 4.2 值表示

静态化后每个类型有原生表示，不再有 tag。

| Rio 类型 | 表示 | 大小 |
|---|---|---|
| `int` | `i64` | 8 |
| `float` | `f64`（`double`） | 8 |
| `bool` | `i8`（寄存器中用 `i64`） | 1 |
| `char` | `u32`（Unicode 标量） | 4 |
| `void` | 无 | 0 |
| `str` | **单指针** → `StrHeader` | 8 |
| `list[T]` | **单指针** → `VecHeader` | 8 |
| `dict[K,V]` | **单指针** → `MapHeader` | 8 |
| `func[..][R]` | `{ fnptr, env }` | 16（传引用） |
| `class` | 指针 → 实例 | 8 |
| `T?`（可空） | 指针，`null` = 0 | 8 |

**关键决策：`str` 用单指针而非「指针+长度」两字。**

理由：MS x64 下 16 字节结构按引用传递（实测确认），会产生额外间接寻址和临时对象。单指针是一个寄存器就能传的值，且：
- 字符串字面量指向静态只读 header，零分配
- 未来加引用计数只需在 header 里加字段，不破坏 ABI

```c
// 布局（core 与 rt 共享，写进 core.rio 的注释里冻结）
// ⚠️ 2026-11-16 裁决：rc 字段已废弃（内存模型规范 §1 明确非目标：引用计数）。
//    内存模型落地时移除，属 ABI 变更，需在 P13 单独评审。
struct StrHeader {
    i64 len;      // 字节数
    i64 cap;      // 容量（字面量为 0，表示不可变）
    i64 rc;       // 【已废弃】原预留引用计数，当前恒为 0
    u8  data[];   // 紧随其后
};

struct VecHeader {
    i64 len;
    i64 cap;
    i64 rc;       // 【已废弃】原预留引用计数
    i64 elem_size;
    u8  data[];
};
```

`rc` 字段现在不用，但**先占位**——这是给内存管理方案留的后路，避免将来改 ABI。

### 4.3 名字修饰（mangling）

重载与泛型单态化都要求确定性的符号名，且**必须跨编译单元稳定**（因为 `rt.o` 是预编译的）。

方案：Itanium 风格的「长度前缀」，避免任何歧义：

```
_R <len><name> <encoded-params...>

例：
_R3souti          →  fun sout(x: int)
_R3soutPKc        →  fun sout(x: str)        K=str, c=const?（此处 K 表示 str）
_R3maxii          →  fun max(a: int, b: int)
_R3maxff          →  fun max(a: float, b: float)
_R7HashMap2Ki5Vec3int
```

类型编码表（冻结）：

| 类型 | 编码 |
|---|---|
| `int` | `i` |
| `float` | `f` |
| `bool` | `b` |
| `char` | `c` |
| `void` | `v` |
| `str` | `K` |
| `list[T]` | `L` + T |
| `dict[K,V]` | `D` + K + V |
| `func[A,B][R]` | `F` + A + B + `_` + R |
| 泛型参数 | `T` + 索引 |
| 用户类 `Foo` | `N` + len + name |

**符号策略在 P11 已修正为「按定义个数分流」**（原文是「唯一签名也修饰」）：
唯一名字保持裸名，只有 ≥2 个定义的名字才修饰。原因是硬约束 —— `lib/rt/rt.o`
是构建期预编译好的、`core_win64.o` 是纯汇编写死的 `core_*` ABI，全部修饰会
让它们全部对不上，运行时整个作废。详见 `docs/p11_plan.md` §1。
重载集修饰时编码的是**形参类型**（未标注编成 `v`），与泛型实例编码**类型实参**
共用同一形状。

### 4.4 `core` 冻结 ABI

`lib/core/core.rio` 是编译器唯一认识的「魔法文件」，全部声明为 `extern`：

```rio
// core.rio —— 编译器内在函数声明（冻结 ABI v1）
// 实现：core_win64.s + core_win64.c
// 任何签名变更都必须升 ABI 版本号

extern fun mem_copy(dst: ptr, src: ptr, n: int): void
extern fun mem_move(dst: ptr, src: ptr, n: int): void
extern fun mem_set(dst: ptr, byte: int, n: int): void
extern fun mem_eq(a: ptr, b: ptr, n: int): bool

extern fun alloc(size: int, align: int): ptr      // 返回 null 表示失败
extern fun realloc(p: ptr, old_size: int, size: int, align: int): ptr
extern fun free(p: ptr): void

extern fun io_write(fd: int, buf: ptr, n: int): int
extern fun io_read(fd: int, buf: ptr, n: int): int

extern fun proc_exit(code: int): void
extern fun proc_abort(): void
extern fun panic_raise(msg: ptr, len: int): void   // 写 stderr 后 abort

extern fun eh_push(frame: ptr): ptr                // 异常帧链，见 §8.2
extern fun eh_pop(frame: ptr): void
extern fun eh_throw(exc: ptr): void

extern fun time_ns(): int
extern fun sys_breakpoint(): void
```

**这份清单必须保持极小**。判断标准：只有「Rio 语言本身无法表达」或「必须直接触碰 OS」的才进来。任何能用 Rio 写出来的，一律下沉到 `rt`。

编译器侧对 `core` 的依赖仅限：解析 `core.rio` 得到签名 + 生成调用 `core_*` 的汇编。**编译器不认识 `rt` 和 `std` 的任何符号**，它们只是普通 Rio 模块。

### 4.5 栈探测

Windows 上大于 4KB 的栈帧需要 `__chkstk` 探测，否则会踩到 guard page。这是**编译器自动插入**的，不暴露给用户：

- 函数序言里若 `frame_size >= 4096`，调用 `core.stack_probe`
- 该函数由 `core` 提供，签名固定

---

## 五、依赖关系

### 5.1 编译期依赖

```
compiler ──解析──► lib/core/core.rio        （必须，ABI 契约）
         ──解析──► lib/rt/rt.rio            （可选，取决于是否用 runtime）
         ──解析──► lib/std/*.rio            （可选，按 import 按需）
```

编译器**不链接**任何 Rio 代码，只生成 `.o` 并驱动链接器。

### 5.2 链接期依赖

```
用户程序.o
   ├── rt.o            （min/full 档位需要）
   ├── core_win64.o    （总是需要）
   └── 平台库：kernel32, msvcrt
```

### 5.3 禁止的依赖

| 禁止 | 原因 |
|---|---|
| `core` 依赖 `rt` | 破坏种子层，自举时死锁 |
| `rt` 依赖 `std` | 破坏分层 |
| 编译器依赖 `rt`/`std` 的符号名 | 会让 stdlib 演进变成 ABI 变更 |
| `rt`/`std` 依赖编译器内部数据结构 | 自举时无法表达 |

---

## 六、按需链接（三档 Runtime）

| 档位 | CLI | 内容 | 场景 |
|---|---|---|---|
| **none** | `--runtime=none` | 仅 `core`，无分配器、无 panic | 裸机 / 嵌入 / 教学 |
| **min** | `--runtime=min` | `core` + `rt`（分配器、panic、IO） | 想要小体积、自带字符串与容器 |
| **full** | `--runtime=full`（默认） | `core` + `rt` + `std` | 日常 |

实现方式：
1. 链接器输入按档位选取对象文件列表
2. `std` 是源码，按 `import` 图**按需编译**（只编译被引用的模块），避免拉进无关代码
3. 提供 `--link-arg` 让用户追加自定义对象

---

## 七、自举路线

目标：将来用 Rio 重写 `rcc`。要为此准备的是**约束**，不是提前写代码。

### 7.1 现在就生效的约束

1. **`rt` 从第一天起就用 Rio 写**，不用 C 写。这样编译器自举时，运行时已经就绪。
2. **编译器内部只用 Rio 能表达的数据结构**：不要用 C++ 模板元编程承载语义逻辑，不要依赖异常（用返回值），不要依赖 RTTI。
3. **`rcc` 拆成 `librcc` + 瘦 driver**，将来 Rio 版 driver 可以链接同一套语义。
4. **`std` 尽早具备编译器自己需要的设施**：`str` 处理、`HashMap`、`list`、文件 IO、格式化。自举的瓶颈从来不是后端，是标准库。

### 7.2 自举里程碑

| 阶段 | 内容 |
|---|---|
| S0 | `rcc`（C++）能编译完整 `rt` + `std` |
| S1 | 用 Rio 写一个 lexer，输出与 C++ 版逐 token 一致（差分测试） |
| S2 | Rio 版 parser，输出 AST 的 JSON 与 C++ 版一致（差分测试） |
| S3 | Rio 版 sema + backend |
| S4 | Rio 版 `rcc` 编译自身，产物与 C++ 版产物二进制等价或行为等价 |

---

## 八、待定问题（需要决策）

### 8.1 内存管理

> **⚠️ 本节已于 2026-11-16 裁决（用户拍板）：以 `docs/memory-model-spec-full.md` 为准。**
>
> 该规范（v0.3，引用按其节号，不按行数）明确把**「垃圾回收、引用计数」列为非目标**（§1），
> 采用 Body（壳子）/ Content（内容）二分 + **出生地** `in local` / `in result`
> + 能力类型 `Ref<T>` / `Mut<T>` / `Take<T>` + `Pool` 稳定地址容器 + `Link` 成对引用的方案。
>
> 因此：
> - **下列三方案表已作废**，仅作历史记录保留。
> - **`ListHeader` / `DictHeader` 的 `rc` 字段（当前恒为 0）标注为废弃**，
>   内存模型落地时一并移除。这影响布局，是 ABI 级决定，需在 P13 单独评审。
> - 规范目前是**设计草案且为脱离 Rio 的伪代码**，需完成 Rio 语法映射后才能作为实现依据
>   （详见 `docs/p12_plan.md §4.3.1`）。

~~三个方向：~~

| 方案（**已作废**） | 优点 | 缺点 |
|---|---|---|
| **A. 作用域自动释放（类 RAII）** | 零运行时开销，符合 Rust 理念 | 需要所有权/借用分析，编译器复杂度高 |
| **B. 引用计数** | 实现简单，header 已预留 `rc` | 循环引用泄漏，每次赋值有原子/非原子开销 |
| **C. 手动 + 显式 `drop`** | 最简单，最快出活 | 不安全，用户体验差 |

~~**建议：先 B（引用计数，非原子），header 已预留 `rc` 字段，不阻塞 ABI。**~~
**（此建议已被 2026-11-16 裁决推翻，见上方说明。）**

### 8.2 异常与 `try/catch`

语言已有 `try/catch/finally/throw`。生成汇编时，实现方式：

| 方案 | 说明 | 工作量 |
|---|---|---|
| **A. 表驱动 unwind（Rust/Itanium 风格）** | 需要 `.pdata`/`.xdata` 与自定义 unwinder | 极大 |
| **B. 显式帧链（setjmp/longjmp 风格）** | `core` 维护 handler 栈，`throw` 时跳转 | 小，`core.eh_*` 已列出 |
| **C. 改为 Result 返回** | 抛弃异常语义 | 改语言 |

**建议：B。** 代价是「析构不保证执行」，需要文档明确；但能让 P7 阶段一周内跑通。

### 8.3 泛型约束（这是最大的设计缺口）

去 `any` 后，`sout(x)` 打印任意类型、`sort(list[T])` 比较任意元素，都需要「T 必须能转字符串 / 能比较」这类**约束**。没有约束，单态化到 `sout<T>` 时无法生成代码。

三个方向：

| 方案 | 说明 | 与现有语言的关系 |
|---|---|---|
| **A. 引入 interface / trait** | `fun sout<T: Display>(x: T)`，字典传递 | 语言已有 `interface` 标签（`DESCRIBE_LABELS`），可自然延伸 |
| **B. 隐式协议（编译器约定）** | 编译器要求每个类型有 `__fmt_T` 符号，单态化时自动查找 | 不需要新语法，但不可扩展、不透明 |
| **C. 限定泛型只能用于容器** | `list[T]` 可以，但 `sout<T>` 不行，打印只走重载 | 最省事，但表达力弱，`ds.rio` 里大量 `func[any]` 回调无法迁移 |
| **D. 单态化即约束（无接口系统）** | 定义处不检查；每个实例化点生成一份具体代码，检查发生在实例化处 | 不需要新语法；实例就是普通函数，只是名字带 mangling 后缀 |

**实际决策（P10 已按此实现）：D。**

理由不是「最省事」，而是编译模型决定了 A 无处安放：后端直接生成 x86-64 汇编，
没有 IR、没有独立 sema pass，不存在一个「约束求解器」该待的地方。而单态化
让约束变得不必要 —— `sout[T]` 在 `sout[int]` 和 `sout[str]` 两处各生成一份
具体代码，每份里 `T` 都是已知的具体类型。

这同时避免了 A 的工期问题：不需要接口语法、约束求解、字典传递，
也不需要为泛型单开一条代码生成路径（实例复用普通函数的全部机制，
包括寄存器分配、int↔float 转换、可赋值性校验、命名实参）。

代价是**检查只到实例化处、不进函数体**：`fun f[T](x: T)` 里对 `x` 做的操作
要等具体实例才暴露，且实例化处的检查只覆盖「形参 vs 实参」，不覆盖函数体内部。
详见 `docs/p10_plan.md` §7。

（原建议 A 的保留意见：若将来 `ds.rio` 的 `func[any]` 回调迁移出现无法用
单态化表达的用例，再回到 A。目前 legacy 里没有这样的用例。）

### 8.4 旧 `-s` 符号导出怎么办

决策已定：`CompileVisitor` 暂时保留给 `-s`。但这意味着**新旧两套符号系统并存**直到 P12。需要接受这段技术债，或者把 `-s` 也纳入新 sema（推荐后者，但放到 P9 之后）。

---

## 九、实施路线

### 9.1 阶段划分

每阶段结束都必须有**可运行的产物**和**通过的回归测试**，不做大爆炸式改造。

| 阶段 | 内容 | 产出验收 |
|---|---|---|
| **P0** 骨架 | CMake 去 LLVM；删 llvm visitor 与 RI；建 `compiler/` `lib/core` `lib/rt` `lib/std` 目录；CLI 加 `rcc build`；`core.rio` 声明 + `core_win64.s` 最小实现 | 手写 Rio 程序能编出 exe |
| **P1** 值表示 | `rt.rio` 的分配器、`str`/`list` 底层、`sout` 打印 | test_1、test_3 通过 |
| **P2** 表达式 | 局部变量、算术、比较、逻辑、复合赋值、类型推导 | test_2、test_4、test_13 通过 |
| **P3** 控制流 | if/elif/else、while/until/for、break/continue/ret | test_5 通过 |
| **P4** 函数 | 定义、递归、匿名函数、闭包、默认参数、命名参数 | test_6/7/9/10 通过 |
| **P5** 容器 | list/dict 字面量与索引 | test_8 通过 |
| **P6** 类 | class/ctor/字段/方法/继承/super | test_16/17 通过 |
| **P7** 异常 | try/catch/finally/throw（方案 B） | test_11/12 通过 |
| **P8** 模块 | 编译期读入被导入的 `.rio`，合并成单个 `ProgramNode`；`import x` 与 `import(alias="x")` 两种写法；依赖先于使用方内联、菱形去重、环检测 | test_p8_modules 通过 |
| **P9** 静态化 | 类型标注权威化（写了标注就按标注分配/转换/校验，未标注继续推断）；移除隐式 int 兜底；5 处类型错误诊断（变量声明/赋值/实参/返回值/未知类型名），诊断带 `file:line:col` | `test_p9_types`（20 例）通过 + 全量回归不回退 |
| **P10** 泛型 | `[]` 语法、单态化（`_R<len><name><encoded-params>` mangling）、实例缓存、类型实参推导（显式 + 结构推导）；决策见 §8.3 方案 D「单态化即约束」 | `test_p10_generics`（22 例）通过 + 全量回归不回退 |
| **P11** 重载 | 重载决议 + 按定义个数分流的 mangling（单定义保裸名，≥2 定义才修饰）；`_R<len><name><形参编码>` 编码形参类型（未标注编成 `v`）；预登记解决前向调用；歧义/重复定义/无候选三类诊断 | `test_p11_overload`（18 例）通过 + 全量回归不回退 |
| **P12** 收尾 | 删旧 RA 路径；重写 run.bat/test.bat；文档；发版 | 仓库干净 |

**P0–P8 = 追平现有能力；P9–P11 = 语言升级。**
顺序上先做端到端竖切（P0 就出 exe），避免陷在语义层里看不见进展。

进度（2026-11-17）：P0 ✅、P1 ✅、P2 ✅、P3 ✅、P4 ✅、P5 ✅、P6 ✅、P7 ✅、P8 ✅、P9 ✅、P10 ✅、P11 ✅、**P12 ✅**。
P1/P2/P3 的内容（值表示、表达式、控制流）已在 P1 一次做齐，
阶段记录见 `docs/p1_plan.md`；P4 见 `docs/p2_plan.md`；P5 见 `docs/p5_plan.md`；
P6 见 `docs/p6_plan.md`；P7 见 `docs/p7_plan.md`；P8 见 `docs/p8_plan.md`；P9 见 `docs/p9_plan.md`；P10 见 `docs/p10_plan.md`；P11 见 `docs/p11_plan.md`；P12 见 `docs/p12_plan.md`。
当前 legacy 17 个测试通过 15 个（test_1–13、16、17）。
`test_14_iter` / `test_15_forinrange` 是 G6（闭包写回）延期的已知失败，非回归。
test_14_iter / test_15_forinrange 仍然不通过，但**不再是 P8 的门槛**：
它们依赖旧 LLVM 时代的 `release/Lib/builtin/ds.rio`（`encapsulated` + RA 指令），
属于 P12「重写标准库」的范围。P8 的门槛改为 `tests/agent/p8-modules/`（13 个用例）。

### 9.2 依赖关系

```
P0 ──► P1 ──► P2 ──► P3 ──► P4 ──► P5 ──► P6 ──► P7 ──► P8
                                                      │
                                                      ▼
                                            P9 ──► P10 ──► P11 ──► P12
```

P9 依赖 P8 完成（否则库里大量 `any` 无法迁移）。
P10/P11 依赖 P9（类型系统定型后才能做约束与重载）。
P12 依赖全部。

P9 的实际做法见 `docs/p9_plan.md`：**「注解必填」降级为「标注权威 + 推断兜底」**。
原因是 legacy 17 个用例与 `rt.rio`/`core.rio` 全部无标注，而 `rt.rio` 由构建期
自举编译 —— 强制必填会先让构建挂掉。P9 拿到的是「已有标注第一次真正生效」
加上 5 处类型错误诊断，这才是这一阶段真正缺的东西。

P10 的实际做法见 `docs/p10_plan.md`：**§8.3 的三方案之外选了「单态化即约束」**。
定义处不检查，检查发生在实例化处；实例就是普通函数（mangling 后缀区分），
复用寄存器分配 / int↔float 转换 / 可赋值性校验 / 命名实参全部既有机制。
P10 顺带修掉三个被「一律 int」掩盖的 P9 遗留缺陷：`list[float]` 被当 int、
具名函数当函数值传参时类型是 int、方法泛型返回类型在调用点退化成 int。

P11 的实际做法见 `docs/p11_plan.md`：**§4.3 的「全部函数都修饰」被改成「按定义个数分流」**。
唯一名字保持裸名（`rt.o` / `core_win64.o` 的 ABI 一字不动，自举不受影响），
只有 ≥2 个定义的名字才修饰成 `_R<len><name><形参编码>`，编码形参类型
（未标注编成 `v`，**不回退 int**，否则 `f(x)` 与 `f(x: int)` 会撞车）。
「未标注形参 = 通配符」是 P9「标注权威 + 推断兜底」在重载上的延续。

### 9.3 风险清单

| 风险 | 影响 | 缓解 |
|---|---|---|
| 泛型约束机制未定（§8.3） | 阻塞 P10–P11，工期不可估 | **已决策**：P10 选方案 D「单态化即约束」（见 §8.3），无接口系统；P11 重载不依赖约束机制 |
| `rt` 用 Rio 写，但 Rio 还没有容器 → 鸡生蛋 | 阻塞 P1 | `rt` 的容器用「裸指针 + 手写内存布局」绕过，不依赖 `std` |
| `sout` 需要泛型打印 | 阻塞 P1 的 test_1 | P1 先用「多类型参数 + 运行时按类型分派」顶上；P11 的**重载**才是正统解法，但 `sout` 走编译器特化路径（不进重载表），标准库版本留给 P12 |
| 现有 17 个测试是动态语义写成的 | P9 后大量测试需要改 | P9 实际选了「未标注继续推断」，因此 15/17 无需改动；剩余 2 个属 P12 |
| 新旧符号系统并存 | 维护成本 | P12 强制收口 |

---

## 十、下一步建议

1. **§8.3（泛型约束）已决策**：P10 选「单态化即约束」，无接口系统。
   若将来 `ds.rio` 的 `func[any]` 回调迁移出现无法用单态化表达的用例，再回到方案 A。
2. 决策 §8.1（内存管理）与 §8.2（异常），两者都影响 ABI，越早定越好。
3. **P11 已完成**（重载 + mangling）：符号按定义个数分流，唯一名字保裸名，
   重载集才修饰；决议在调用点按实参静态类型做（见 `docs/p11_plan.md`）。
   方法重载仍未支持（`registerClassBody` 一个方法名一份定义），属 P12 范围。
4. 下一步是 **P12（收尾）**：删旧 RA 路径、重写 `run.bat`/`test.bat`、
   重写标准库（`ds.rio` 的 `sout` 重复定义与 RA 迭代器）、发版。

---

## 十一、测试体系（现行）

旧的 `tests/test_1_*` … `test_17_*` 十七个目录 + `test_scripts/*.ps1` 二十四套脚本
已经全部移除。它们的问题是三重的：**判定逻辑有分歧**（`fc /W` 与逐字节比对对同一份
输出可能给出不同结论）、**运行器靠硬编码清单**（每加一个套件都要改 `test_all.ps1`）、
以及**用例不是数据**（想看某个用例覆盖了什么，只能读 PowerShell 源码）。

现行体系用「测试集」作为唯一单位。每个测试集是一个自包含目录：

```
tests/
├── new-test.ps1          # 生成测试集模板（参数式 / 引导式）
├── validate.ps1          # 按契约校验所有测试集
├── cases/                # A：用户提交的可复现问题（由 pack.ps1 进来）
├── agent/                # C：Agent 维护的测试集
│   └── _shared/driver.ps1   # 数据驱动套件的唯一判定逻辑
└── user/                 # B：用户自由区，Agent 不动
    ├── pack.ps1          # user → cases/<name>
    └── run-all.ps1       # 扫描式运行器
```

### 11.1 测试集契约

```
<name>/
├── manifest.json     # name/version/type/entry/timeoutSec/expected
├── README.md
├── code/             # 被测代码
├── execute.ps1       # 唯一入口：构建 → 运行 → 判定（三阶段合一）
├── input/
└── expected/{output.txt, exit-code.txt}
```

**没有 `setup.ps1`**：环境准备并入 `execute.ps1` 的构建阶段，避免「两个脚本谁先跑」
这种顺序问题。

### 11.2 三种测试集的分工

| 区域 | 谁写 | 谁校验 | 说明 |
|---|---|---|---|
| `cases/` | 用户（经 `pack.ps1`） | `validate.ps1` | 用户报告的可复现问题 |
| `agent/` | Agent | `validate.ps1` | 回归套件，Agent 自己维护 |
| `user/` | 用户 | **不校验** | 自由实验区，Agent 不得改动 |

`user/` 刻意不纳入校验：它是给人随手试的地方，如果也要求符合契约，
就等于逼用户先学会契约才能试代码。

### 11.3 数据驱动

`agent/` 下 17 个套件把用例表放进 `code/cases.json`，由 `_shared/driver.ps1`
统一执行。字段：`id`、`file`（或 `main`+`files`）、`expect`、`matchMode`、
`exitCode`、`flags`、`expectFailure`、`mustMatch`（+`mustMatchLiteral`/`mustMatchRegex`）、
`compileOnly`。

`flags` 是数组，原样放在 `-build` 之前，用于传单横线开关（目前只有
`-allow-cond-destroy` 需要）。不写等于不传。

三种片段匹配语义被**刻意保留**，因为它们对应老套件里真实存在的三种写法：
`$out.Contains()`（字面）、`-match`（正则）、`-like "*x*"`（通配符）。
统一成一种会静默放宽或收紧判定。

### 11.4 运行器

`tests/user/run-all.ps1` 是**扫描式**的：遍历 `tests/{agent,cases}` 下每个带
`execute.ps1` 的目录并执行，汇总成一张表。新增测试集不需要改运行器 ——
这是与旧 `test_all.ps1` 最关键的差别。

```bat
scripts\test.bat                      :: agent + cases
scripts\test.bat -Area agent          :: 只跑 agent
scripts\test.bat -Filter p13          :: 名字过滤
scripts\test.bat -StopOnFail -KeepLog
```

### 11.5 两个 PowerShell 5.1 的坑（已在 driver 里处理）

1. **原生 stderr 会被控制台宽度硬折行**。`& $exe ... 2>&1 | Out-String` 默认按
   120 列折行，一条诊断可能从词中间被劈开（`instance-level` → `instance-l` +
   `evel`），导致期望片段匹配失败，而且**是否触发取决于临时目录路径长度**。
   解法：`Out-String -Width 10000`。
2. **`$ErrorActionPreference='Stop'` 会把原生命令的 stderr 变成终止错误**。
   预期失败的用例必须临时降级为 `Continue`，否则整个套件在第一例就崩。
