# 标准库系统能力扩展提案（待审批）

> 生成时间：2026-11-20。
> **性质：提案，不是已批准设计。** 按项目硬规则 `1b1ccafb`，本文列出的 API 面变更 / 新内建原语必须逐条经用户批准后才能实施。
> 本文只做现状调研与选项分析，不含任何代码改动，不修改 `lib/` / `builtins.rio` / `core_win64.s`。

---

## 0. 背景与范围

用户 2026-11-20 批准「丰富标准库功能」，并明确将**系统能力**（文件 I/O、时间日期、环境变量）标注为「另立设计」。本提案把三类能力拆开，逐项给出：现状、可选 API 面、所需新原语、待裁决问题。

与之同批完成的纯 Rio 扩充（`math` / `str` / `ds` 共 30+ 新函数，**零新原语**）不在此提案内，已单独落地。

---

## 1. 现状调研（原仓库 + 当前实现）

### 1.1 已有的底层能力（`lib/core/core_win64.s` + `builtins.rio`）
- `core_io_write(fd, buf, n)` / `core_io_read(fd, buf, n)`：fd 只映射 0/1/2 三个标准句柄，底层已用 Win32 `WriteFile` / `ReadFile`，但**句柄是固定的标准句柄**，用户无法打开自己的文件。
- `core_time_ns()`：单调时钟（QueryPerformanceCounter 类），纳秒；适合计时/耗时，**不适合日历日期**。
- `core_alloc` / `core_free`：堆分配。
- 线程 / 锁 / 原子 / Chan / Link：P14 / P13 已实现。

### 1.2 缺失的能力
| 能力 | 现状 | 缺口 |
|---|---|---|
| 文件读写 | 仅标准 fd | 无 open / close / seek / 用户文件句柄 |
| 日历时间 | 仅单调 `core_time_ns` | 无年月日时分秒、时间戳、时区 |
| 环境变量 | 无 | 无 getenv / setenv |

### 1.3 约束
- Rio 没有解引用运算符，用户侧 std 只能用 `core_*` 原语 + 指针算术 + `core_mem_copy/set` 操作内存（见 `str.rio` / `ds.rio` 的实现模式）。
- 任何新 `core_*` 原语必须：①在 `core_win64.s` 实现 Win64 ABI 例程；②在 `builtins.rio` 前导声明；③显式处理返回值/错误码语义。
- 新增标准库公开函数须同步 `tests/agent/std-full/execute.ps1` 的三层基线表。

---

## 2. 分项提案

### 2.1 文件 I/O

**候选 API 面（三选一，须裁决）**

| 方案 | 形态 | 优点 | 代价 |
|---|---|---|---|
| A 句柄式 | `io.open(path, mode): ptr` + `io.close(h)` + `io.read(h, n): str` + `io.write(h, s)` | 贴近 POSIX，灵活 | 引入「句柄」这一新的用户可见资源，需定义其生命周期/销毁规则（与内存模型的 Body/Content 关系） |
| B 一次性 | `io.readFile(path): str` + `io.writeFile(path, s): bool` | 无句柄、无生命周期问题，最简 | 只能整体读写，不能流式/追加 |
| C 混合 | 先 B 的两个整体函数，句柄式 A 留后续 | 风险最低，先满足最常见需求 | 覆盖场景有限 |

**所需新原语（方案 A/B 都要）**
- `core_file_open(path: ptr, len: int, mode: int): ptr` —— 封装 `CreateFileA`，返回句柄或 0（失败）。
- `core_file_close(h: ptr): void` —— `CloseHandle`。
- `core_file_read(h: ptr, buf: ptr, n: int): int` —— `ReadFile`，返回实读字节数（可复用现有 io_read 的机制，但句柄来自参数）。
- `core_file_write(h: ptr, buf: ptr, n: int): int` —— `WriteFile`。

**待裁决问题**
1. 选 A / B / C 哪个 API 面？
2. 失败语义：返回 0/null、返回错误码、还是 `throw`？（倾向：句柄式失败返回 0，整体式失败返回 false，不引入新错误类型。）
3. 文件路径编码：`str` 是字节串，直接传 UTF-8 字节给 `CreateFileA`（ANSI 代码页）还是新增宽字符 `CreateFileW` 转换？（倾向：先 `CreateFileA` 字节透传，与现有 `core_cstr_len` 口径一致。）
4. 句柄生命周期是否纳入内存模型（Body/Content / 出生地）？还是作为不参与所有权检查的不透明资源？（倾向：首版当作不透明 `ptr`，用户显式 close，不纳入所有权检查——但这与「资源泄漏静态可检」的长期目标冲突。）

### 2.2 时间日期

**候选 API 面**

| 方案 | 形态 |
|---|---|
| A 最小 | `time.now(): int`（Unix 毫秒时间戳）+ `time.monotonic(): int`（复用 `core_time_ns`） |
| B 结构化 | 增加 `time.local(): struct`（年/月/日/时/分/秒）——但 struct 目前只支持标量字段，且无 `for-in`，格式化输出受限 |
| C 格式化 | `time.format(ts, fmt): str` —— 需 Win32 `GetLocalTime` + 自己拼串 |

**所需新原语**
- `core_time_unix_ms(): int` —— `GetSystemTimeAsFileTime` 转 Unix 毫秒。
- （B/C）`core_date_parts(ts: int, out: ptr): void` —— 填 6 个 int 到调用方缓冲区。

**待裁决问题**
1. 选 A / B / C？倾向 A 先落地（最小、无新类型）。
2. 时区：UTC 还是本地时间？（`GetSystemTimeAsFileTime` 是 UTC；本地需 `GetLocalTime`。）
3. 时间戳单位：秒 / 毫秒 / 纳秒？（与 `core_time_ns` 的纳秒单调钟区分开，避免混淆。）

### 2.3 环境变量

**候选 API 面**
- `prog.getenv(name: str): str`（不存在返回空串或 null）
- `prog.setenv(name: str, value: str): bool`

**所需新原语**
- `core_getenv(name: ptr, n: int): ptr` —— `GetEnvironmentVariableA`，返回字节串指针（需处理「不存在」）。
- `core_setenv(name: ptr, nl: int, value: ptr, vl: int): int` —— `SetEnvironmentVariableA`。

**待裁决问题**
1. 是否纳入首版？（环境变量属进程级全局可变状态，与 P14 的「任务不碰可变全局」Q13 口径可能冲突。）
2. 不存在时返回 "" 还是 null？（Rio 已支持 `T?` + `if var` 窄化，倾向 null。）
3. `setenv` 是否开放？修改进程环境是全局副作用，是否只给 `getenv`？

---

## 3. 与内存模型 / 并发的交互（需在裁决时明确）

- **文件句柄**：若当作不透明 `ptr`，则 `take`/`release`/出生地规则都不适用，用户必须显式 close；若纳入 Content，则借用检查会覆盖它，但需要新的「句柄类型」和 drop glue。
- **环境变量**：进程级全局可变状态。P14 的 spawn 任务 Q13 检查禁止任务触碰可变全局，若 `setenv` 开放，需在 `checkSpawnQ13` 白名单里明确拒绝。
- **时间**：`core_time_ns` 已是单调钟且线程安全；新增的日历时间是只读，无并发问题。

---

## 4. 建议推进顺序（待批准后才执行）

1. 用户逐条裁决 §2 的待裁决问题（文件 API 面 / 失败语义 / 路径编码 / 句柄生命周期；时间范围 / 时区 / 单位；环境变量是否纳入 / 返回值 / setenv）。
2. 对选中的每一项：先补 `core_win64.s` 原语 + `builtins.rio` 声明，用独立探针验证 ABI。
3. 再写 std 层公开函数（`io.*` / `time.*` / `prog.getenv`），更新 `rio_doc.md` 与 `tests/agent/std-full` 三层基线。
4. 全量回归 + 发版（版本号由用户定）。

---

## 5. 一句话总结

文件 I/O / 时间日期 / 环境变量三类系统能力都缺底层原语，属语言设计层变更，本文列明 API 面选项、所需 `core_*` 原语与待裁决问题；**裁决前不实施任何改动**。
