# p13-3-nullable

决议 B 的回归套件：可空类型 `T?`、`if var` 取值，以及容器 `get()` 的**静态**返回类型按元素类型分化（用户 2026-11-18 裁决 A）。

## 核心口径：可空性由静态类型决定，不由运行时值决定

`get()` 的返回类型是**编译期事实**，按元素的静态类型分两档：

| 元素类型 | `get()` 的静态返回类型 | 越界行为 | `if var` |
| --- | --- | --- | --- |
| 引用型（`str`/`list`/`dict`/类/`ptr`/`Ref`/`Mut`/`Take`/`Pool`） | `T?` | 给 null | ✅ 允许 |
| 值类型（`int`/`float`/`bool`/`char`/`struct`） | `T`（非可空） | 给零值，与 `[i]` 一致 | ❌ 编译期报错 |

判据只有一条：**这个类型有没有「没有」这个状态**。指针族的空是空指针，与任何合法值都不冲突；值类型的零值（`0`/`false`/`0.0`/`'\0'`）是**合法值**，拿它当 null 会让 `list[int]` 里真实的 `0` 静默变成「越界」—— 这正是裁决 `a87037d8` 证伪「零值当 null」的那条理由，不能在容器返回值上重演。

实现上只有一个事实源：`CodeGen::isNullableCapable` / `CodeGen::containerGetReturnType`。`inferType`、`T?` 标注解析、`if var` 的允许性判断全部走它，所以静态类型与「能不能 `if var`」不会各说各话。

## 覆盖范围

| 用例 | 验证点 |
| --- | --- |
| `pos_list_get` | 值类型元素 `xs.get(i)` 命中返回元素 |
| `pos_list_get_oob` | 值类型元素越界给**零值**（不是 null，也不 panic） |
| `pos_list_get_negative` | 负下标同样给零值（无符号比较拦下） |
| `pos_value_zero_is_value` | **回归重点**：`0`/`false`/`0.0` 是合法值，不能被当成「没有」 |
| `pos_dict_get` | `d.get(k)` 命中返回 value |
| `pos_dict_get_missing` | 值类型 value 未命中给零值 |
| `pos_ref_get` | 引用型元素 `get()` 仍是 `T?`，命中返回值、越界走 else |
| `pos_ref_dict_get` | 引用型 value 同上 |
| `pos_null_ret` | `fun f(): Box?` 可以 `ret null`，也可以 `ret 具体值` |
| `pos_nullable_field` | `var p: Box?` 之后 `p.v` 可解析（穿透可空取字段归属） |
| `pos_index_still_zero` | `xs[i]` 越界仍是零值 —— 与值类型元素的 `get()` 同口径 |
| `neg_if_var_value_elem` | **值类型元素的 `get()` 配 `if var` 必须编译期报错**（依据静态类型，不看运行时值） |
| `neg_if_var_non_nullable` | `if var p = 5` 报错（右侧必须是 `T?`） |
| `neg_value_type_nullable` | `int?` 报错（只有引用型可空） |
| `neg_nullable_to_nonnull` | `var s: str = ss.get(0)` 报错，必须先 `if var` 取出 |
| `neg_scope_leak` | `if var p` 的 `p` 出了 then 块不可见 |

## 设计口径

- 只有**引用型**可以写 `T?`；值类型（int/float/bool/char/struct）没有 null 这一状态。
- `T` 可以隐式提升到 `T?`；反方向必须显式用 `if var` 取出。
- 引用型元素的 `get()` 越界返回 null，**不 panic**（用户明确否决了 panic 方案）。
- 值类型元素的 `get()` 越界返回零值，与 `[i]` **完全一致**；此时 `get()` 不提供「有没有」的信息，要判断请用 `size()`。
- 编译器**不做**隐式空检查：`p.v` 在 `p` 为 null 时是未定义行为，安全性由「必须先 if var 取出」的规则保证。
