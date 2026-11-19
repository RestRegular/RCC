# p14-6-acceptance

P14-6 验收套件。

- `pos_particle_two_phase`：§10.8 粒子系统两阶段示例的 Rio 落地（par_each + 捕获只读另一类型 Pool + 槽位 id 查找；par_zip 按 Q5=C 未纳入）。验证阶段1 并行写 forces、阶段2 并行写 particles，各由 par_each 自带 join 分隔。
- `pos_two_phase_in_closure`：被闭包捕获的 Pool 再次作为 par_each 接收者、以及闭包内 `at()` 的 boxed 解引用回归（P14-6 修复）。
- `neg_write_captured_pool_elem`：闭包内写「捕获 Pool 经 at() 取出的元素」必须编译期拒绝（§10.4 / Claude Q-A 裁决）。
- `neg_mut_conflict`：两个 spawn 都以 Mut 传同一对象必须编译期拒绝（Q4 / Claude G2 裁决）。
