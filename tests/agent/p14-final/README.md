# p14-final

P14 最终收尾（用户 2026-11-20 总裁决：完整实现全部未实现项，不留已知限制）验证套件。

- `pos_q13_sout_in_task`：Q13 白名单——任务可调 sout（I/O 特化）与 core_* 运行时原语（Q14 已证明线程安全）。
- `pos_at_narrow_safe`：`pool.at()` 越界返回 null，`if var` 窄化后安全；越界分支不崩溃。
- `neg_q13_global` / `neg_q13_transitive`：spawn 目标直接 / 经调用链写可变全局，编译期拒绝。
- `neg_g3_link`：spawn 目标用 Link 遍历原语（peers.count），编译期拒绝。
- `neg_g4_birth_scope`：实参出生地是已关闭的内层 scope，编译期拒绝。
- `neg_ref_conflict`：spawn 实参 `.ref()` 现在参与效果检查，与 take 冲突被拒。
- `neg_lock_reentrant_cross`：跨函数调用链上的同锁重入（f 持锁 c 时 g 又锁 c），编译期拒绝（Q6=A）。
