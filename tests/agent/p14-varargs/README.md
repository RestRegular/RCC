# 变长参数（`*args` / `**kwargs`）回归套件

对应设计 `docs/p12_varargs_proposal.md`（Q1–Q7 全部已裁决）。9 个用例：

- **收集语义**（Q1/Q2/Q5/Q6）：`*xs: int` 收成 `list[int]` 可索引；空调用得空表；
  `**opts: int` 收成 `dict[str,int]`；固定形参 + 变长槽混用；
- **调用点展开**（Q7-a/b/c）：`fwd(*xs)` 把容器直接转给另一个变长函数；
  `**kwargs` 不在末尾 → 编译报错；
- **泛型变长**（Q4）：首参定 T（含显式 `first[float](...)`）；`first(1, 2.5)` 严格报错；
- **强制标注**（Q2）：`*xs` 无标注 → 编译报错。

编译器特化 `sout` 的桩必须写 `*args: any`（Q2 不豁免）。
