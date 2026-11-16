# P12-1 旧实现删除前的依赖审计（A1–A7）

> 对应 `docs/p12_plan.md` §3.2。本文件是**删除前**的审计记录 + **删除后**的核对结论。
> 审计日期：2026-11-16。基线版本：`9a1729c`（P0–P11 重写完成）。

## 0. 被删除的范围

`CMakeLists.txt` 的 `RCC_LEGACY_SOURCES` 原有 18 个 `.cpp`，约 8.6k 行：

```
code/src/visitors/rcc_compile_visitor.cpp                  (3922 行，最大头)
code/src/visitors/rcc_collect_symbol_visitor.cpp
code/src/visitors/rcc_print_visitor.cpp
code/src/visitors/rcc_json_visitor.cpp
code/src/visitors/tools/rcc_function_call_compiler.cpp
code/src/components/symbol/rcc_symbol.cpp
code/src/components/ri/rcc_ri.cpp
code/src/builtin/rcc_builtin.cpp
code/src/builtin/rcc_builtin_core.cpp
code/src/builtin/functions/rcc_builtin_import.cpp
code/src/builtin/functions/rcc_builtin_export.cpp
code/src/builtin/functions/rcc_builtin_io_functions.cpp
code/src/builtin/functions/rcc_builtin_ds_functions.cpp
code/src/builtin/functions/rcc_builtin_prog_functions.cpp
code/src/builtin/functions/rcc_builtin_datatype_functions.cpp
code/src/builtin/functions/rcc_builtin_lowlevel_functions.cpp
code/src/builtin/functions/rcc_builtin_super.cpp
code/src/builtin/dll_extension_manager/rcc_dll_extension_manager.cpp
```

删除过程中**追加**确认删除的骨架（原计划未列全，见 A2）：

- `code/include/visitors/` 整个目录（含 `rcc_base_visitor.h`：51 个纯虚函数）
- AST 里 47 处 `acceptVisitor` 声明 + 46 处实现
- `code/include/components/`、`code/include/builtin/`
- `code/declarations/builtin/`、`code/declarations/components/`
- `code/include/interfaces/` 整个目录、`code/include/internal/`
- `code/declarations/analyzer/rcc_ast_dec.h`

---

## A1 · 被删类/函数/头文件的所有引用

**命令**

```powershell
git grep -n "CompileVisitor\|SymbolTable\|CollectSymbolVisitor\|PrintVisitor\|JsonVisitor\|rcc_ri\|BuiltinManager"
```

**删除前**：引用集中在 18 个待删 `.cpp` 与 `code/include/visitors/`、`code/include/components/`、
`code/include/builtin/`、`code/declarations/builtin/**` 内部，形成闭环。

**删除后结论**：✅ 无有效残留。唯一命中是一句注释：

```
code/src/analyzer/parser/rcc_parser.cpp:14: // 这是历史约定，CompileVisitor 依赖它；新代码请用 !first。
```

已改写为不再引用已删类的表述（保留「first = 是否出错」这个反直觉约定的说明）。

---

## A2 · 头文件前置声明与接口依赖

**审计发现**：删除 18 个 `.cpp` 后编译仍会失败，因为大量**前置声明和基类骨架本身**
还活着，只是没人用了。三类：

1. **Visitor 基类**：`rcc_base_visitor.h` 有 51 个纯虚 `visitXxx`，AST 每个节点类都有
   对应的 `acceptVisitor(Visitor&)`（47 处声明 + 46 处实现）。它们**只**为已删除的
   visitor 存在。
2. **C ABI 接口**：`code/include/interfaces/` 的 `IRCCObjectInterface` / `IRCCPosInterface` /
   `IRCCCompileInterface` / `IRCCSymbolInterface` / `IRCCBuiltinInterface`，是给旧 DLL 扩展
   （`lowlevel.bindDllExt`）跨 DLL 传对象用的。`bindDllExt` 已随 S1 删除。
3. **悬空的前置声明**：`rcc_ast_dec.h`（含 `class Visitor; class CompileVisitor;` 等）、
   `rcc_builtin_interface.h` 的 `compileVisitorI` 字段。

**额外发现（值得记录）**：`IRCCObjectInterface` 那批 `const char* ToString()` /
`BriefString()` / `ToJsonString()` 等重载，实现是 `return toString().c_str();` ——
返回**临时 `std::string` 的裸指针**，调用即悬垂。删除它们顺带消掉了一个真实缺陷。

**删除后结论**：✅ 全部删除。两处需要补回被连带断开的依赖：

| 断掉的依赖 | 原因 | 处理 |
|---|---|---|
| `LabelNode` / `ListExpressionNode` 前置声明 | 原本由 `rcc_base_visitor.h` 的 `rcc_ast_dec.h` 间接带入 | 在 `rcc_ast_components.h` 顶部显式前置声明 |
| `getNodeTypeName` / `getNodeTypeFormatName` 声明 | 原本在 `rcc_ast_dec.h` | 移入 `rcc_ast.h`（它们本就属于 `NodeType`） |

---

## A3 · CMake 源列表与链接依赖

**命令**

```powershell
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -C build -j 10
```

**结论**：✅ `RCC_LEGACY_SOURCES` 整块清空后：

1. 先只清空源列表、**保留**文件 → 构建通过（证明没有隐藏的链接期依赖）
2. 再删除 18 个文件 → 构建通过
3. 删除 visitor/interface 骨架 → 构建通过

`target_link_libraries` 无需改动（旧实现没有引入额外链接库）。

---

## A4 · `rcc_main.cpp` 对旧实现的引用

**审计发现**：`-s` 原本**完全**依赖 `ast::CompileVisitor` 的静态成员
（`__symbol_flag__`、`__symbol_flag_export__`、`__symbol_option_format__`）；
另有 `-c` / `--compile` / `--compile-level` 与 `-ir` 分支。

**处理**：

- `-s` 改接到新的 `analyzer::collectSymbols` / `renderSymbols`（见 §3.3 / §3.4），
  静态成员全部移除
- `-c` / `--compile` / `--compile-level` **删除**（旧 RA 路径专属，新后端没有对应概念）
- `-ir` 分支删除；`handleLlvmFlag()` 删除
- 移除 `.addDependent("format", "output")`（六种格式都可流式输出到 stdout）
- 移除 `.addMutuallyExclusive("spec-symbol", "fmt")`

**结论**：✅ 无残留。

---

## A5 · CLI 参数与脚本对旧实现的依赖

**审计发现**：

| 位置 | 依赖 | 状态 |
|---|---|---|
| `scripts/run.bat` | `LLC_EXE=...llc.exe`、`%INPUT_LL%`、`-ir`、`.ll`、`.obj` | ⬜ **P12-4 处理** |
| `scripts/test.bat` | 同上 | ⬜ **P12-4 处理** |
| `test_scripts/*.ps1` | 已全部改为 `-build` | ✅ |
| `tests/test_*/build.log` | 入库的历史日志，含 `.ll` 路径 | ⬜ 见 A7 |

**结论**：⚠️ 部分延后。`run.bat` / `test.bat` 的重写属 P12-4（与标准库重写一起做，
因为它们还要指向新的 `lib/std`），本轮不动。

---

## A6 · 测试脚本是否调用旧命令/旧产物

**命令**：`powershell -File test_scripts/test_legacy_all.ps1`

**删除后结果**：**15 通过 / 2 失败**

```
失败用例：test_14_iter, test_15_forinrange
```

两者失败原因均为 `Cannot determine the object type of attribute access`，
是 P8 阶段就已知的 codegen 缺口 **G6**，**已获批准延期到 P13**。
删除前后失败集合完全一致 → ✅ 本次删除**没有引入回归**。

`test_p0`–`test_p11`：**147 例全绿，0 失败**。

---

## A7 · 最终 exe 是否仍依赖旧对象文件

**命令**

```powershell
git grep -il "llvm" -- ":!docs/*" ":!logs/*"
```

**结论**：⚠️ **有输出，但全部可解释**。逐条分类：

| 命中 | 性质 | 处置 |
|---|---|---|
| `.clang-format:4` `BasedOnStyle: LLVM` | 代码格式化风格名，与编译器后端无关 | **保留**（不是残留） |
| `CMakeLists.txt:139` `Backend: x86-64 GAS assembly (no LLVM)` | 构建期状态输出，明确写着 no LLVM | **保留**（不是残留） |
| `.idea/customTargets.xml` | IDE 个人配置，未入库语义 | 忽略 |
| `scripts/run.bat`（6 处） | 真实的 LLVM 残留（`llc.exe` / `-ir` / `.ll`） | ⬜ **P12-4 重写** |
| `test_scripts/test_p0.ps1:5` | 注释「不依赖 LLVM，只用 GNU as / ld」 | **保留**（说明性） |
| `test_scripts/test_p8_modules.ps1:10` | 注释「它们依赖旧 LLVM 时代的 release/Lib/builtin/ds.rio」 | **保留**（说明性） |
| `tests/test_*/build.log`（12 个） | 入库的历史构建日志，记录当时的 `.ll` 路径 | ⬜ 见下 |
| `tests/test_13_compound_assign/test.rio:1`<br>`tests/test_14_iter/test.rio:1`<br>`tests/test_15_forinrange/test.rio:1` | 测试源码第 1 行注释 `// Test ... in LLVM IR backend` | ⬜ **不能改**（见下） |

### ⚠️ A7 的验收标准本身有冲突

原文通过标准是「`git grep -il "llvm"` 无输出」。这个标准**不可达**，且理由正当：

1. `.clang-format` 的 `BasedOnStyle: LLVM` 是格式化器风格名，与后端无关，删了会改变
   全仓库代码风格。
2. `CMakeLists.txt` 那行是**主动声明**「no LLVM」，删掉反而丢失信息。
3. `tests/test_13/14/15/test.rio` 第 1 行的注释属于**冻结的测试输入**——
   用户已明确要求 `tests/test_14_iter` 和 `tests/test_15_forinrange` 的
   `test.rio` / `expected.txt` **保持不动**（不得为了让测试变绿而改期望）。
   改这一行注释等于改动冻结输入。

**修正后的通过标准**（本文件采用）：*不存在指向 LLVM 的**功能性**依赖* ——
即没有任何构建/运行路径去调用 `llc.exe`、生成或读取 `.ll`/`.obj`。
按此标准：唯一的真实功能性残留是 `scripts/run.bat`，已登记到 P12-4。

`tests/test_*/build.log` 是历史日志，属仓库卫生问题而非功能依赖；
在 P12-5 更新 `logs/update_logs.md` 时一并处理（重新生成或从索引中移除）。

---

## 汇总

| 审计项 | 结论 | 遗留 |
|---|---|---|
| A1 引用残留 | ✅ 通过 | 一句注释已改写 |
| A2 前置声明/接口 | ✅ 通过（范围扩大：连带删掉 Visitor 骨架与整套 C ABI） | 无 |
| A3 CMake/链接 | ✅ 通过 | 无 |
| A4 `rcc_main.cpp` | ✅ 通过 | 无 |
| A5 CLI/脚本 | ⚠️ 部分 | `run.bat` / `test.bat` → P12-4 |
| A6 测试脚本 | ✅ 通过（15/2，与删除前一致；147 例 P 系列全绿） | `test_14`/`test_15` → P13（G6） |
| A7 exe 依赖 | ✅ 通过（按修正后的标准） | 原「无 llvm 字样」标准不可达，已说明理由 |

**A1–A7 全部有结论，无「无法解释的旧实现残留」。P12-1 可以收尾。**
