# P13-3 实施方案：能力类型降级链 + `Take` 实参调用规则

> **本文性质**：实施方案，**待用户确认后才动 codegen**。
> 设计依据：`docs/p13_3_syntax_audit.md` §6（Q1–Q5 用户裁决，权威口径）。
> 基线：`release\RCC.exe`（2026-11-18 23:49:27），HEAD `7daa181`，工作区有未提交改动（见 §7）。
> 目标：把 P13-3 从「1/3」推进到「完成」——接通降级链、落实 §8.5 调用规则；不引入新语法、不改 `sout`。

---

## 1. 范围总览

| 任务 | 对应裁决 | 性质 | 改动文件 |
|---|---|---|---|
| T1 降级链 `Mut→Ref`、`Take→Mut/Ref` | Q1=A | 类型兼容性放宽 | `rcc_sema_types.h`、`rcc_codegen.cpp` |
| T2a §8.5 不重叠（别名/路径分析） | Q4=A | 新增调用点检查 | `rcc_codegen.cpp`、`rcc_codegen.h` |
| T2b §8.5 不逃逸（函数体内 Take 形参用法约束） | Q4=A | 新增体内检查 | `rcc_codegen.cpp`、`rcc_codegen.h` |
| T2c §8.5 后置状态接 `ensures` | Q4=A | 复用现有机制，补调用点推导 | `rcc_codegen.cpp` |
| T3 回归套件 | 全部 | 新增测试 | `tests/agent/p13-3-capability/` |

**明确不做**（裁决已定）：`f(take x)` 新语法（Q3=D，用 `eat(pl)` 等价替代）、`sout` 打印行为（Q5 保持）、`x.take()`→Content 语义（Q2=A 保持）、P13-2 现有语义与标准库公开接口。

---

## 2. T1：降级链（Q1=A）

### 2.1 改动点

**（a）类型判定单一入口 `checkAssignable`** — `code/src/backend/rcc_codegen.cpp:627-826`

现有逻辑（667-681）：两侧都是 `TK_MEMORY` 时，`tr != sr` 一律报错。改为：

```cpp
if (tr >= 0 && sr >= 0) {
    // 新增：允许的降级方向（角色数值越大权限越高：BODY<CONTENT<REF<MUT<TAKE 待确认顺序）
    // 采用显式白名单，避免依赖枚举顺序：
    //   TAKE -> MUT / REF ; MUT -> REF （含传递 TAKE -> REF）
    if (!isAllowedDowngrade(targetRole, sourceRole))
        return "...: different memory roles (only Take→Mut→Ref downgrades are allowed)";
    // inner 类型仍须一致（现有 676-680 不动）
}
```

关键：`isAllowedDowngrade(t, s)` 用**显式白名单**，不依赖 `MemoryRole` 枚举的数值顺序（当前枚举是 `BODY,CONTENT,REF,MUT,TAKE`，数值顺序与权限顺序不一致，不能简单比大小）。白名单 = `{(MUT,REF),(TAKE,MUT),(TAKE,REF)}`。

**（b）`MemoryType::equals` 口径** — `code/include/sema/rcc_sema_types.h:423-430`

`equals` 严格 role 相等是"类型相同"的语义，**不应改**（`var a: Mut[T] = b` 仍要求精确类型）。降级只在 `checkAssignable` 放行的调用/赋值场景生效，不混淆"类型相等"与"可赋值"。故 T1 **不改 equals**，仅在方案里记录此口径。

**（c）核对 10 个 `checkAssignable` 调用点**

`checkAssignable` 有 10 个调用点（变量定义、赋值、实参绑定 8417、返回等）。放宽后所有场景统一获得降级能力，符合规范"降级可用于任意传值"。重点回归：`emitArgsToRegisters:8417`（已有 Take 形参加 Body 实参落定 8504-8527）需确认降级后的 `TAKE→MUT/REF` 不会干扰隐式 take 落定。

### 2.2 语义边界（Q1=A 明确）

- `Mut→Ref`、`Take→Mut`、`Take→Ref`（含传递）放行；
- `Ref→Mut`、`Ref→Take`、`Mut→Take` 等一切**升级**继续拒绝；
- `Content` 不加入降级源（Q2=A）；
- 裸 Body → 能力形参的既有隐式转换（689-700）不变。

### 2.3 验收

- 正例：`show(pl.mut())`（Mut→Ref）、`f_take` 体内把 Take 形参传给 Ref/Mut 形参。
- 反例：`heal(&pl)`（Ref→Mut）仍报升级错误；`eat(pl.take())`（Content→Take）仍报不匹配。

---

## 3. T2：§8.5 调用规则（Q4=A）

> 这是 P13-3 的大头。核心是复用 P13-2 已建好的**路径/别名基础设施**，并新增调用点检查。

### 3.1 可复用的现有设施（已实测存在）

| 设施 | 位置 | 用途 |
|---|---|---|
| `exprPathText(node)` | `codegen.cpp:1643-1667` | 表达式 → 路径文本（`p`、`p.name`、`xs[0]`）；调用时取各实参的槽路径 |
| `pathIsPrefixOf(p,q)` | `codegen.cpp:1668-1682` | 按路径段判定前缀（含相等），`p` vs `p.name` 正确识别 |
| `ActiveRef{path,owner,refVar,indexExpr,loText,hiText}` | `codegen.h:488-498` | 活跃引用事实表，含动态下标、区间视图边界 |
| `checkWriteConflicts` | `codegen.cpp:1683-1709` | P=Q 前缀即冲突的既有判据 |
| `rangesDisjoint` / `hasActiveRefOn` | `codegen.cpp:1911+` | 区间/下标"可证明不相交"的判定（i≠j 事实） |
| `_ensuresMarks` + `checkEnsuresAtExit` | `codegen.cpp:1229-1271 / 1337-1360` | ensures 解析与出口校验（已对 Take 形参生效） |
| `takeReceiverName` | `codegen.cpp:5887-5901` | 识别 `X.take()` 的接收者路径 |
| `emitArgsToRegisters` | `codegen.cpp:8353-8545` | 实参绑定现场；Take/Content 形参接 Body 实参的落定在 8504-8527 |

### 3.2 T2a 不重叠（别名/路径分析）

**判据**：调用 `f(take x, args...)`，取 Take 实参的源路径 `ptake`（`eat(pl)` → `"pl"`；`eat(pl.take())` → `takeReceiverName` → `"pl"`），逐个检查其余**能力类**实参（`Ref`/`Mut`/`Take` 形参对应的实参）的路径 `pj`。两者冲突当且仅当**无法证明不同**，即：

1. `pj` 为空（无法解析路径）→ 冲突（保守）；
2. `pathIsPrefixOf(pj, ptake) || pathIsPrefixOf(ptake, pj)` → 冲突（同一 slot 或子路径重叠）；
3. 两者是同一 owner 的动态下标路径且**不能证明不相交**（复用 `hasActiveRefOn` / `rangesDisjoint` 的 i≠j 判据）→ 冲突；
4. 否则（两个静态不同路径，如 `p` vs `q`）→ 可证明不同，放行。

**实现形态**：新增 `checkTakeArgOverlap(fname, args, paramTypes, pos)`，在 `emitArgsToRegisters` 的实参类型校验循环（8403-8424）**之前**调用；需要能拿到形参的角色——从 `paramTypes[i]` 的 `MemoryType::getRole()` 判定；缺少形参类型信息（未标注/重载未决议）时保守跳过并记 TODO。

**错误信息**：`Argument N of 'f': take-argument 'pl' overlaps with argument M ('&pl') — both may refer to the same slot (P13-3 / §8.5 rule 1)`。

### 3.3 T2b 不逃逸（函数体内 Take 形参用法约束）

**规则**：函数体内，`Take[T]` 形参 `t` 只允许：
- 作为内建转移操作的接收者：`t.take()` / `t.fill(c)` / `t.clear()` / `t.move_into()`；
- 作为实参传给另一个函数的 `Take` 形参（可再传下去）；
- 降级为 `Mut`/`Ref` 临时传出（Q1 放行后的用法）。

禁止：
- 绑定到变量（`var x = t`）—— **Q2 已有类型级负向检查**（`L3: Take[T] cannot be used as the type of variable`），但那是"声明为 Take 类型"，还需挡住 `var x = t`（推断出 Take 类型的绑定）；
- 存入容器/结构体字段；
- 作为返回（`ret t`）—— Q2 已有类型级检查，需确认表达式路径也挡住；
- 被逃逸闭包捕获。

**实现形态**：
- 新增 `checkTakeParamUsage(name, usageKind, pos)`（或复用/扩展 `checkBorrowNotEscaping:2011`，但语义不同：借用"完全不能移动"，Take"可转移不可存储/返回"——建议独立函数）；
- 在变量定义、赋值、返回、闭包捕获的发射点，对源表达式做 `isTakeParam(name)` 判定（用 `isFunctionParam` 1361 + 查形参类型 role==TAKE），命中禁止用法即报错。
- 简化落地：本轮先覆盖"绑定到变量/赋值/返回/存容器"四类高频逃逸；闭包捕获路径若现有设施难接，登记为 TODO 并在用例标注。

### 3.4 T2c 后置状态接 `ensures`

现状：`_ensuresMarks` 在**被调函数**出口校验（1337-1360），调用点**不推导**后置状态。Q4=A 要求"以被调函数 ensures 为准，缺省 ?"。

**实现形态**：
- 收集被调函数的 ensures 承诺（解析时已存，需确认是否有按函数名的可查询表；若无，新增 `_functionEnsures: fname -> map<paramName, BodyState>` 于函数签名收集阶段填充）；
- 调用返回后，把"被 take 的实参"的壳子状态设为 ensures 承诺的状态（缺省 `?` = `BodyState::UNKNOWN` + 标记 merged-unknown，使后续条件销毁规则按 §8.4 处理）；
- 这样 `eat(pl)` 之后 `pl` 的状态由 `fun eat(p: Take[P]) ensures[!p]` 决定为 Empty，缺省则 `?`。

> **风险提示**：缺省 `?` 会让 `eat(pl); sout(pl.name)` 之后的访问按"不确定"处理（规范要求改用 `get()`）。这符合规范，但会改变现有部分用例的行为——实施时须跑全量回归，把受影响的用例按规范更新（不是"顺手改测试让它过"，而是按 §3.4 访问规则表修正）。

---

## 4. T3 回归套件 `tests/agent/p13-3-capability/`

沿用现有数据驱动格式（`execute.ps1` 转接 `_shared/driver.ps1`，`code/cases.json` + `code/*.rio`）。用例清单：

| 分组 | 用例 | 期望 | 覆盖 |
|---|---|---|---|
| 降级正例 | `down_mut_to_ref` | 通过，输出正确 | Mut→Ref |
| | `down_take_to_ref`（函数体内） | 通过 | Take→Ref |
| | `down_take_to_mut`（函数体内） | 通过 | Take→Mut |
| 降级反例 | `up_ref_to_mut` | 失败，报升级错误 | Ref→Mut 禁止 |
| | `up_ref_to_take` | 失败 | Ref→Take 禁止 |
| | `content_to_ref` | 失败 | Content 不入降级源（Q2=A） |
| 既有语义回归 | `bare_body_to_take` | 通过 | Q3=D 的 Take 入口 |
| | `content_take_still_works` | 通过 | Q2=A `x.take()`→Content 不变 |
| 不重叠正例 | `overlap_distinct` | 通过 | `f(pl, &q)` 不同 slot |
| 不重叠反例 | `overlap_same_var` | 失败 | `f(pl, &pl)` |
| | `overlap_same_mut` | 失败 | `f(pl, pl.mut())` |
| | `overlap_unknown_path` | 失败 | 无法解析路径，保守拒绝 |
| 不逃逸反例 | `escape_bind` | 失败 | `var x = t`（Take 形参绑定） |
| | `escape_return` | 失败 | `ret t` |
| | `escape_store` | 失败 | Take 形参存容器 |
| 不逃逸正例 | `take_passthrough` | 通过 | Take 形参再传给 Take 形参 |
| 后置状态 | `ensures_empty` | 通过/失败按断言 | `ensures[!p]` 生效 |
| | `ensures_default_unknown` | 验证 `?` | 缺省后置状态 |
| Q5 记录 | `sout_ref_prints_addr` | 保持输出引用格式 | 记录行为，非断言内容 |

预计 ~20 例。

---

## 5. 改动文件清单（预估）

| 文件 | 改动 |
|---|---|
| `code/src/backend/rcc_codegen.cpp` | checkAssignable 降级白名单；checkTakeArgOverlap；checkTakeParamUsage；ensures 调用点推导；emitArgsToRegisters 插入调用 |
| `code/include/backend/rcc_codegen.h` | 新增上述方法声明；必要时新增 `_functionEnsures` 成员 |
| `code/include/sema/rcc_sema_types.h` | 不改 equals；仅确认 role 枚举用途（T1 口径已定不改） |
| `tests/agent/p13-3-capability/*` | 新增套件 |
| `docs/p13_plan.md` | §9.2 状态更新为完成（实施后） |
| `docs/pending_designs.md` | 移除 P13-3 待设计项；登记"输出引用值方法"为后续项（Q5） |

---

## 6. 风险与开放点

1. **别名分析误伤**（最高风险）：Q4=A 的"不能证明不同就拒绝"是保守判据，可能拒绝合法调用。缓解：路径判据复用 P13-2 已验证的 `pathIsPrefixOf`/`rangesDisjoint`；实施中每类判据配正/反用例。若发现大面积误伤，回来与用户确认是否放宽（**不自行降级为保守版**——Q4 已裁决 A）。
2. **缺省 `?` 后置状态改变现有行为**：`take` 后访问壳子可能被拒。按规范 §3.4 修正用例，不改编译器语义。
3. **不逃逸的闭包捕获**：若现有设施难接，本轮先覆盖四类高频用法，闭包路径登记 TODO（须在交付说明中如实标注，不隐瞒）。
4. **`_functionEnsures` 是否需要新表**：取决于现有签名收集阶段是否已保存 ensures 供调用点查询；实施时先核实（记忆/代码），避免重复造表。
5. **与 P13-4 的边界**：路径/别名分析是与 P13-4 共用的地基，实现时写成独立可复用组件（不硬编码在 take 调用点），为 P13-4 铺路。

---

## 7. 工作区状态提醒

当前工作区有一批未提交改动（docs/、tests/ 各执行脚本、`tests/cases/temp-test/` 等，见 `git status`）。这些与本方案无关，实施时**不触碰、不混入 P13-3 提交**。提交 P13-3 时只 add 本方案涉及的文件。

---

## 8. 确认项

请确认本方案后我再动手。若有调整，重点看：
- §3.2 不重叠判据的保守程度（是否接受"无法证明不同即拒绝"）；
- §3.3 不逃逸本轮覆盖范围（闭包捕获是否必须本轮做）；
- §3.4 缺省 `?` 后置状态对现有用例的影响（是否接受按规范修正用例）。
