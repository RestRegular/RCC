# p13-3-nullable

决议 B 的回归套件：可空类型 `T?` 与 `if var` 取值。

## 覆盖范围

| 用例 | 验证点 |
| --- | --- |
| `pos_list_get` | `xs.get(i)` 命中返回元素 |
| `pos_list_get_oob` | 越界返回 null（走 else），**不 panic** |
| `pos_list_get_negative` | 负下标同样走 else（无符号比较拦下） |
| `pos_dict_get` | `d.get(k)` 命中返回 value |
| `pos_dict_get_missing` | 键不存在返回 null |
| `pos_null_ret` | `fun f(): Box?` 可以 `ret null`，也可以 `ret 具体值` |
| `pos_nullable_field` | `var p: Box?` 之后 `p.v` 可解析（穿透可空取字段归属） |
| `pos_index_still_zero` | `xs[i]` 越界仍是零值 —— get() 没有改变 `[i]` 的语义 |
| `neg_nullable_to_nonnull` | `var v: int = xs.get(0)` 报错，必须先 `if var` 取出 |
| `neg_if_var_non_nullable` | `if var p = 5` 报错（右侧必须是 `T?`） |
| `neg_value_type_nullable` | `int?` 报错（只有引用型可空） |
| `neg_scope_leak` | `if var p` 的 `p` 出了 then 块不可见 |

## 设计口径

- 只有**引用型**可以写 `T?`；值类型（int/float/bool/char/struct）没有 null 这一状态。
- `T` 可以隐式提升到 `T?`；反方向必须显式用 `if var` 取出。
- `get()` 越界返回 null，**不 panic**（用户明确否决了 panic 方案）。
- `[i]` 保持原语义（越界读零值），两者并存。
- 编译器**不做**隐式空检查：`p.v` 在 `p` 为 null 时是未定义行为，安全性由「必须先 if var 取出」的规则保证。
