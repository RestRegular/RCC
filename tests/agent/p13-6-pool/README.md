# P13-6 Pool 与成员集合 回归套件

对应规范 `docs/memory-model-spec-full.md` §6，裁决见 `docs/p13_6_syntax_audit.md` §3，
方案见 `docs/p13_6_implementation_plan.md`。7 个用例覆盖 Q1/Q2 两项落地：

- **Q1 三档真实分配器**：`Pool.chunked` 多块追加（旧槽不动、字段读写正常）、
  `Pool.boxed` 每槽独立、`Pool.fixed` 容量内正常 + 满时运行时 panic（退出码 3）。
- **Q2 成员集合最小集**：`ds.listAppend` 使成员进入集合，release 被拦（重数 1）；
  两次 append 饱和为 many（诊断显示 `mult=many`）；未入容器的 Body 可正常 release。

`dict.clone()`/递归释放（Q4）、状态规则放宽/§11.1 通路（Q3）、Link 条款（P13-7）、
成员集合完整化（Q2 的 B 范围）本阶段不做，见审计 §5 后续项。
