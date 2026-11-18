# P13-6 实施方案：Pool 三档分配器 + 成员集合最小集

> **本文性质**：实施方案，**依据已裁决的 Q1–Q4**（见 `p13_6_syntax_audit.md` §3），未改任何 codegen。
> 遵守 RCC 硬规则（`1b1ccafb`）：本方案只实现规范 §6 已冻结的设计（Pool 普通类型、不带类型参数、API 名冻结），**不新增语法/关键字/公开 API**。
> 裁决：Q1=A 三档真实分配器（默认 chunked）、Q2=A 成员集合最小集、Q3=A 不放宽状态规则、Q4=A dict.clone 不做。

---

## 0. 范围与不做什么

**做**：
1. core 层 `core_pool_*` 三档分配器（fixed/boxed/chunked，槽地址稳定，侧位图 2 位/槽）。
2. codegen 三档构造器从 stub 接成真实分配；`pool.new[T]` 在池内分配槽位。
3. 编译期成员集合最小集（路径索引 + 重数 `1`/`many`/`⊤` + 四原语效果）+ release 判据接入"成员重数=0"。

**不做**（本阶段登记为后续项）：`dict.clone()`/递归释放（Q4）、状态规则放宽/§11.1 示例通路（Q3）、Link 条款（P13-7）、成员集合完整化 *@L/嵌套/闭包/函数边界（Q2 的 B 范围）、并发 chunked（P13-8）。

---

## 1. 关键约束（来自事实调研）

- **Pool 无类型参数**：元素大小只有 `pool.new[T]` 时才知道 → 槽位必须**惰性分配**（构造器只建池头，`new[T]` 时按 `T` 大小分槽）。
- **地址稳定**（§6.1）：槽位一旦分配永不搬家 → 三档都只在追加时分配新块，绝不 realloc 已有槽。
- **侧位图**（§6.5）：每槽 2 位 `full`/`links_nonempty`，所有 Pool 一律带；`links_nonempty` 本阶段只置位不消费（P13-7 用）。
- **ABI 铁律**（`core_win64.s:21-25`）：任何借 `%rsi/%rdi` 当临时的例程必须 push/pop 恢复；跨 `core_*` 调用的活值放栈槽；3 push + `sub $N` 使栈 16 字节对齐。
- **core 例程注册点**：`core_win64.s`（实现 + `.globl`）、`core.rio`（extern，写带 `core_` 全名）、`coreReturnType()`（非 int 返回登记）。构建链路无需改（单个 .s 整体汇编、自动链 core.o、MinGW 默认链 kernel32）。

---

## 2. Q1：Pool 三档真实分配器

### 2.1 Pool 句柄布局（core 与 codegen 共用常量）

池头（8 字节对齐，固定大小，仿 `kListHeaderSize` 常量放 `rcc_codegen.h`）：

```
偏移  字段        说明
0     mode        1=fixed 2=boxed 3=chunked
8     elem_size   惰性设置：首个 new[T] 时写入（0=未定）
16    used        已分配槽数
24    capacity    fixed 的容量；boxed/chunked=0（动态）
32    meta        档位专属：
                 fixed  -> 指向 data 块（cap*elem_size + 位图）
                 boxed  -> 0（每槽独立 core_alloc）
                 chunked-> chunk 链首指针
```

槽位句柄 = 池头指针本身（地址稳定的"壳子"）。`pool.new[T]` 返回的 Body 槽地址由 `core_pool_alloc` 给出。

### 2.2 core 例程（`lib/core/core_win64.s` 新增，仿 `core_alloc`/`core_list_append`）

| 例程 | 签名 | 语义 |
|---|---|---|
| `core_pool_new` | `(mode:i64, cap:i64) -> ptr` | 分配并初始化池头（mode 1/2/3，cap 供 fixed；boxed/chunked 忽略 cap）。fixed 时同时分配 data 块 + 位图（elem_size 未定，按 cap*8 预留最大 8 字节槽？——见下注） |
| `core_pool_alloc` | `(pool:ptr, elem_size:i64) -> ptr` | 按档位分槽：fixed 扫位图找空槽（满则返回 0 → codegen 报 abort）；boxed `core_alloc(elem_size,8)`；chunked 找有空槽的 chunk，无则追加新 chunk（旧块不动）。置 full 位，used++，返回槽地址 |
| `core_pool_release` | `(pool:ptr, slot:ptr) -> void` | 清 slot 的 full 位，used--；boxed 档额外 `core_free(slot)`。（槽归还是内部策略，非用户原语） |

**fixed 档的 elem_size 问题**：`Pool.fixed(n)` 时还不知道 T 的大小。方案：fixed 的 data 块按 `n * 8` 预留（与 Rio 槽统一 8 字节对齐的惯例，`rcc_codegen.h` 里 slot 最小 8），`core_pool_alloc` 校验 `elem_size <= 8` 才分槽（当前所有 class 实例都是 8 字节对齐的指针槽，见 `emitConstructorCall:12467` 按 `cls->size` 分配——class 的 size 是字段总字节数，可能 >8）。**修正**：fixed 档改为惰性分配首元素时再一次性分配整块（此时已知 elem_size），池头先记 cap。即 fixed 的 data 块在第一次 `core_pool_alloc` 时按 `cap*elem_size + 位图` 分配，之后不变。

### 2.3 codegen 接线

- **三档构造器**（`rcc_codegen.cpp:9398-9429`）：求值实参后保留值——`fixed(n)` 的 n、`chunked(block:n)` 的 n 进 `%edx`；调 `core_pool_new(mode, cap)` 取代 `core_alloc(8,8)`。mode：fixed=1、boxed=2、chunked=3。
- **`pool.new[T]`**（`:9430-9474`）：校验 receiver/T 后，需要把"对象分配到池槽"传给 `emitConstructorCall`。做法：给 `emitConstructorCall` 加一个可选参数 `const std::string& allocOverride`（方言：空=普通 `emitAlloc`；非空=先 call `core_pool_alloc(pool, size)` 并把结果当对象地址）。`pool.new` 分支把池头地址与 elem_size 传进去，构造器正常清零/初始化/调 ctor，只是对象来自池槽。这样 ctor 逻辑完全复用，改动面最小。
- **`coreReturnType()`**（`:438-463`）：`core_pool_new` / `core_pool_alloc` 登记返回 `getPtrType()`；`core_pool_release` 登记 void。
- **`lib/core/core.rio`**：新增 3 条 extern（带 `core_` 全名）。

### 2.4 验收（Q1）

- `Pool.fixed(2)` + 多次 `new[T]` 不崩、槽地址稳定（同一 Body 变量跨语句地址不变）。
- `Pool.boxed()` / `Pool.chunked(block:2)` 各分配若干对象，正常运行。
- fixed 满了的负向行为（返回 0 → codegen 报清晰诊断，不静默野指针）。
- 现有 Pool 相关用例（L3-K-take/fill、L3-release 系列、A2/C1/C2/C3）全绿。

---

## 3. Q2：成员集合最小集 + release 判据

### 3.1 数据结构（header，仿 `_activeRefs`/`_writeEffects`）

```
enum class Mult { ZERO, ONE, MANY, TOP };  // 重数格，ONE+ONE=MANY，MANY+n=MANY，TOP 吸收
struct MemberSetEntry { std::string containerPath; std::string memberRoot; Mult mult; bool live; };
std::vector<MemberSetEntry> _memberSets;   // 按容器路径索引
```

- **路径索引**：容器局部变量名 / 字段路径（如 `team`、`game.team`）。
- **活跃性**（§6.2/§6.3）：容器此后还会被读/迭代/传出 = live；死变量不计入 release 判据。本阶段用与 `_activeRefs` 相同的语句级 NLL 近似（`drainActiveRefs` 的模板），保守：拿不准算 live。
- **重数饱和**：`ONE+ONE=MANY`；`MANY+n=MANY`；`TOP` 吸收一切。

### 3.2 原语效果（Q2 限定四种）

| 原语 | 效果 |
|---|---|
| `push(x)` / `listAppend(c, x)` / `[x]` 字面量进容器 | `set(c, x) += 1`（饱和） |
| `remove_all(x)` | `set(c, x) = ZERO`（精确） |
| `clear()` | 整个容器集合清空 |
| `snapshot()` / `copy` | 新容器集合 = 原集合拷贝 |

不改这些原语的 ABI/语义，只在 codegen 发射时**登记事实**（与 `applyWriteEffects` 同风格）。

### 3.3 release 判据接入（`emitMethodCall` 的 release 分支 `:12803-12865`）

在现有 9 步之后、成功之前插入第 10 步：
```
for each live container set S in _memberSets:
    if S 含 x 且 mult(x) != ZERO:
        failAt(pos, "L3: cannot release 'x': still a member of live container 'S' (mult=...) (P13-6 / §6.4)")
```
诊断带推断链（规范 §11.4）：容器路径、重数、加入点（若记录）。

### 3.4 语义边界（Q3=A）

- **不放宽状态规则**：探针已证"存进容器后 release"被状态规则挡住（not Empty），本阶段不改。成员判据是防御性机械检查，当前少有触发路径，但判据本身落地。
- Link 条款不查（P13-7）。

### 3.5 验收（Q2）

- 负向：`var team=[a]; ...; pool.release(a)` 若走到 release（需构造可达路径，如通过 Ref 间接持有使 a 保持 Full）→ 报成员集合冲突。
- 正向：`remove_all` / `clear` 后集合归零 → release 通过。
- 重数：`append` 两次 → MANY，`remove_all` → ZERO。
- 快照：`copy` 后两容器各自追踪。

---

## 4. 实施顺序

1. Q1a core 层（`core_win64.s` + `core.rio` + `coreReturnType`）→ 单独编译验证 `as` 汇编通过。
2. Q1b codegen 三档构造器 + `emitConstructorCall` 参数化 + pool.new 接线 → 跑 Pool 相关用例。
3. Q2 成员集合数据结构 + 四原语登记 + release 判据 → 跑成员集合用例。
4. 建 `tests/agent/p13-6-pool` 回归套件，跑全量 13+ 套。
5. 更新 `p13_plan.md` / `pending_designs.md` / 记忆，清理探针。

---

## 5. 风险与回退

- **汇编风险**：新 core 例程若违反 callee-saved/栈对齐会静默崩溃（历史教训：ABI 违规导致元素变 0）。每加一个例程立即用最小 Rio 程序实测，不攒批。
- **fixed 惰性分配**：与"fixed 最快"的设计意图略偏（首元素时多一次分配），但对外语义不变（容量固定、地址稳定）。若用户不认可，可回退为"fixed 构造时按默认 8 字节槽预留"。
- **成员集合活跃性**：保守近似可能误报（把死容器算 live）；本阶段宁可漏报不可误报（Q2=A 服务 release 判据，误报会挡合法程序）。
