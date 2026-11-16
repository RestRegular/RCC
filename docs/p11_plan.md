# P11 —— 重载 + mangling

> 目标：让同名不同签名的函数（`fun f(x: int)` / `fun f(x: str)`）能共存，
> 调用点按实参的静态类型决议到正确的那一份。
> 验收：`test_scripts/test_p11_overload.ps1`（18 个用例）全绿 + 全量回归不回退。

## 1. 核心决策：符号按定义个数分流

`architecture_plan.md` §4.3 原定的规则是「**所有**函数都用同一套修饰，
保证一致性；需要裸名的地方用 `extern` 标注」。P11 **没有**照做，理由是
硬约束：

`lib/rt/rt.o` 是**构建期预编译好的**，主程序链接时靠符号名对上；
`core_win64.o` 更是纯汇编写死的 `core_*` ABI（§4.4）。如果按 §4.3 原样
把所有函数都修饰成 `_R...`，那么 `rt_print_int`、`core_io_write` 这些
已经存在的符号全部对不上，运行时整个作废 —— 而 `rt.rio` 又必须由 RCC
自己编译（自举回归），改名字等于同时改运行时源码和预编译产物。

所以 P11 采用**按定义个数分流**：

| 名字的定义个数 | 符号 | 说明 |
|---|---|---|
| 1（唯一） | 裸名，例如 `area` | 与 P0–P10 完全一致，`rt.o` / `core_*` ABI 一字不动 |
| ≥2（重载集） | 全部修饰，例如 `_R4areaii` | 只有真的需要区分的名字才付代价 |

这个决策的代价与收益：

| | 全部修饰（§4.3 原文） | 按定义个数分流（P11 实际） |
|---|---|---|
| 运行时 ABI | 全部破坏，需重写 `rt.rio` + `core_win64.s` | 完全不变 |
| 符号可读性 | 调试时满屏 `_R...` | 唯一名字仍是 `area` |
| 一致性 | 规则单一 | 两种形态，要看名字才知道 |
| 跨单元链接 | 需要统一的 `extern` 约定 | 单定义名天然可链接 |

不一致的代价被两件事压住了：
1. 判据是**纯局部的**（这个文件里这个名字有几个定义），不需要跨单元信息；
2. 一个名字要么全裸名、要么全修饰，不存在「一半一半」的中间态。

`extern fun` 在 `core.rio` 里虽然只是文档（`extern` 不在 `KEYWORDS` 里，
词法器根本不认），但 `encapsulated` 是**真实参与编译**的：函数体只有
一个 `encapsulated` 表示「实现由运行时提供」，这类声明必须保持裸名、
且**不计入重载计数** —— 否则测试桩 `fun sout(*args, end="\n")` 一出现，
`sout` 就会被当成重载名修饰掉，链接时就找不到 `rt` 里的实现了。

## 2. 名字修饰（mangling）

沿用 P10 冻结的形状，**只换编码内容**：

```
_R<len><name><encoded-params>
```

| | P10 实例 | P11 重载 |
|---|---|---|
| 编码什么 | **类型实参**（`identity[int]` 里的 `int`） | **形参类型**（`fun f(x: int)` 里的 `int`） |
| 未标注形参 | — | `v` |
| 例 | `identity[int]` → `_R8identityi` | `fun area(w: int, h: int)` → `_R4areaii` |
| 例 | `Box[str]` → `_R3BoxK` | `fun show(x: str)` → `_R4showK` |
| 例 | `Pair[int,str]` → `_R4PairKi` | `fun show(x)` → `_R4showv` |

类型编码表与 P10 完全一致（`i/f/b/c/v/K/P/L/D/F/T0/N<len><name>`），
见 `p10_plan.md` §2。`mangleOverload` 与 `mangleInstance` 是同一个函数
体的两个名字 —— 形状必须保持一致，否则同一个名字既泛型又重载时
（P11 里这种名字不会出现，但形状一致留了余地）会产生两套符号规则。

**关键点：编码用「未标注」的原始形态**。`fun f(x)` 的形参类型在
`FunctionSignature::paramTypes` 里是 `TK_UNKNOWN`，**不是**回退的 `int`
（`_functionParams` 里才回退 int）。如果这里也回退 int，
`fun f(x)` 与 `fun f(x: int)` 会编出同一个符号，重载直接失效。

## 3. 实现结构

改动集中在 `code/src/backend/rcc_codegen.cpp` 与
`code/include/backend/rcc_codegen.h`（P11 块在 552–639 行）。

### 3.1 成员

| 成员 | 作用 |
|---|---|
| `FunctionSignature` | 一份签名的全部信息：名字、符号、形参名/类型/标注掩码/**标注原文**、默认值、返回类型、泛型形参表、`encapsulated` 标志、定义节点指针 |
| `collectSignature(node)` | **纯收集**：不做登记、不报错。因此预登记与正式发射可以各调一次而结果一致 |
| `registerOverloadSignatures(fns)` | 预登记：数每个名字的定义个数，≥2 的才填 `_overloads` / `_overloadedNames` / `_signatures` |
| `mangleOverload(name, paramTypes)` | 形参类型 → `_R<len><name><codes>` |
| `symbolFor(name, paramTypes)` | 唯一名字返回裸名，重载名返回修饰名 |
| `resolveOverload(name, args, &ambiguous)` | 候选过滤 + 打分 + 取唯一最优，返回符号；空串 = 失败 |
| `overloadScore(formal, actual, annotated)` | 单个形参的匹配分：-1 不匹配，越大越精确 |
| `normalizeArgsFor(args, names, defaults, out)` | 按给定签名摊平命名实参、补齐默认值。装不下返回 false |
| `emitResolvedCall(symbol, displayName, args)` | 已决议的调用：摊平 → 按签名走寄存器 → `call` |
| `failOverload(name, args, pos, ambiguous)` | 无候选 / 歧义的诊断，列出全部候选签名 |

### 3.2 为什么必须预登记

函数体是**按文件顺序**发射的。`f1` 的函数体里调用文件后面才定义的 `f2` 时，
`f2` 的签名还没进任何表 —— 而重载决议恰恰需要候选签名才能比较。

所以 `emitProgram()` 在 `registerClass` 之后、`computeLayout` 之前插入
一次 `registerOverloadSignatures(functionNodes)`。位置有两个讲究：

* 在 `registerClass` **之后**：形参标注可能是类名（`fun show(p: Point)`），
  解析成 `ClassType` 要靠类登记表已经填好。
* 在 `emitFunctionDefinition` **之前**：那是唯一需要候选集的时刻。

### 3.3 决议算法

对每个候选签名：

1. **绑定**（`normalizeArgsFor`）：命名实参按名字落位，位置实参顺序填空位，
   空位用默认值补。补不上 → 该候选不合格。
2. **逐个实参打分**（`overloadScore`），任一为 -1 → 该候选不合格：

| 形参 vs 实参 | 分 | 理由 |
|---|---|---|
| 完全相等（`equals`） | 100 | 精确匹配永远最优 |
| 同族数值，同 kind | 90 | `f(int)` 接 int 比 `f(float)` 更贴 |
| 同族数值，跨 kind | 50 | 隐式数值转换，能用但不最优 |
| 指针族互通 | 30 | str/list/dict/class/ptr 都是一个 8 字节指针 |
| 形参未标注 | 1 | **通配符**，什么都能接，但优先级最低 |
| 形参标注了却解析不出来 | 0 | 比通配符更弱：让「拼错类型名」由标注校验去报，而不是报「没有匹配的重载」 |
| 实参是 void / 跨族 | -1 | 不合格 |

3. **取唯一最优**：总分最高者胜。同分且**符号不同** → 歧义，不猜。
   同分但符号相同不算歧义 —— 那是同一签名的重复定义，该由发射阶段的
   `_emittedSymbols` 报重复定义，报歧义会掩盖真正的问题。

「未标注形参是通配符」是 P9 的「标注权威 + 推断兜底」在重载上的延续：
写了标注就按标注匹配，没写就什么都能接。这条规则让 `fun f(x: int)` +
`fun f(x)` 的共存成为可能 —— 前者精确匹配 int，后者接住其余一切，
而不是让未标注的那份被排成「int」而撞车。

### 3.4 三个接入点

| 位置 | 改动 |
|---|---|
| `emitFunctionCall` | 裸名调用前：若名字是重载集，决议 → `emitResolvedCall` → return；否则走原路径 |
| `emitModuleFunctionCall` | 模块内联函数同理：`mod.f(1.5)` 必须和 `f(1.5)` 决议到同一符号 |
| `emitFunctionDefinition` | 重载名的发射符号从 `_overloads` 里按 **def 指针**查（不能用 `_signatures`：同签名的重复定义在那里面只剩先来的那一份）；查不到 = 内部错误；已发过 = 报重复定义 |
| `inferType` 的 FunctionCall 分支 | 重载名不在 `_functionReturns` 里（那是「一名字一签名」的表），所以要先决议一次拿返回类型，否则 `var v = pick(1.25)` 的槽位会按 int 分配，把 double 的位模式当整数存 |

`emitFunctionDefinition` 里查符号必须用 def 指针而不是重算 mangling：
那时 `paramTypes` 已经被「未标注回退 int」改过了，再算一遍会得到
`_R1fi` 而不是 `_R1fv`。

## 4. 与 P10 之前的对照

| 写法 | P11 之前 | P11 之后 |
|---|---|---|
| `fun f(x: int)` + `fun f(x: str)` | 后者覆盖前者（`_functionParams` 一个名字一条） | 两份独立机器码，`f(1)`/`f("a")` 各走各的 |
| 符号 | 都叫 `f` → 汇编器报 symbol already defined | `_R1fi` / `_R1fK` |
| `fun area(w: int, h: int)`（唯一） | `area` | `area`（不变） |
| `rt_print_int` / `core_io_write` | 裸名 | 裸名（不变） |
| `f(1.25)` 里 `f(x: float)` 返回 float | 无法表达 | 返回类型跟着决议走，`2.5` 而不是位模式 |
| 同名不同 arity | 后定义者覆盖 | 按实参个数决议 |
| 同分候选 | 静默取最后一个 | `Ambiguous call to overloaded function 'f' ...` |
| 同一签名写两遍 | 汇编器报 `symbol already defined` | `Duplicate definition of overloaded function 'f' ...` |

## 5. 顺带修掉的两个问题

1. **`generate()` 的状态复位不完整** —— 只清了 `_functionParams` /
   `_functionReturns`，`_functionParamNames` / `_functionDefaults` /
   `_functionParamAnnotated` / `_encapsulatedFunctions` / `_hasUserMain`
   从来没清过。历史上「同一 CodeGen 实例只 generate 一次」所以没暴露，
   但新加的 P11 表如果也不清，第二次 generate 会看到上一次的候选集。
   这次把四张 P11 表显式清掉，并在注释里记下那五张历史遗留表的状态。
2. **诊断要报原始标注文本** —— `FunctionSignature::annotationTexts` 保存
   标注原文，`failOverload` 用它渲染候选列表。否则
   `fun f(x: Piont)`（拼错类型名）在候选列表里会显示成占位类型，
   用户看不出自己拼错了什么。

## 6. 已知限制

- **重载名不能当函数值**：`var g = h` 里的 `h` 若同时是重载集，
  `emitIdentifier` 只认 `_functionReturns[name]`（那是「一名字一签名」的表，
  重载名不在里面），于是 `g` 拿到的是裸名 `h` 的闭包对象，而这个符号
  并不存在（重载集的每一份都叫 `_R...`），调用时按裸名链接会失败或错配。
  正确做法是要么按左值标注决议，要么干脆禁止，属 P12 范围。
- **方法不参与重载**：`registerClassBody` 里 `info.methods[mname] = fn`
  仍然是一个方法名一份定义，同名方法后定义者覆盖前定义者。类的虚表布局、
  方法符号命名（`Class.method`）都需要一起改，属 P12 范围。
  `ds.rio` 里跨类的同名方法（`Iterable.size` / `List.size`）不受影响 ——
  那是不同的 owner 类，符号本来就不同。
- **`ds.rio` 的重复顶层 `sout` 未处理**：`release/Lib/builtin/ds.rio` 里
  顶层 `sout` 定义了两次（165 行、333 行）。按 P11 的规则它会被判成重载集
  并修饰，但那个文件是 LLVM 时代的遗留（依赖 `encapsulated` + RAII 迭代器），
  本来就编译不过，属 P12 范围。
- **决议只看静态类型**：`f(someUnknown)` 里实参类型推不出来时按 0 分算，
  可能选到通配符那份。这是单遍 codegen 的固有局限 —— 没有独立 sema pass
  去把类型彻底算清楚。
- **不做返回值参与的重载**：`fun f(): int` 与 `fun f(): str` 无法共存
  （符号相同，会报重复定义）。这是对的：Rio 没有「按返回值选重载」的语境。
- **重载名不能用 `f[...]` 显式类型实参**：`f[int](1)` 里 `f` 若同时是
  重载集，泛型路径优先。这种名字本身不该存在，没有专门诊断。
- **默认值只影响绑定，不参与 mangling**：`fun f(a: int, b: int = 1)` 与
  `fun f(a: int)` 编出不同符号（形参个数不同），但
  `fun f(a: int = 1)` 与 `fun f(a: int = 2)` 编出同一个符号 → 报重复定义。

## 7. 验收

```powershell
cmake --build build
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p11_overload.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_legacy_all.ps1
```

- `test_p11_overload.ps1`：18 个用例
  - 9 正向（三标量族、arity、返回类型跟随、标注优先于通配符、
    指针族精确优先、类类型形参、重载间互相调用、单定义保持裸名、混合场景）
  - 4 决议（数值族隐式转换、默认值参与候选过滤、命名实参绑定、三选一取最优）
  - 5 反向（无候选、arity 全不匹配、歧义、重复签名、候选列表含标注原文）
- 全量回归（P11 实测）：

  | 套件 | 结果 |
  |---|---|
  | `test_p1_control` | 13 / 0 |
  | `test_p2_functions` | 10 / 0 |
  | `test_p5_containers` | 14 / 0 |
  | `test_p6_classes` | 15 / 0 |
  | `test_p7_exceptions` | 17 / 0 |
  | `test_p8_modules` | 13 / 0 |
  | `test_p9_types` | 20 / 0 |
  | `test_p10_generics` | 22 / 0 |
  | `test_p11_overload` | 18 / 0 |
  | `test_legacy_all` | 15 / 2（`test_14_iter` / `test_15_forinrange` 属 P12） |

- **运行时 ABI 未动的证明**：`nm release/lib/rt/rt.o` 导出的仍是
  `rt_print_int`、`rt_write_str` 等 13 个裸名，一个 `_R` 前缀都没有。
  `rt.rio` 的自举仍是隐含的最强回归：它由构建期自己编译并链接进每个 exe。
