# P6 阶段记录：类（class / ctor / 字段 / 方法 / 继承 / super）（对应路线图 P6）

> 前置：P5 已验收通过（P0 5/5、P1.2 13/13、P4 函数值 10/10、P5 容器 14/14、legacy 13/17）。
> 本文件只描述本阶段的范围、设计、阻塞点与验收标准，不改动 `architecture_plan.md` 的总体路线。

---

## 一、本阶段要解决什么

`architecture_plan.md` §9.1 把 P6 定义为「类：class / 构造器 / 字段 / 方法 / 继承 / super」，
验收标准是 `test_16_class_basic` 与 `test_17_class_inheritance` 通过。

P5 结束后这两个用例都红着，报错是：

```
Error: Code generation failed: Undefined variable: x     (test_16)
Error: Code generation failed: Undefined variable: val   (test_17)
```

报错同样有误导性。`this.x` 里的 `x` 并不是一个「变量」，而是成员访问；
后端此前完全没有 `.` 这条路径，`emitExpression` 里遇到 `InfixExpressionNode`
一律走 `emitBinary`，于是 `x` 被当成标识符去查作用域，自然查不到。

本阶段要补四件事：

1. **成员访问** `obj.field`（读 + 写）—— 这是 `p5_plan.md` §七 明确指出的阻塞点，
   也是 `test_14` / `test_15` 将来要用 `.size` 的同一套机制；
2. **对象布局与分配** —— 类实例的内存形态；
3. **构造器**（含 `super(...)`）与**方法调用**的 ABI 约定；
4. **类的静态类型**（`sema::ClassType`）—— 让 `obj.field` 能解析出字段偏移、
   让 `p.method()` 能找到方法、让 `sout(p.x)` 能按字段静态类型分派。

---

## 二、AST 形态（实测）

| 写法 | 节点 |
|---|---|
| `class Point { ... }` | `ClassDefinitionNode`，`getNameNode()` = `IdentifierNode("Point")` |
| `class Derived: Base` | 类名 `IdentifierNode` 上的**标签**就是基类名（`hasTypeAnnotation: true, label: Base`） |
| 类体 | `getBodyNode()` = `BlockRangerNode` |
| `var x: int, y: int` | 一个 `VariableDefinitionNode` 带 `getVarDefs()[0..n]`，字段类型在 `VarDefData::getLabelNodes()` |
| `ctor(...) { ... }` | `ConstructorDefinitionNode`，`getParamNode()` = `ParenRangerNode`，`getBodyNode()` = `BlockRangerNode` |
| `fun sum(): int { ... }` | `FunctionDefinitionNode`，形状与自由函数**完全一致**（`callNode` = `FunctionCallNode{left: IdentifierNode("sum"), right: ParenRangerNode}`） |
| `this.x` / `p.x` | `InfixExpressionNode`，`op == "."`，left = 对象表达式，right = `IdentifierNode` 字段名 |
| `this.x = v` | `AssignmentNode`，`target` 就是上面那个 `.` 中缀 |
| `super(v)` | 普通 `FunctionCallNode`，被调者 = `IdentifierNode("super")` |
| `p.sum()` | `FunctionCallNode`，left = `.` 中缀（`p.sum`），right = `ParenRangerNode`（实参） |
| `Point(10, 20)` | 普通 `FunctionCallNode`，被调者 = `IdentifierNode("Point")` —— **与自由函数调用在 AST 上完全无法区分** |

最后一条是本阶段最关键的结构性事实：构造调用没有专属节点，
只能靠「这个标识符是不是已登记的类名」来判定，并且判定必须排在
「这个标识符是不是局部变量」**之后**（见 §五.3）。

---

## 三、对象布局

对象用**单个指针**表示（与 `str` / `list` / `dict` 一致），指向一块扁平的
8 字节槽数组，**没有头**：

```
Point { x: int, y: int }        偏移 0: x     偏移 8: y      size = 16
Derived: Base { val, extra }    偏移 0: val   偏移 8: extra  size = 16
```

三条规则：

1. **字段 i 在 `+8*i`**，每个槽恒为 8 字节，与容器元素槽一致；
2. **基类字段排在前面**（`computeLayout` 先递归排基类，再把派生类自己的字段接在后面），
   于是派生类对象的内存前缀与基类布局**完全一致**；
3. 因为没有头，`Derived*` 可以直接当 `Base*` 用，`super(...)` 也就能把
   同一个 `this` 指针原样交给基类构造器，不需要任何偏移调整。

第 2、3 条是本阶段最重要的设计决定：**布局兼容换掉了全部类型转换逻辑**。
代价是不支持多继承（Rio 也不需要）和虚函数表（P6 范围内没有虚方法）。

字段偏移在 `ClassInfo::fieldOffsets` 里存的是**字节偏移**，不是槽序号。
（实现时这里踩过一次：`computeLayout` 里每个字段 `offset += 8`，而
`emitAttribute` 又乘了一次 8，导致 `e.b` 读到了 `+64` 而不是 `+8`，
输出成随机指针值。已修正。）

对象分配用 `core_alloc(size, 8)`，紧接着 `core_mem_set(obj, 0, size)` 整体清零，
所以没赋值的字段默认是 0。空类（没有字段）也至少分配 8 字节，避免零长分配。

---

## 四、ABI 约定

| 符号 | 约定 |
|---|---|
| `<Class>.__init__` | 隐藏 `this` 在 `%rcx`，用户实参从 `%rdx` 起；返回 `this` |
| `<Class>.<method>` | 隐藏 `this` 在 `%rcx`，用户实参从 `%rdx` 起 |
| `super(...)` | 取当前 `this` 槽的值放进 `%rcx`，调基类 `__init__` |
| 构造调用 `Point(a, b)` | 先 `core_alloc` + 清零，再把对象指针放 `%rcx`、实参从 `%rdx` 起排 |

`<Class>.__init__` 这个符号名沿用 legacy `test_16` / `test_17` 的 `.ll` 文件里的约定
（`%Point = type { ptr, ptr }`，ctor 符号 `Point.__init__`），便于对照。

隐藏 `this` 占用一个整型参数寄存器，所以 `emitArgsToRegisters` 加了
`intStart` / `floatStart` 两个参数，把用户实参的寄存器游标推到 `%rdx`。
「最多 4 个整型实参」的上限因此对方法变成 3 个用户实参 —— 这是 P6 已知限制。

`this` 在构造器 / 方法体里**绑定成一个局部槽**（`allocateSlot("this", ...)`，
值来自 `%rcx`），而不是特殊寄存器。好处是 `emitIdentifier("this")`、
`emitAttribute`、`emitSuperCall` 全都走既有的槽位逻辑，不需要为 `this` 开特例。

默认返回：构造器用 `defaultReturnVar = "this"`，所以没写 `ret` 时返回 `this`
（构造调用的结果就是这个对象）；方法用 `defaultReturnVar = ""`，
没写 `ret` 时按返回类型给 0 / 0.0。

---

## 五、三个必须按顺序判断的地方

### 1. `.` 的分派位置

`emitExpression` 里 `.` 必须在**通用中缀分支之前**截获，
`emitAssignment` 里左值是 `.` 时必须走 `emitAttributeAssign`，
`inferType` 里 `.` 要返回字段的静态类型（否则 `sout(p.x)` 按 int 分派）。

### 2. 方法调用 vs 自由函数调用

`p.sum()` 的被调者是 `.` 中缀，在 `emitFunctionCall` 开头就截获，
查 `collectMethods(owner)`（沿继承链收集，派生类同名方法胜出），
用**定义所在类**的符号名 `owner.method` 发射 `call`。

### 3. 构造调用 vs 局部变量

判定顺序必须是：

```
if (callee 是 "." 中缀)      → 方法调用
else if (name == "sout")     → sout 特化
else if (name == "super")    → super 调用
else if (不是局部变量 && 是已登记类名) → 构造调用
else if (是局部变量)          → 间接调用
else                         → 直接调用
```

构造判定放在「是不是局部变量」**之后**，与 memory `db817d1e` 记的
「直接/间接调用按局部变量判定」一致：局部变量遮蔽同名类时以局部变量为准。

---

## 六、实现清单

`code/include/sema/rcc_sema_types.h`：

- `TypeKind::TK_CLASS`，`isPointerLike()` 覆盖它，`getSize()` 返回 8；
- `class ClassType final : public Type`，`equals()` 按类名比较。

`code/include/backend/rcc_codegen.h`：

- `ClassInfo`（字段名/类型/顺序/偏移、size、ctor 参数与体、方法表）；
- `MethodRef`（方法定义 + 定义所在类）；
- `registerClass` / `computeLayout` / `emitClassBodies`（三段式，见下）；
- `emitAttribute` / `emitAttributeAssign` / `emitConstructorCall` /
  `emitMethodCall` / `emitSuperCall`；
- `findClass` / `collectMethods` / `classNameOf` / `findFieldOffset` /
  `findFieldType` / `classTypeOf` / `resolveTypeName` /
  `collectParamNamesT` / `collectParamTypesT` / `methodReturnType`；
- `emitFunctionBody` 加 `defaultReturnVar` / `hasThis`；
- `emitArgsToRegisters` 加 `intStart` / `floatStart`。

`code/src/backend/rcc_codegen.cpp`：

- `emitProgram` 改成三段式：
  1. 遍历程序，把 `ClassDefinitionNode` 挑出来 `registerClass`，函数体**不立即发射**；
  2. 对每个类 `computeLayout`（递归排基类）；
  3. 发射所有自由函数；
  4. 对每个类 `emitClassBodies`（ctor + 方法）。
  顺序是硬要求：`main` 里的 `Point(10, 20)` 必须在类登记之后才被识别成构造调用，
  而方法体里调用别的类的方法也要求全部布局已完成。
- `parseTypeName` 只认内建类型名，类名一律返回 `TK_UNKNOWN`。
  所以新增 `resolveTypeName`：先走 `parseTypeName`，认不出来再查类登记表。
  所有「类型名 → TypePtr」的地方（形参、返回类型、字段类型）都改走它。

---

## 七、已知限制

1. **方法最多 3 个用户实参**（隐藏 `this` 占掉 `%rcx`），构造器同样；
2. **没有虚方法 / 动态派发** —— 方法调用按**静态类型**绑定；
3. **没有多继承**，只支持单继承链；
4. **没有访问控制**（`private` / `public`），字段与方法一律可见；
5. **没有析构**，对象永不释放（`rc` 字段尚未用于类）；
6. **`.size` / `.len` 尚未接入** —— 机制已就绪（`emitAttribute` 就是入口），
   但需要先区分「字段访问」与「内建属性」，等 P8 一起做。

---

## 八、验收

新增 `test_scripts/test_p6_classes.ps1`，15 个用例覆盖：

字段读写、构造器、多字段、无构造器类（全零初始化）、无参方法、带参方法、
方法改字段、继承字段、继承方法、三层继承、浮点字段、对象作实参、
对象作返回值、两个独立对象互不影响、方法返回值参与运算、嵌套对象字段。

结果：

```
P0 5/5   P1.2 13/13   P4 10/10   P5 14/14   P6 15/15
legacy 13/17（test_16、test_17 转绿）
```

剩余 4 个 legacy 失败用例与本阶段无关：

- `test_11_throw` / `test_12_trycatch` —— 异常，P7；
- `test_14_iter` / `test_15_forinrange` —— `import` 与 `ds.forInRange`（函数调用，非 for-in 语法），P8。
