# P13-7 Link 成对引用 回归套件

对应规范 `docs/memory-model-spec-full.md` §7，裁决见 `docs/p13_7_syntax_audit.md` §6，
方案见 `docs/p13_7_implementation_plan.md`。11 个用例覆盖：

- **声明与对偶校验**：三段方括号 `Link[T][peer][card]`（Q1）、peer 互指 + 类型一致 + 基数相容表（Q4/Q5）。
  `parent:opt ↔ children:many` 是规范自带范例，必须放行（Q5=B 单向宽松）。
- **成对原语**：单次 `add` 即建立对称边（两端 `count()`/`has()` 一致）、`remove`/`clear` 两端同步、
  `unlink_all` 清空对象的全部 Link 字段。
- **不变量与边界**：§11.2 含环图（a→b→c→a，环无需特殊处理、销毁点唯一）、自环合法。
- **take/release 判据**（§7.4）：未 `unlink_all` 就 `take` 编译期拒绝；`set` 仅 opt/one，`many` 上拒绝。
- **同池检查**（Q6=A）：跨池 `add` 运行时 panic（`cross-pool link`，退出码 3）。

## 本次未覆盖（登记为后续项）

- §7.5 的实例级冲突判断 / `a≠b` 五来源 / snapshot 迭代修改：当前编译器无 `for-in` over Link
  （无消费者），只实现了顺序场景的保守规则（add 后 take/release 被 Link 事实表拒绝）。
- `one` 的 `set` 自动从旧对端移除：当前 `set` 简化为 add（方案 §4.2 已标注偏差）。
- `links_nonempty` 物理槽位图（Q11=A 裁决已定，实现用编译期 LinkSet 事实表替代；物理位图待补）。
