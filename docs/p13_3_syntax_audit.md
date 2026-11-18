# P13-3 前置审计：能力类型降级链与 `take` 实参语法

> **本文性质**：审计 + 裁决记录，**不是实施方案，未改任何 codegen**。
> 遵守 RCC 硬规则（`1b1ccafb`）：任何语言设计变更必须先向用户提出并获批。
> 审计阶段：①把规范 §3 / §8.5 的主张对照 `release\RCC.exe` 实测；②标出与现有语法的正面冲突；③列出必须先裁决的问题。
> **裁决状态（2026-11-19）：Q1–Q5 已全部逐条讨论并裁决，见 §6。**
>
> 审计基线：`release\RCC.exe`（2026-11-18 23:49:27，33711868 字节），HEAD `7daa181`。
> 每个结论均有可复现探针（`build\p13_3_probe\*.rio`）。
> 前置依据：`docs/p13_plan.md` §9.2、`docs/pending_designs.md` §6、`docs/memory-model-spec-full.md` §3/§8.5。

---

## 6. 用户裁决记录（2026-11-19，逐条讨论确认）

> 逐条讨论方式由用户指定：一次一个待裁决项、给解释与推荐项、确认后再进下一项。
> 五项全部裁决完毕，可作为实施方案的权威依据。

| 项 | 主题 | 裁决 | 核心口径 |
|---|---|---|---|
| Q1 | 降级链形态 | **A** | `&` 恒产 Ref 不动；只放宽 `checkAssignable`（`codegen.cpp:671-675`）允许 `MUT→REF`、`TAKE→MUT`、`TAKE→REF`（含传递，inner 须一致），禁止一切升级；裸 Body 既有隐式转换不变 |
| Q2 | `Content[T]` 与 `Take[T]` 关系 | **A** | 分工保持：`Take`=独占壳子的许可（二等、实参位创建），`Content`=取出的内容值（可绑定/移动/落回壳子）；`f(take x)`→Take、`x.take()`→Content，互不混用、互不转换；`eat(pl.take())` 保持报错 |
| Q3 | `f(take x)` 语法 | **D**（用户修正了原 A） | **不引入新语法**。实测裸 Body 传 `Take` 形参已可行（`eat(pl)`，探针 z1/z2/z3），`Take` 入口已存在；规范 §3.3 的 `f(take x)` 用「形参声明 `Take[T]` + 传裸对象」等价替代，须在文档认可此等价。否掉专用 token / parser 改动 |
| Q4 | §8.5 调用规则深度 | **A**（完整三条，用户推翻了推荐的保守版 B） | ①不重叠：做路径/别名分析判断实参是否可能同槽，除非能证明不同否则拒绝；②不逃逸：被调函数内 Take 形参只用于内建转移/降级传出/再传；③后置状态：接现有 `ensures`（缺省 `?`）。**注**：别名/路径追踪基础设施与 P13-4 子引用的路径登记是同一套，等于提前拉入 P13-4 部分地基，避免了保守版返工 |
| Q5 | `sout(Ref[T])` 打印 | **保持现状**（用户自定义口径，非 A/B） | `sout` 是底层原语，不对数据做语义转换，保持其引用输出格式（打印引用本身，视为"引用的输出格式"而非 bug）；**之后另行提供输出引用指向值的方法**，属新增 API/语言特性，按硬规则单独提案，本阶段只登记不实施 |

**审计修正记录**（相对 §9.2 的表述）：
- §3.3「Take 二等值」的「不可绑定/不可返回」**已实现**为负向检查（`L3-Take-var` / `L3-Take` 均 `expectFailure:true`），非缺口。
- 裸 Body → Ref/Mut/Take 形参的隐式转换（`checkAssignable:689-700`）**已实现**，故 Q3 无需新语法。
- 真正缺口收敛为：降级链（Q1）、调用规则（Q4）、`Content`↔`Take` 边界（Q2 已定边界）；`sout` 按 Q5 不修。

**后续待设计项**（本阶段登记，不实施）：输出引用所指值的新方法（Q5 引出，新增语言/标准库 API，须单独提案获批）。

## 0. 结论摘要

P13-3 的总体判定仍是「**1/3**」，但缺口比 §9.2 的表述更精确：

- **已经闭合的部分**（无需再裁决）：五种能力类型能做形参标注；`&x` / `p.ref()` / `p.mut()` / `x.take()` / `list.get_mut()` 的产出类型正确；**§3.3「Take 二等值」的「不可绑定 / 不可返回」已实现为负向检查**（回归例 `L3-Take-var` / `L3-Take` 为 `expectFailure:true`）。
- **真正缺口**（本提案的对象）：
  1. **降级链 `Take→Mut→Ref` 完全未接通**（`MemoryType↔MemoryType` 一律拒绝）；
  2. **`f(take x)` 实参语法零基础**（`take` 只是普通标识符）；
  3. **§8.5 `Take` 实参调用规则**（不重叠/不逃逸/后置状态）未实现；
  4. **`sout(Ref[T])` 打印指针地址**的小缺陷（§9.2 附带发现）。

**一个必须先澄清的语义分叉**：规范里「降级」其实有两种完全不同的来源，实测发现其中一种已经以另一种名字存在：

| 规范写法 | 含义 | 现状 |
|---|---|---|
| §3.2「`Take→Mut→Ref` 降级」 | 能力类型值之间的转换 | ❌ 全断 |
| §7.3「Body 实参隐式取引用 / take」 | 裸 Body 实参按形参类型自动变成 Ref/Mut/Content/Take | ✅ 已实现（`heal(pl)` 直接过，`pl` 是裸 Body） |

由于「裸 Body → Mut 形参」已经能工作（探针 `q5_bare_to_mut`：`sout(heal(pl))` 打印 `bob`），很多人期待的「`&pl` 传给 Mut 形参」在语义上是否还需要，取决于用户对**取引用 `&` 到底产出什么**的裁决（见 Q1）。

---

## 1. 逐项实测对照

### 1.1 降级链 — ❌ 完全未实现

| 探针 | 写法 | 结果 |
|---|---|---|
| `p1_ref_ok` | `show(&pl)`，`show(p: Ref[Player])` | ✅ 打印 `bob` |
| `p2_ref_to_mut` | `heal(&pl)`，`heal(p: Mut[Player])` | ❌ `cannot assign Ref[Player] to Mut[Player]: different memory roles` |
| `q3_mut_to_ref` | `show(pl.mut())`，`show(p: Ref[Player])` | ❌ `cannot assign Mut[Player] to Ref[Player]` |
| `q4_content_to_ref` | `show(pl.take())`，`show(p: Ref[Player])` | ❌ `cannot assign Content[Player] to Ref[Player]` |
| `q5_bare_to_mut` | `heal(pl)`（裸 Body 实参） | ✅ 打印 `bob` |

**核心代码**：`code/src/backend/rcc_codegen.cpp` 的 `CodeGen::checkAssignable`（627–826），关键在 667–710：

```cpp
if (tr >= 0 && sr >= 0) {
    if (tr != sr)                          // ← 671-675：角色必须完全相同
        return "cannot assign ... : different memory roles";
    // 角色相同才比 inner->equals()
}
// 唯一的跨角色通道：683 行 Body↔裸类名别名；689-700 行「形参带角色 + 实参是裸 ClassType」的隐式 ref/mut/take
```

`MemoryType::equals`（`code/include/sema/rcc_sema_types.h:423-430`）也严格要求 role 相等。
**结论**：接通降级链的单点改动位置是 `checkAssignable` 的 671–680，配套 `MemoryType::equals` 的语义口径要一并确认。

### 1.2 `f(take x)` 实参语法 — ❌ 零基础

| 探针 | 写法 | 结果 |
|---|---|---|
| `p3_take_arg` | `eat(take pl)` | ❌ 语法错误（`take` 后 `pl` 未闭合 / `TOKEN_RPAREN` 无前缀构造） |

**代码事实**：
- 词法表（`code/src/rcc_core.cpp:32-106`、`code/src/rcc_base.cpp:15-19`）**没有 `take` 关键字**，`take` 目前是普通标识符，仅作为 `.take()` 方法名字符串存在。
- 实参解析走 `buildRangerExpression`（`code/src/analyzer/parser/builder/rcc_prefix_builder.cpp:486-554`），无专门的 take 前缀节点。
- 支持 `f(take x)` 需**词法 + parser + codegen 三层新增**。

### 1.3 §8.5 `Take` 实参调用规则 — ❌ 未实现

规范 §8.5 要求：①不重叠（其余实参不得同槽 Ref/Mut/Take）；②不逃逸（被调函数内 Take 只能转移/降级/再传）；③后置状态以 `ensures` 为准（缺省 `?`）；④经容器的间接访问安全。检索无对应检查。`ensures` 标注本身在 P13-2 已可用（`fun f(t: Take[P]) ensures[!t]`），但**实参层面的不重叠/不逃逸校验**没有。

### 1.4 `sout(Ref[T])` 打印指针地址 — ❌ 附带 bug

| 探针 | 写法 | 结果 |
|---|---|---|
| `p6_ref_sout` | `sout(r)`（`r: Ref[str]`） | ❌ 打印 `0x000001c18fe39070` 而非字符串内容 |

**原因**：`code/src/backend/rcc_codegen.cpp:9620-9630` 的 `sout` 类型分派末端，`isPointerLike()`（`rcc_sema_types.h:63-69`）把 `TK_MEMORY` 算作指针型，命中 `rt_print_ptr`；无任何针对 `TK_MEMORY` 的解引用/按 inner 打印逻辑。

---

## 2. 与现有 Rio 语法 / 已实现语义的冲突

1. **取引用 `&` 的产出已固定为 `Ref`**（`rcc_prefix_builder.cpp:44-65`、`codegen.cpp:3075-3109`）。规范 §3.2 若要求「`&x` 能按目标类型降级成 `Mut`」，就必须改变 `inferType` 让 `&` 的产出类型**上下文相关**——这与现有「`&` 恒产出 Ref」冲突，是一个**语义层裁决**（见 Q1）。
2. **`x.take()` 产出 `Content[T]`**（`codegen.cpp:3250-3260`），而规范 §3.2 的 `Take` 是独立能力角色。`Content[T]` 与 `Take[T]` 是**两个不同 role**，实测 `eat(p.take())` 报 `cannot assign Content[Player] to Take[Player]`。二者是否应当统一/可转换，必须裁决（见 Q2）。
3. **「裸 Body → 能力形参」隐式转换已存在**（§7.3，探针 q5），降级链若按「值之间转换」实现，会与这条既有通道在语义上重叠，需要明确优先级（见 Q1）。

这些都**不是**实现细节，而是语言语义选择，按硬规则必须先裁决。

---

## 3. 待裁决问题

> 以下问题互不依赖，可逐条答复；每条给出推荐选项。答复后我才据此写实施方案并动 codegen。

### Q1：`&x` 的产出与降级链的形态

规范 §3.2 要求降级 `Take→Mut→Ref`，但现状是「`&` 恒产出 Ref」+「裸 Body 按形参隐式转能力」。请选降级链的实现形态：

- **A（推荐）**：`&x` 保持恒产出 `Ref`；降级链只在**显式持有能力值**时生效——即 `Mut` 值/`Take` 值能传给 `Ref` 形参（放宽 `checkAssignable`：允许 `MUT→REF`、`TAKE→MUT`、`TAKE→REF`，禁止反向）。裸 Body 的既有隐式转换不变。这样改动最小、不碰 `&` 的语义，覆盖规范「只能降级」的核心。
- **B**：让 `&x` / `p.mut()` 的产出类型**上下文相关**（形参要 Mut 就产 Mut），等价于扩展现有 §7.3 隐式转换到「显式能力表达式」。灵活但改动 `inferType` 语义，风险更高。
- **C**：严格按字面实现 `Take→Mut→Ref` 全部方向 + 禁止一切升级，并把 `Content` 也纳入降级源（需配合 Q2）。

### Q2：`Content[T]` 与 `Take[T]` 的关系

规范 §3.3 说 `Take` 是二等值，只能在实参位置由 `f(take x)` 创建；现状 `x.take()` 产出的是 `Content[T]`（可绑定、可移动）。

- **A（推荐）**：二者分工保持——`Content[T]` 是「取出的内容」（可绑定/转移/落回壳子），`Take[T]` 是「临时借用壳子的独占能力」（只在实参位创建）。`f(take x)` 产 `Take[T]`，`x.take()` 产 `Content[T]`，互不混用。这最贴合规范。
- **B**：统一二者——`x.take()` 直接产出 `Take[T]`，废弃 `Content` 作为独立角色（会牵动 P13-2 大量既有用例与 §3.5 内容转移语义，**不推荐**）。

### Q3：`f(take x)` 语法的落地范围

- **A（推荐）**：新增 `take` 为**上下文关键字**，只在实参位置（`f(take x)`）识别为 Take 能力实参；方法调用 `x.take()` 与标识符 `take` 在其他位置保持原义。词法层只在实参解析处识别，避免污染普通标识符。
- **B**：新增全局 `take` 保留关键字（禁止任何变量叫 `take`），改动词法表但更简单粗暴。
- **C**：本阶段先不做 `f(take x)`，只接通降级链（Q1）与调用规则（Q4），`f(take x)` 推到 P13-3 尾期。

### Q4：§8.5 调用规则的实现深度

- **A（推荐）**：完整实现①不重叠 + ②不逃逸 + ③后置状态缺省 `?`（复用 `ensures` 机制）；这是规范验收要求。
- **B**：本阶段只做①不重叠（最关键、防读空壳），②③推后。
- **C**：本阶段不做调用规则，只做类型层降级 + 语法。

### Q5：`sout(Ref[T])` 打印行为

- **A（推荐）**：修复——`sout` 对 `TK_MEMORY` 的 REF/MUT 按 inner 类型解引用后打印内容（与裸 Body 打印一致）；`Take`/`Content` 同理或明确报错。
- **B**：保持打印指针地址（当作调试特性），只记录不修。

---

## 4. 探针复现方式

```powershell
python build\p13_3_probe_gen.py        # 生成 p1..p6
python build\p13_3_probe_gen2.py       # 生成 q1..q4
# 逐个编译：
.\release\RCC.exe -build --path build\p13_3_probe\<name>.rio --output build\p13_3_probe_out\<name>.exe
```

---

## 5. 本文档边界

**已确定（有出处）**：规范 §3/§8.5、§9.2 实测、探针结果、代码定位。
**未裁决**：Q1–Q5，答复后形成实施方案才动 codegen。
**明确不做**：本轮不改任何代码、不改标准库公开接口面。
