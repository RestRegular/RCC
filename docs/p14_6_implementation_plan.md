# P14-6 实施方案：验收（粒子示例 + §10.9 论证对应 + 回归套件）

> **本文性质**：实施方案 + 验收报告（2026-11-20）。依据已裁决的 P14 全部设计（`docs/p14_plan.md` §7，Q1–Q20）、
> 规范 §10.8/§10.9、以及 **2026-11-20 可视化浏览器中 Claude Sonnet 5.5 对 P14-6 的裁决**（记忆 `c31b11ee`，会话 dacdc5b2）。
> 遵守硬规则 `1b1ccafb`：本文只含已批准设计内的缺陷修正与文档订正，无新增语言设计。
> 裁决原则：Claude 裁决可自行执行项；拿不准的 6 点上报用户（见 §6）。

---

## 0. 阶段定位与裁决依据

- 路线（`p14_plan.md §2`）：P14-6 是 P14 收尾阶段，前置 P14-1a/1b/3/4/5 均已完成。
- Claude 裁决（Q-A～Q-E，见记忆 `c31b11ee`）：
  - **Q-A** 粒子示例形态：par_zip 不纳入；正确形态是「遍历被写 Pool + 只读捕获另一 Pool + at(id) 查找」，两阶段各一条 par_each 自带 join；spike 须验证三件事（捕获 Ref[Pool]、at() 只读、闭包内写另一 Pool 元素必须被拒）。
  - **Q-B** `.mut()` 漏判属已批准设计内缺陷修正，可直接做。
  - **Q-C** 分项：G5 条文当场补、G4 补负例、G3 先评估（能复用则补否则列限制）、G1 列已知限制、跨函数锁重入列已知限制。
  - **Q-D** 五条→六条纯文档订正。
  - **Q-E** 验收结论：有条件通过，如实列已知限制。

---

## 1. 实测发现（codegraph + 编译运行探针）

### 1.1 粒子示例不能直接照抄规范
- par_zip/par_zip_into 被 Q5=C 排除；签名按 Q19 为 `fn step(pool: Mut[Pool[Particle]], cfg: Ref<Config>)`。
- par_each 闭包只有 1 个元素形参、无 id 参数；取另一 Pool 元素须 `pool.at(id)`（返回裸 `ptr`）。
- 无 `pool.len()`（Q-E 明确禁止顺手加），容量固定为 8。
- 两阶段靠两条顺序 par_each 各自 join 保证，规范里的 `scope s1/s2` 省略。

### 1.2 三个真实缺陷（P14-6 修复）
1. **G2 `.mut()` 漏判**：spawn 效果检查 `takeReceiverName` 只识别 `.take()`，注释谎称也识别 `.mut()`；两个 spawn 都传 `pl.mut()` 被当读-读放行（真竞态）。修复见 §2.1。
2. **boxed 接收者解引用缺失（at/par_each）**：被闭包按引用捕获的 Pool 会 boxed，栈槽存盒地址而非 pool 指针；`at()` 分支与 `par_each` 接收者漏解一层，导致 `core_pool_slot_at` 沿垃圾 prev 链死循环（CPU 100%）或崩溃。由 P14-6 探针发现并修复，见 §2.2。
3. **§10.4 捕获 Pool 元素写入未被拒**：`var f = captured.at(id); f.value = ...` 这种写另一槽位的写法能编译通过。Claude Q-A 要求必须为负例。修复见 §2.3。

### 1.3 §10.9 六条论证的对应与缺口（p14_plan.md:112 原误写「五条」，已订正）
| 条 | 论证 | 实现对应 | 状态 |
|---|---|---|---|
| 1 | 只通过实参获得数据 | emitScope 收集 spawn 实参 + Q3 具名函数 | **缺口 G1**：Q13 跨函数全局/extern 分析未实现，静态不成立 |
| 2 | 效果不冲突 | mayAlias + 写判定 + 父线程冻结（Q4） | `.mut()` 漏判已修（§2.1）；`.ref()` 不产路径属既有缺口（见 §5） |
| 3 | 状态位视为内容访问 | BodyState + checkTakeArgOverlap + par_each Link 禁令 | **缺口 G3**：F/E/Link 状态位未参与 spawn 检查；Link 分析仅 par_each 闭包内有 |
| 4 | 出生地包住 scope | 出生地登记 + Q17 join-before-drop | **缺口 G4**：无直接静态检查，靠时序间接保证；探针证实不安全程序可编译（见 §5） |
| 5 | 共享可变只经 Locked | emitLock + rejectGuardEscape + 重入检测 | 基本闭合；跨函数重入不检测（deferred） |
| 6 | 只剩死锁活锁 | — | **不成立**：上述 1/3/4 及跨函数重入均未静态覆盖 |

---

## 2. 本阶段改动

### 2.1 G2 修复（codegen）
- `rcc_codegen.h` / `rcc_codegen.cpp`：新增 `mutReceiverName`（独立于 `takeReceiverName`，避免污染 take 的所有权语义）。
- `emitScope` 效果推导：`.take()` 与 `.mut()` 均算写能力，取各自接收者路径。
- 负例：`neg_mut_conflict`（mut/mut）、`neg_mut_vs_bare`（mut/bare）均编译期报 `parallel conflict`。
- 回归：修复不改变非 Mut 路径；全量见 §4。

### 2.2 boxed 接收者修复（codegen）
- `pool.at(id)` 分支（`emitFunctionCall`）：`recvVar->boxed` 时多 `movq (%rcx), %rcx` 解一层 box。
- `emitParEach` 接收者：`poolSlot->boxed` 时把 pool 指针解到专属栈槽再用（影响 `core_pool_count` 与 `core_pool_slot_at` 两处调用）。
- 未 boxed 接收者生成的机器码逐字节不变。
- 验证：粒子两阶段 spike 从「死循环/崩溃」变为输出 `1/11/21/31`。

### 2.3 §10.4 捕获 Pool 元素写入拒绝（codegen）
- `emitParEach` 闭包扫描：先收集「由 `captured.at(...)` 初始化的局部别名」，赋值目标是这些别名时编译期拒绝。
- 负例 `neg_write_captured_pool_elem` 报 `cannot write element 's' obtained from a captured pool via at()`。
- 合法写法（只读捕获另一 Pool + 只写元素参数）不受影响。

### 2.4 文档（Q-C/Q-D）
- `memory-model-spec-full.md` §10.9 后补 **happens-before 同步点条文**（spawn / join / par_each join / lock-unlock / send-recv / Atomic），对应 Q15 裁决，按实现实际同步点写。
- `p14_plan.md:112`「五条」→「六条」。

---

## 3. 测试套件 `tests/agent/p14-6-acceptance`（4 条）

| 用例 | 类型 | 内容 | 期望 |
|---|---|---|---|
| `pos_particle_two_phase` | 正向 | §10.8 粒子两阶段示例 Rio 落地 | `1/11/21/31` |
| `pos_two_phase_in_closure` | 正向 | 捕获 Pool 的 boxed at/par_each 回归 | `100×3` |
| `neg_write_captured_pool_elem` | 拒绝 | 闭包写捕获 Pool 元素 | `cannot write element` |
| `neg_mut_conflict` | 拒绝 | 两 spawn 以 Mut 写同一对象 | `parallel conflict` |

---

## 4. 验证结果

- 基线（修复前）：37 套件 / 1012 用例全绿。
- 修复后：见下方「最终验证」更新（每轮全量结果以实际运行为准）。
- Claude 特别要求（Q-B）：修复导致原本合法用例被拒时应停下报告，不改测试迁就 → 实测未发生，所有原有用例仍通过。

---

## 5. 已知限制（P14-6 首轮状态；收尾轮已全部闭合，见 p14_plan.md §8.2）

> 2026-11-20 用户总裁决后，下列 1–5 项已全部实现，不再是限制。保留此节仅为记录首轮验收的如实结论。

1. ~~**Q13 未实现**~~ → 收尾轮已实现 `checkSpawnQ13`（跨函数全局/extern 汇总分析）。
2. ~~**G3** spawn 目标未参与 Link 分析~~ → 已实现 `checkSpawnLinkPrimitives`。
3. ~~**G4** 出生地包住 scope 无静态检查~~ → 已实现 `checkBirthPlaceEnclosesScope`。
4. ~~**`.ref()` 不产效果路径**~~ → 已修复 `exprPathText` 分支顺序。
5. ~~**跨函数锁重入不检测**~~ → 已实现 `scanLockReentrancy`。

**仍不在静态保证范围**（规范 §10.9 第 6 条原有声明，非缺陷）：**死锁与活锁**。首版无任务取消机制（Q9/Q10），循环等待无法静态排除。

---

## 6. 必须留给用户裁决的点（Claude 明确点名，我不自行决定）

1. **P14 能否在 G1 未实现时收口**（验收标准问题）。
2. **Q13 白名单**：哪些 I/O/全局访问允许；是否先做「只查 spawn 目标函数体直接引用」的更严临时规则。
3. **新增接口一律不顺手加**：`pool.len()`、`par_each` 增 id 形参、`at()` 增 null 窄化；另 `at()` 对无效槽返回未窄化指针是潜在安全缺口，是否处理。
4. **§10.8 示例文本修订**：用 par_each 重写、阶段1 并行/串行，改规范原文须用户批准。
5. **G2 修复若拒既有合法用例**是修复还是放宽（本次未发生）。
6. **G1/G3 延后到哪个阶段**的排序。
