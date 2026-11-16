# P10 —— 泛型（单态化）

> 目标：让 `fun identity[T](x: T): T` 与 `class Box[T]` 这样的泛型写法
> 真的能编译、能运行，且**按实参类型生成各自的机器码**。
> 验收：`test_scripts/test_p10_generics.ps1`（22 个用例）全绿 + 全量回归不回退。

## 1. 核心决策：单态化即约束

`architecture_plan.md` §8.3 给了三条路（接口约束 / 模板约束 / 单态化即约束）。
P10 选**第三条**，理由来自编译模型本身：

后端直接生成 x86-64 汇编，没有 IR，也没有独立的 sema pass。在这种结构下
「约束」这个概念无处安放 —— 没有类型检查器去验证 `T` 是否满足某个接口，
也**不需要**：单态化意味着 `sout[T]` 在 `sout[int]` 和 `sout[str]` 两个
调用点各生成一份具体代码，每份代码里的 `T` 都是已知的具体类型，
检查自然发生在**实例化处**。

所以泛型定义处**不做任何检查**：

```rio
fun identity[T](x: T): T { ret x }   // 一个字都不检查，也不产生机器码
```

只有 `identity[int]` / `identity(3.5)` 这样的具体调用才会：
1. 确定 `T` 的绑定（显式给出或从实参推导）
2. 用绑定把形参/返回类型具体化
3. 生成一份带 mangled 名的实例函数

这个决策的代价与收益：

| | 单态化即约束 | 接口约束（方案 A） |
|---|---|---|
| 编译期开销 | 每个实例一份代码 | 一份代码 |
| 运行期开销 | 无（直接调用，无装箱） | 虚调用 / 字典传参 |
| 表达能力 | 只要实例化时类型合法就能用 | 需要预先声明接口 |
| 需要的新机制 | 类型形参 + 推导 + 单态化 | 接口系统 + 约束求解 + 字典传递 |

选第三条等于「不引入新概念」：不需要接口语法、不需要约束求解、
不需要为泛型单开一条代码生成路径 —— 实例就是普通函数，只是名字带后缀。

## 2. 名字修饰（mangling）

`architecture_plan.md` §4.3 定下的 Itanium 风格编码，P10 原样实现：

```
_R<len><name><encoded-params>
```

| 类型 | 编码 |
|---|---|
| `int` | `i` |
| `float` | `f` |
| `bool` | `b` |
| `char` | `c` |
| `void` | `v` |
| `str` | `K` |
| `ptr` | `P` |
| `list` | `L` |
| `dict` | `D` |
| `func` | `F` |
| 泛型形参 | `T0` |
| 类 `N` | `N<len><name>` |

例：`identity[int]` → `_R8identityi`，`Box[str]` → `_R3BoxK`，
`Pair[int,str]` → `_R4PairKi`。

**必须跨编译单元稳定**：`lib/rt/rt.o` 是构建期预编译好的，
主程序链接时靠符号名对上。任何编码改动都是 ABI 破坏。

## 3. 实现结构

改动集中在 `code/src/backend/rcc_codegen.cpp`（类型系统活在 codegen 里）
与 `code/include/sema/rcc_sema_types.h`（新增 `TypeKind::TK_GENERIC`
和 `GenericType`）。

### 3.1 泛型函数

| 成员 | 作用 |
|---|---|
| `GenericFunction` | 登记一个泛型声明：名字、类型形参表、定义节点、形参/返回类型（含 `GenericType`）、形参名 |
| `collectGenericParams(callNode)` | 从 `fun identity[T](...)` 的 callNode 里取出 `[T]` |
| `explicitTypeArgs(callee)` | 从调用点的 callee 里取出 `[int]` 这样的显式实参 |
| `deduceTypeArgs(...)` | 显式实参优先，其余从「形参类型 vs 实参类型」的结构对应里推 |
| `substituteGenerics(t, bindings)` | 把类型里的 `GenericType(T)` 换成具体类型 |
| `instantiateGeneric(name, bindings)` | 生成 mangled 名、具体化签名、登记进 `_functionParams` 等表、把函数体塞进 `_pendingInstances` |
| `emitPendingInstances()` | 统一发射队列里的实例体 |
| `tryEmitGenericCall(...)` | 调用点的入口：推导 + 实例化 + 发一条 `call` |

关键点：**泛型声明在 `emitFunctionDefinition` 里登记后立刻 return，不发射**。
所以没用到的泛型是零成本（`pos_unused_generic` 验证了这点）。

实例体**不能**在调用点就地发射 —— 调用点的栈帧正被使用，就地插入会破坏它。
所以先排队（`_pendingInstances`），等当前函数发完再统一发
（`emitPendingInstances()` 在 `generate()` 里、`emitProgram()` 之后调用）。
队列用游标推进而不是一次遍历，因为发射实例 A 时可能又发现实例 B。

实例登记进 `_functionParams` / `_functionParamNames` / `_functionReturns` /
`_functionParamAnnotated` 这四张**与普通函数共用**的表，所以寄存器分配、
int↔float 转换、可赋值性校验、命名实参/默认实参全都复用现成机制，
不需要为泛型开第二条路径。

### 3.2 泛型类

`class Box[T]` 在 `registerClass` 里**不登记为可构造的类**，只登记成模板
（`GenericClass`）。真正登记发生在 `Box[int]` 被用到的时候
（`ensureClassInstance`），此时用模板再走一遍 `registerClassBody`，
类名换成 mangled 名，`T` 靠 `_currentGenericBindings` 解析成具体类型。

实例体在 `emitGenericClassBodies()` 里统一发（`generate()` 中、
`emitProgram()` 之后）。**必须在顶层语句发射之后**：实例常常是在顶层
`var b = Box[int](42)` 里才第一次出现的。

### 3.3 类型解析的统一入口

P9 遗留的一个真实缺陷在这次一并修掉：`parseTypeName` 硬编码
`list`→`list[int]`、`dict`→`dict[str,int]`，完全忽略 `LabelNode` 的
`labelDesNodes`。所以 `var xs: list[float] = [1.5, 2.5]; sout(xs[0])`
打印的是 double 的位模式（`4609434218613702656`）。

根因是 `LabelNode::getFullLabel()` **只拼 `labelPath`、从不渲染
`labelDesNodes`** —— `list[float]` 在它眼里就是 `"list"`。所有 `[...]`
里的类型实参只存在于 `labelDesNodes`（包在 `ListExpressionNode` 里）。

新增的统一入口：

| 成员 | 作用 |
|---|---|
| `flattenTypeArgs(node, out)` | 把 `int, str` 这样的逗号序列摊平成列表 |
| `typeFromTypeArgNode(node, genericParams)` | 从单个 AST 节点解析类型（可能是 `LabelNode`、`IndexExpressionNode`、`IdentifierNode`） |
| `typeFromLabelNode(label, genericParams)` | 从 `LabelNode` 解析：先取 `labelPath` 作基名，再看 `labelDesNodes` 作类型实参 |
| `typeFromLabelList(labels, genericParams)` | 从标签列表解析（多个标签取第一个能解析的） |
| `typeArgsOf(node)` | 从 `Box[int]` 这样的节点取出 `[int]` |

`typeFromLabelNode` 解析类型形参时分两级：先查 `_currentGenericBindings`
（实例化上下文中 `T` → `int`），查不到才返回 `GenericType(T)`
（声明上下文中，尚未绑定）。

## 4. 类型推导（`deduceTypeArgs`）

两条路径：

1. **显式类型实参**：`identity[int](42)` —— 直接绑定，个数必须精确匹配。
   少给一个不能靠推导悄悄补上，否则 `pair[int](1, "x")` 这种写错个数的
   代码永远发现不了。
2. **结构推导**：形参类型与实参类型按结构对齐，遇到 `GenericType` 就绑定。

支持的结构：

| 形参类型 | 实参类型 | 推导 |
|---|---|---|
| `T` | 任意 | `T` = 实参类型 |
| `list[T]` | `list[float]` | `T` = `float` |
| `dict[K,V]` | `dict[str,float]` | `K` = `str`，`V` = `float` |
| `funi[A][B]` | 具名函数 | `A`/`B` 从函数签名推 |

`funi[A][B]` 那条有个坑：`inferType` 对标识符只返回变量槽类型，
取不到函数签名。所以当形参是 `TK_FUNC` 时，若实参是具名函数标识符，
就从 `_functionParams` / `_functionReturns` 里把签名补出来
（`map2(addOne, 41)` 靠这个）。

## 5. 顺带修掉的三个 P9 遗留缺陷

单态化把这些原本被「一律 int」掩盖的问题暴露出来了，一并修掉：

1. **`list[float]` / `dict[str,float]` 被当成 int** —— 见 §3.3。
2. **具名函数当函数值传参时类型是 int** —— `inferType` 的 Identifier 分支
   只查变量槽，查不到就返回 `getIntType()`。现在若名字是已知函数，
   返回 `FunctionType`。同时 `sameValueFamily` 把 `TK_FUNC` 归入指针族
   （闭包对象指针本来就是 8 字节指针），否则 `apply(addOne, 41)` 会被
   「cannot assign int to fun(int): int」挡下来。
3. **方法的泛型返回类型在调用点解析不出来** —— `methodReturnType` 用
   `resolveTypeName(getFullLabel())`，`T` 解析不出来就静默退化成 int，
   `Box[float].get()` 打印出 double 位模式。现在走 `typeFromLabelList`
   并按 owner 类名恢复 `_currentGenericBindings`（调用点可能在任何地方，
   那时的绑定上下文早已恢复成空）。

## 6. 与 P9 之前行为的对照

| 写法 | P10 之前 | P10 之后 |
|---|---|---|
| `fun identity[T](x: T): T` | `Cannot determine function name` | 登记为泛型，不发射 |
| `identity[int](42)` | `undefined reference to 'identity'` | 生成 `_R8identityi`，输出 `42` |
| `identity(3.5)` | `undefined reference to 'identity'` | 推导 `T=float`，输出 `3.5` |
| `first(ys)` 其中 `ys: list[float]` | `Cannot deduce type arguments` | 推导 `T=float`，输出 `1.5` |
| `class Box[T]` | `Unknown type 'Box'` | 按需单态化 |
| `var xs: list[float]` + `sout(xs[0])` | `4609434218613702656` | `1.5` |
| `apply(addOne, 41)` | `cannot assign int to fun(int): int` | `42` |

## 7. 已知限制

- **没有约束**：`fun f[T](x: T)` 里对 `x` 做的任何操作都要等实例化才知道
  合不合法。`f[int](1)` 里 `x + 1` 能过，`f[str]("a")` 里 `x + 1` 也「能过」
  （运行时把指针当整数加）。这是单态化即约束的固有代价 —— 检查在实例化处，
  而实例化处的检查只覆盖「形参类型 vs 实参类型」，不覆盖函数体内部。
- **不做显式特化**：没有 `fun identity[int](x)` 这种为特定类型写不同实现的语法。
- **泛型不参与继承**：`class Derived[T]: Base[T]` 未实现，基类名仍按
  `getFullLabel()` 取（`[...]` 会被丢掉）。
- **类型实参只能是类型**：`Box[list[int]]` 支持（嵌套解析走 `typeFromLabelNode`），
  但 `Box[3]` 这种非类型实参不报错也不生效。
- **`_functionParamAnnotated` 对实例一律置 true**：实例的形参类型是从模板
  具体化来的，都算「已标注」，所以实例调用点的实参校验是全开的。

## 8. 验收

```powershell
cmake --build build
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p10_generics.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_legacy_all.ps1
```

- `test_p10_generics.ps1`：22 个用例
  - 11 正向（显式类型实参 / 泛型类 / 嵌套实例化 / 零成本未使用）
  - 7 推导（裸调用、`list[T]`、`dict[str,T]`、`funi[A][B]`、实例共享）
  - 4 反向（类型实参个数、推不出来、实例化实参不匹配、泛型类实参个数）
- `test_legacy_all.ps1`：仍为 15/17，不回退
  （`test_14_iter` / `test_15_forinrange` 属 P12 范围）
- `test_p1/p2/p5/p6/p7/p8/p9`：全部 0 失败

`rt.rio` 的自举仍是隐含的最强回归：它由构建期自己编译并链接进每个 exe。
