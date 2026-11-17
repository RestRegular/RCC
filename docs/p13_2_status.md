# P13-2 实施总结（2026-11-18）

> 本文记录 P13-2 内存模型语法的实施状态，基于本会话 33 个提交。
> 所有变更均来自已批准的设计文档 `memory-model-syntax-resolutions.md`
> 与 `p13_2_syntax_resolutions.md`，未自行设计语法。

## 已完成（按依赖顺序）

| 阶段 | 提交 | 内容 |
|---|---|---|
| L0 去伪 | 16c5cf6 | 7 项静默错误改编译报错 |
| L1 地基 | 2bfe1d2 / 84d4a91 | ret/return 拆 token 与退最近作用域；裸块识别（Q-D） |
| L2 结构语义 | c3cdcdb / fa5f3ce / b8d16d3 / 9b5e542 | if/while `: T` 标注校验；break/ret value 值合流；if 内 ret value |
| L3 地基 | 3620e94 / 24ea573 | 五类型进 sema（Body/Content/Ref/Mut/Take）；Take 位置约束 |
| L3 移动 | bda8c72 / 9ac487c / 680689a / 62b17bf / 60d4cf6 | Content 移动；移动状态表；Content 形参 take；& 前缀；ref/mut；take 落定 |
| A Pool | 0122e10 / 85ab087 | Pool 类型骨架 + 标签区出生地识别 |
| B pool.new | c2c462e | `pool.new[T](...)` 语法形态 + 出生地一致性检查 |
| C 四态 | 9fa9ae6 / 01bfcbe | E/F/H/X 事实表；fill/clear/release 内建方法；take 状态修正（Body→E） |
| D 挂起态 | f49e008 | pending_take + 独占检查 + 独立 take 警告 |
| E 子引用 | 07c8ba0 / 5e85a3d / 9b0017d / 5a800ec / 645ad6b | 子引用类型+地址；路径表+冲突；NLL；穿透展开；get_mut+不等事实 |
| F 诊断 | ba1f99b / 9ebc47b | 子句进 AST + from 检查；ensures 语法校验 |

**验证**：`tests/p13_2_regression.ps1`，56 条用例全 PASS；每次提交后回归。

## 剩余项与状态

| 项 | 状态 | 原因 |
|---|---|---|
| ensures 逐出口验证（§5.1） | 未做 | 需 Take 形参状态跟踪 + 出口分析；语法校验已做 |
| writes 泛型合流（§5.3） | 未做 | 需泛型定义/实例化的效果表；决议标为实现细节 |
| split(k)（§4.3） | 未做 | 需多返回值解构；独立增量 |
| Maybe/if-let/match（§6.1） | **设计文档明确暂缓** | Q3 机制待用户提供设计 |
| 条件销毁属性语法（§5.4） | **设计文档明确暂缓** | 归 Q4 条件销毁，待设计 |
| 值语义 struct（§6.2） | **设计文档明确暂缓** | struct 声明语法待用户提供 |

## 判断

- objective 中「L0→L1→L2→L3→L4」各阶段的**主体**均已落地。
- L4 的「可选类型」部分被设计文档明确推迟，不属当前可实施范围。
- 剩余 3 项（ensures 逐出口、writes 合流、split）是已确认语义的独立硬骨头，
  不影响已交付功能的正确性；可在后续轮次继续。
