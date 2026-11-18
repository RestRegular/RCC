# P12-3-C：`ds` 模块设计文档

> 状态：设计已定稿（2026-11-17 用户批准）
> 范围：`lib/std/ds.rio` 的 API 面、语义契约、当前能力边界
> 前置：P12-3-A（骨架）、P12-3-B（io/str）已完成
> 关联：内存模型阶段归属见 §6

---

## 1. 定位与总原则

`ds` = **data structures**，面向 `list` / `dict` 的高频操作。

三条总原则：

1. **不为历史兼容制造技术负担**（用户 2026-11-16 裁决）。旧 `ds.rio` 里靠
   `encapsulated` / RA 指令 / `dt.setType` 实现的部分一律不保留。
2. **标准库不得依赖闭包写回**（`p12_plan.md §4.3.3`）。G6 已延期到 P13，
   所以 `iter` / `forInRange` 必须写成**纯函数式契约**（返回累计值），
   不能是「回调里改外层变量」。
3. **能用语言内建表达就不进标准库**。`list + list`、`a[i] = v` 已由后端直接支持，
   `ds` 不重复提供。

---

## 2. 当前能力边界（实测，2026-11-17）

这一节是 `ds` 实现的**硬约束**，每条都实测过。

### 2.1 可用的语言能力

| 能力 | 状态 | 实测 |
|---|---|---|
| `list` 字面量 / 索引读 / 索引写 | ✅ | `[1,2,3]`、`xs[1]`、`xs[0]=9` |
| `list` 拼接 `a + b` | ✅ | `[1] + [2]` → size 2 |
| `list += [x]` | ✅ | 追加到新 list（**不共享底层**） |
| `dict` 字面量 / 索引读 / 索引写 | ✅ | `{"a":1}`、`d["a"]`、`d["b"]=2` |
| `size(x)` 对 str/list/dict | ✅ | 3 / 4 / 2，读 +0 长度头 |
| `core_mem_copy` / `core_mem_set` | ✅ | 指针算术 + 逐字节写 |
| 泛型 `[T]` + 单态化 | ✅ | int/str/float 各自实例化 |
| 泛型函数默认参数 | ✅ | **本轮修复**（见 §2.3） |
| 闭包作为回调参数 | ✅ | 可调用，但**不能写回外层** |
| `toStr(x)` | ✅ | `toStr(42)` → `"42"` |
| 字符串拼接 `s1 + s2` | ✅ | `"a" + "b"` → `"ab"` |

### 2.2 不可用的能力（决定 API 形态）

| 能力 | 状态 | 影响 |
|---|---|---|
| 闭包写回外层变量 | ❌ G6 延期 | `iter`/`forInRange`/`repeat` 不能靠回调累加 |
| 运行时类型信息 `type(x)` | ❌ 已删 | `dictKeys`/`dictValues` 无法按元素类型返回 |
| `list[int]` 与 `list[str]` 重载 | ❌ 被视为重复定义 | `sort` 只能用泛型，不能重载分派 |
| 原地追加越过长度头 | ❌ 索引写有越界检查 | 见 §2.3 |
| `core_free` | ❌ 从未被调用 | 容器只分配不释放 |
| 作用域退出析构 | ❌ codegen 无此机制 | 见 §6 |

### 2.3 本轮修复：泛型默认参数

**缺陷**：`fun f[T](x: T, n: int = 42)` 调用 `f(1)` 返回栈垃圾（`2769425277904`）。

**根因**：`emitFunctionDefinition` 的泛型分支在登记 `_functionParams` 前就 `return`，
且 `GenericFunction` 结构体没有 `defaults` 字段。

**修复**：`GenericFunction` 加 `defaults`；声明分支收 `collectParamDefaults`；
`instantiateGeneric` 按 mangled 名登记 `_functionDefaults`。

**结果**：`sort(xs, reverse=false)` 这种旧 API 形态现在可以直接写，不必拆成两个函数。

### 2.4 原地增删的边界（重要）

实测结论：

- **原地删除可行**：左移覆盖 + 缩短长度头。`[10,20,30]` 删下标 1 → `[10,30]` ✅
- **原地追加不可行**：`emitIndexAssignValue` 在 `rcc_codegen.cpp:3051-3053` 有
  `cmp (%rcx), %rax` / `jae skip` 越界检查，手工改大长度头后 `xs[n] = v` 仍被跳过。

因此：

| API | P12 实现 | P13 目标 |
|---|---|---|
| `listRemove` | 原地删（左移 + 缩短） | 不变 |
| `listAppend` | 返回新 list | 改为原地追加 |

---

## 3. API 面（共 23 个）

### 3.1 保留（15 个）

| API | 签名 | 说明 |
|---|---|---|
| `size` | 复用 `str.size(arg: any): int` | 不重复定义 |
| `isEmpty` | `fun isEmpty(arg: any): bool` | `size(arg) == 0` |
| `top` | `fun top[T](xs: list[T]): T` | 末元素，空列表返回默认值 |
| `copy` | `fun copy[T](xs: list[T]): list[T]` | 浅拷贝（对应内存模型的 `clone` 语义） |
| `repStr` | `fun repStr(s: str, times: int): str` | 重复字符串 |
| `repList` | `fun repList[T](x: T, times: int): list[T]` | 重复元素构造列表 |
| `listAppend` | `fun listAppend[T](xs: list[T], item: T): list[T]` | **返回新 list**（P13 改原地） |
| `listRemove` | `fun listRemove[T](xs: list[T], index: int): void` | **原地删**（左移 + 缩短） |
| `dictRemove` | `fun dictRemove(d: dict, key: str): void` | 原地删，见 §3.4 |
| `dictKeys` | `fun dictKeys(d: dict): list` | 弱类型返回，见 §3.5 |
| `dictValues` | `fun dictValues(d: dict): list` | 同上 |
| `sort` | `fun sort[T](xs: list[T], reverse: bool = false): list[T]` | 插入排序，泛型单态化 |
| `join` | `fun join[T](xs: list[T], sep: str): str` | `toStr` 逐元素拼接 |
| `repeat` | `fun repeat(times: int, h: func[int][void]): void` | 回调无写回，见 §3.2 |
| `forInRange` | `fun forInRange(start: int, end: int, h: func[int][void], sep: int = 1): void` | 同上 |

### 3.2 新增（8 个，用户 2026-11-17 批准）

| API | 签名 | 说明 |
|---|---|---|
| `map` | `fun map[T](xs: list[T], h: func[T][T]): list[T]` | 逐元素变换 |
| `filter` | `fun filter[T](xs: list[T], h: func[T][bool]): list[T]` | 逐元素筛选 |
| `contains` | `fun contains[T](xs: list[T], t: T): bool` | 逐元素 `==` |
| `indexOf` | `fun indexOf[T](xs: list[T], t: T): int` | 找不到返回 -1 |
| `reverse` | `fun reverse[T](xs: list[T]): list[T]` | 逆序新 list |
| `slice` | `fun slice[T](xs: list[T], start: int, end: int): list[T]` | 左闭右开 |
| `concat` | `fun concat[T](a: list[T], b: list[T]): list[T]` | 拼接新 list |
| `sum` | `fun sum(xs: list[int]): int` | 整数求和 |

### 3.3 删除（4 个）

| API | 删除理由 |
|---|---|
| `unpack` | 依赖已删除的 `dt.setType`，`series` 类型不存在 |
| `iterContains` | 依赖 RA 迭代器 + 闭包写回；`contains` 是其纯函数替代 |
| `iterRemove` | 依赖 `dt.type` 运行时类型分派 |
| `joins(*args)` | 变长参数排到 P13（`p12_varargs_proposal.md §8.1`） |

### 3.4 `dictRemove` 的实现约束

`DictHeader { i64 len, cap, rc; ptr keys; ptr vals }`，40 字节头：

- 长度在 `+0`，`keys` 指针在 `+24`，`vals` 指针在 `+32`
- 键数组与值数组是**两个独立的堆数组**
- 删除 = 找到键下标 → 后续元素左移覆盖 → 长度减 1

限制：

- 键类型固定按 `str` 处理（当前 dict 的键在标准库层面只支持 str 比较，
  G9 的 `emitStrEq` 按内容比较）
- 不释放被删元素（`core_free` 未接通，见 §6）

### 3.5 `dictKeys` / `dictValues` 的类型问题

实测：`var xs: list = [1, "a", 2.5]` 能存，但 `xs[1]` 读出的是指针值
`140701262778720` 而非字符串 —— 运行时没有元素类型标签。

所以 `dictKeys(d): list` 返回的是**弱类型列表**：

- `dict[str, int]` 的键是 str，返回后调用方需要自己按 str 使用
- 若将来引入联合类型，这里应改为 `list[K]`

P12 阶段接受这个限制，并在文档里写明。

---

## 4. 闭包写回限制的落地方式

G6 延期意味着**回调不能修改外层变量**。实测：

```rio
var acc: int = 0
eachOf(xs, (v: int) -> { acc = acc + v })   // acc 仍是 0
```

因此 `iter` / `forInRange` / `repeat` 的正确形态是：

### 4.1 `repeat` / `forInRange` —— 纯副作用回调

```rio
fun repeat(times: int, h: func[int][void]): void {
    var i: int = 0
    while i < times {
        h(i)
        i += 1
    }
}
```

回调只能做**副作用**（打印、写文件），不能累加。需要累加时用 `sum` / `map` / `filter`。

### 4.2 旧 `iter` 的处理

旧 `ds.iter` 的契约是「回调收到 value/isFirst/isLast，可写回」。
新架构下无法实现写回，且 `iter` 的语义与 `map`/`filter`/`forInRange` 重叠。

**决定：不提供 `iter`**，由 `map` / `filter` / `forInRange` 覆盖其使用场景。
`forInRange` 保留为「按下标步进遍历」，但不带 isFirst/isLast 回调参数
（需要首尾判断时用下标自己算）。

---

## 5. 测试要求

按 `p12_plan.md §5.6` 的三层验收：

| 层 | 要求 |
|---|---|
| **compile** | `ds.rio` 独立可编译，无未定义符号 |
| **api** | 23 个 API 每个至少一个用例 |
| **behavior** | 语义正确性：边界（空列表、单元素、越界下标）、类型（int/str/float） |

测试脚本：`test_scripts/test_p12_3c_ds.ps1`

必须覆盖的边界：

- `top([])` 不崩溃
- `sort([])` / `sort([x])` 不崩溃
- `slice` 的 start/end 越界与 start > end
- `indexOf` 找不到返回 -1
- `listRemove` 删首/尾/中间/唯一元素
- `contains` 对 str 用内容比较（G9 的 `emitStrEq`）
- `join` 空列表、单元素、多元素
- `map`/`filter` 空列表

---

## 6. 内存模型阶段归属（已确认）

用户 2026-11-17 裁决：**内存模型改造归 P13，P12 只做文档修订**。

### 6.1 为什么不在 P12

实测依据：

1. **codegen 完全没有析构机制**。`emitBlock` 只有 `pushScope()` / `popScope()`
   弹符号表，不生成任何释放代码。全仓搜 `scopeExit|destructor|dtor|emitDestroy|
   dropScope|cleanupScope|leaveScope` **零命中**。
2. **`core_free` 从未被调用**，只出现在 `rcc_codegen.cpp:1344` 的白名单字符串里。
   当前容器只分配不释放。
3. 能力类型（`Ref<T>`/`Mut<T>`/`Take<T>`）、出生地（`in local`/`in result`）、
   `Pool`/`Link` 全是伪代码，需完整 Rio 语法映射。
4. P12 定位是「收尾」，会话 4–10 已排满。

### 6.2 P12 内要做的文档工作

| 项 | 内容 |
|---|---|
| 规范补写 | 把内置 `list`/`dict` 的定位补进 `memory-model-spec-full.md`：Body + 堆内存、元素不是 Body、销毁代码按元素类型生成、`xs.items` / `xs.items[*]` 路径规则、dict 键只读 |
| 架构计划修订 | `architecture_plan.md §8.1` 改指规范；`ListHeader`/`DictHeader` 的 `rc` 字段标注废弃 |

### 6.3 P13 的拆分建议

| 阶段 | 内容 | 验收边界 |
|---|---|---|
| **P13-1** | 内存模型 Rio 映射：能力类型语法、出生地语法、销毁代码生成、路径规则 | 只有内置类型；状态格只需 H/X；引用只作参数与临时子引用 |
| **P13-2** | 容器机制补全：原地增删、`clone`、`take`、扩容 + 真正释放 | `list`/`dict` 完整生命周期 |
| **P13-3** | 引用存储：引用进结构体/容器、`from` 标注、成员追踪 | 阶段 2 |
| **P13-4** | 用户对象：Pool、`take`/`fill`、Link、并发 | 阶段 3 |

### 6.4 P13 落地后 `ds` 的变化

| API | P12 | P13 |
|---|---|---|
| `listAppend` | 返回新 list | 原地追加（`void`） |
| `copy` | 浅拷贝 | 显式 `clone()` 深拷贝 |
| `listRemove` / `dictRemove` | 不释放元素 | 按元素类型生成销毁代码 |
| `dictKeys` | 弱类型 `list` | `list[K]` |

---

## 7. 与旧 `ds.rio` 的差异汇总

| 项 | 旧版 | 新版 |
|---|---|---|
| `unpack` | 有 | 删（依赖 `dt.setType`） |
| `iterContains` | 有 | 删，改用 `contains` |
| `iterRemove` | 有 | 删（依赖 `dt.type`） |
| `joins` | 有（`*args`） | 删（P13 变长参数） |
| `iter` | 有（回调带 isFirst/isLast + 写回） | 删，由 `map`/`filter`/`forInRange` 覆盖 |
| `forInRange` | 回调带 value/isFirst/isLast | 简化为 `h(i)`，无首尾参数 |
| `sort` | 快排 + 闭包比较器 | 插入排序 + 泛型单态化，`reverse` 默认参数 |
| `top` | 返回 `any` | 泛型 `[T]`，类型正确 |
| `repList` | 返回 `list` | 泛型 `[T]` |
| `listAppend` | `void` 原地 | 返回新 list（P13 改回原地） |
| `map`/`filter`/`contains`/`indexOf`/`reverse`/`slice`/`concat`/`sum` | 无 | **新增** |
