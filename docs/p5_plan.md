# P5 阶段记录：容器（list / dict）（对应路线图 P5）

> 前置：P4 已验收通过（P0 5/5、P1.2 13/13、P2 函数值 10/10、legacy 10/17）。
> 本文件只描述本阶段的范围、设计、阻塞点与验收标准，不改动 `architecture_plan.md` 的总体路线。

---

## 一、本阶段要解决什么

`architecture_plan.md` §9.1 把 P5 定义为「容器：list、dict 的字面量、索引、长度」，
验收标准是 `test_8_collections` 通过。

P4 结束后唯一因此红着的就是 **test_8_collections**，报错是：

```
Error: Code generation failed: Unsupported expression in code generation:
  [Node(RANGER): [Token(TOKEN_UNKNOWN): ""] :0:1, line 0]
```

这个报错极具误导性 —— 节点类型显示成 `RANGER`、token 是空的、位置是 `0:1`。
真正的原因是 `DictionaryExpressionNode` / `ListExpressionNode` 在构造时
**给 `RangerNode` 传的是 `NodeType::BLOCK`**（见 `rcc_ast_components.cpp`），
所以 `toString()` 打出来是 `Node(RANGER)`，而 `emitExpression` 里既没有它们的分支，
也没有 `BlockRangerNode` 之外的兜底，最后落到 `fail("Unsupported expression ...")`。

本阶段要补三件事：

1. **列表 / 字典字面量**的求值与堆分配；
2. **索引访问** `xs[i]` / `d[k]`，含 list 的边界检查与 dict 的键查找；
3. **容器的静态类型推断** —— 这是让 `sout` 能正确分派的关键（见 §三.3）。

---

## 二、AST 形态（实测，与直觉不同）

| 写法 | 节点 | 体 |
|---|---|---|
| `{k: v, ...}` | `DictionaryExpressionNode` | `getBodyNode()`，`{}` 时为 `nullptr` |
| `[a, b, ...]` | `ListExpressionNode` | `getBodyNode()`，`[]` 时为 `nullptr` |
| `xs[i]` | `IndexExpressionNode`（**`PostfixExpressionNode` 子类**） | `getLeftNode()` = 目标，`getIndexNode()` = `BracketExpressionNode` |

三个关键坑：

1. **`PairExpressionNode` 不是 parser 实际产出的节点。**
   字典的 `k: v` 走的是 `rcc_infix_builder.cpp` 的 `infixExpressionNodeTypeMap`，
   `TOKEN_COLON → NodeType::PAIR`，造出来的是 **`InfixExpressionNode`**，
   靠 `getOpToken().getType() == TOKEN_COLON` 识别。逗号同理（`TOKEN_COMMA → PARALLEL`）。
   所以拆分函数必须按「中缀链」递归，不能 `dynamic_pointer_cast<PairExpressionNode>`。
   （防御性地留了 `PairExpressionNode` 分支，但正常路径用不到。）

2. **`IndexExpressionNode` 的键被方括号包了一层。**
   `rcc_postfix_builder.cpp` 造的是
   `IndexExpressionNode(left->getMainToken(), left, buildBracketExpression())`，
   键在 `BracketExpressionNode::getBodyNode()` 里，必须剥壳（`unwrapIndexKey`）。

3. **`IndexExpressionNode` 必须在 `emitExpression` 里排在 `PostfixExpressionNode` 之前。**
   它是后者的子类，落到后面那条会被 `emitPostfix` 当成 `++/--` 而报
   `Only simple variable ++/-- is supported`。

---

## 三、设计

### 3.1 运行时布局

与 `architecture_plan.md` §4.2 的 `VecHeader` 对齐：

```
ListHeader { i64 len, cap, rc, elem_size; u8 data[] }   —— 32 字节头，数据从 +32 起
DictHeader { i64 len, cap, rc; ptr keys; ptr vals }     —— 40 字节头
```

- 两者都是**单指针**表示（与 `str` 一致），表达式结果就是头指针，放 `%rax`。
- 每个元素槽恒为 **8 字节**：整型/指针直接存，浮点用 `movsd` 按位存、读取时按静态类型解释。
- 字典的 `keys` / `vals` 是两块连续数组，查找用**线性扫描**。
- 计划里只写了 `MapHeader` 这个名字、没定字段，40 字节头的字段由本阶段确定
  （`len/cap/rc` + 两个数组指针）；`cap` 已经留好，后续加扩容不用改 ABI。

### 3.2 求值顺序

**先分配、再填元素**：

```asm
# 1. 整块分配（头部 + n 个槽）
movq $<32+8n>, %rcx ; movq $8, %rdx ; call core_alloc
# 2. 指针存进临时栈槽（后面每一步都可能再分配）
# 3. 填 len / cap / rc / elem_size
# 4. 逐个 emitExpression(元素)，结果写进 32+8i(%rcx)
```

反过来（先求值再分配）就得把 n 个中间结果跨分配保存，没有寄存器可用。

**中间值一律走栈上临时槽**（`allocateTemp`），不用 `push`/`pop`：
`push` 会让 `%rsp` 变成 8 字节非对齐，中途一旦 `call` 就违反 MS x64 ABI 的
16 字节对齐要求。临时槽名带 `__p5tmp_` 前缀 + 递增计数，不会撞用户标识符，
且走 `allocateSlot` 让 `_frameSize` 正确推进，帧回填会自动覆盖。

### 3.3 静态类型推断（本阶段的核心难点）

`sout` 按**静态类型**分派到 `rt_print_*`，所以：

```rio
var dic = {"name": "Rio", "age": 18}
sout(dic["name"])   // 必须推出 str
sout(dic["age"])    // 必须推出 int
```

同一个字典的两个键，值类型不同。为此：

- `ListType` / `DictType` 进 `rcc_sema_types.h`（`toString()` 分别是 `list[T]` / `dict[K, V]`）。
- `DictType` 额外带一张**键类型精化表** `std::map<std::string, TypePtr>`，
  只对**字符串字面量键**登记「这个键自己的值类型」。
- `inferIndexResultType` 先看键是不是字符串字面量：是就查精化表，否则回退到统一的值类型。
- **精化表不参与 `equals()`** —— 它只是编译期分派用的附加信息，
  两个键类型相同的字典在语义上就是同一个类型。
- 异构字面量的统一值类型退化成 `int`（槽恒 8 字节，退化是安全的，
  只是打印时按 int 解释）。真正的 `any`/和类型属于 P6/P10。

### 3.4 索引访问

**list**：无符号比较一次拦掉越界和负下标，落空返回 0。

```asm
movq (%rcx), %rdx      # len
cmpq %rdx, %rax        # index
jae  miss              # 无符号：负下标变巨大数，同样落空
movq 32(%rcx,%rax,8), %rax
```

**dict**：线性扫描，循环状态（下标、键、容器指针）全放栈槽，
每次迭代重新装载 —— `core_mem_eq` 会按 MS ABI 破坏所有 caller-saved 寄存器。

字符串键按**内容**比较：先比长度（长度在头里，`(%reg)`），再 `core_mem_eq`。
未命中返回 `null`（与 legacy 的 `notFoundBB` 行为一致）。

> 踩坑记录：第一版把**头指针**直接传给 `core_mem_eq`，结果 `{"a":1,"b":2,"c":3}`
> 全部命中第一个键，输出 `1 1 1`。原因是头指针指向的是 `len` 字段，
> 三个键长度都是 1，比出来的永远是「相等」。真实字节从 **+8** 起，
> 必须 `leaq 8(%reg), %rcx` 再传。

### 3.5 函数返回类型改为「声明优先、否则推断」

原来 `emitFunctionDefinition` 对没写 `: type` 的函数一律登记 `int`。
这在容器时代不够用：

```rio
fun make_list() { ret [100, 200, 300] }
sout(make_list()[1])     // 目标类型是 int -> "Index access on a non-container value"
```

改为：声明的 `: type` 优先，否则用 `inferAnonReturnType`（P4 已有的那套
影子作用域推断）从 `ret` 表达式推。推断不出才回退 `int`。
登记位置必须在**形参类型收齐之后**（推断要放形参槽）、`emitFunctionBody` 之前
（函数体内可能有递归调用）、`encapsulated` 早退之前（`sout` 要能查到签名）。

---

## 四、改动文件

| 文件 | 改动 |
|---|---|
| `code/include/sema/rcc_sema_types.h` | 新增 `ListType` / `DictType` + 键类型精化 API |
| `code/include/backend/rcc_codegen.h` | P5 方法声明、三个 `infer*` 声明、`_tempCounter` |
| `code/src/backend/rcc_codegen.cpp` | 三个 `infer*` 实现、六个 `emit*` 实现、`emitExpression` 分派、返回类型推断 |
| `test_scripts/test_p5_containers.ps1` | 新增，14 个用例 |

---

## 五、验收结果（2026-11-15）

| 套件 | 脚本 | 结果 |
|---|---|---|
| P0 回归 | `test_scripts/test_p0.ps1` | **5 / 5** |
| P1.2 控制流 | `test_scripts/test_p1_control.ps1` | **13 / 13** |
| P4 函数值 | `test_scripts/test_p2_functions.ps1` | **10 / 10** |
| P5 容器（本阶段） | `test_scripts/test_p5_containers.ps1` | **14 / 14** |
| legacy 全量 | `test_scripts/test_legacy_all.ps1` | **11 / 17** |

legacy 从 P4 的 10/17 变为 **11/17**，新转绿的是 **test_8_collections**。
剩余 6 个的归属不变：test_11/12→P7，test_14/15→P8，test_16/17→P6。

`test_p5_containers.ps1` 覆盖的形态（14 个用例）：

| 用例 | 覆盖点 |
|---|---|
| `dict_literal` | 字符串键按内容查找 + 每键静态类型正确 |
| `dict_multi_key` | 三个同长度键（专门抓「按头指针比较」的 bug） |
| `dict_miss` | 未命中返回 null（用 `== null` 验证） |
| `dict_var_key` | 键是变量，回退到字典值类型 |
| `dict_long_keys` | 长度不同 / 前缀相同的键，验证先比长度再比内容 |
| `list_literal` | 整数列表下标取值 |
| `list_float` | 浮点元素按位存、按静态类型取 |
| `list_oob` | 越界 + 负下标都返回 0 |
| `empty_containers` | `[]` / `{}` 不崩，取值为 0 / null |
| `nested` | 嵌套容器 `grid[0][1]`、`d["nums"][1]` |
| `loop_index` | 循环里反复取下标 |
| `dict_in_func` | 容器当返回值，再就地索引 |
| `list_sum_loop` | 列表遍历求和 |
| `mixed_dict` | 异构值退化成 int，键仍按内容命中 |

---

## 六、已知限制

1. **字典查找是线性扫描**，复杂度 O(n)，无哈希。
2. **不支持 append / 下标赋值**（`xs[0] = 1`、`d["k"] = v`）：`INDEX_ASSIGNMENT`
   在 parser 里是独立节点类型，属于后续阶段。
3. **容器不释放**：`rc` 字段留着但没人用，所有容器泄漏。
4. **异构字面量的统一值类型退化成 int**，打印时按 int 解释。
5. **容器作函数实参时无法索引**：无类型标注的形参静态类型是 `TK_UNKNOWN`，
   索引时判断不出是 list 还是 dict，直接报错。
   返回值方向没问题（返回类型从 `ret` 推断）。
   要支持得等 P11 的重载/类型推断。
6. **不支持 `.size` / `.len` 成员访问**（`ATTRIBUTE` 节点，属于 P6 的成员访问体系）。
   test_14_iter 用的 `ds.size` 因此仍然阻塞在 P8。

---

## 七、下一步

按 §9.2 依赖链，P5 之后是 **P6（类：定义、字段、方法、继承）**。

P6 要先把 `.` 成员访问（`ATTRIBUTE` 后缀）做出来 —— 这也是 test_16/test_17
当前的阻塞点（`Undefined variable: x` / `val`，其实是 `self.x` 这类成员访问被当成了
普通标识符）。test_14/15 依赖的 `.size` 也属于同一套机制，可以在 P6 顺手做掉。
