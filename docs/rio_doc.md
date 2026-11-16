# Rio 编程语言规范文档

> 适用于 RCC v1.22.0（P12 收尾版本）。
>
> **本文档的纪律**：只写**已验证可用**的语法与 API。每一项都在
> `test_scripts/` 里有对应回归用例；没验证过的内容不写进来，避免「文档说有、
> 编译器说没有」。已知限制单独列在第八章，不含糊其辞。

---

## 一、前言

### 1.1 语言简介

Rio 是一门静态类型、编译到原生机器码的通用编程语言，由 RCC（Rio Compiler
Collection）实现。源码经词法分析、语法分析、语义检查后，直接发射 x86-64 GAS
汇编，再调用系统汇编器与链接器产出可执行文件。

设计取向：

- **静态类型 + 类型推导**：`var x = 1` 能推导出 `int`，但也允许显式标注
  `var x: int = 1`。
- **值语义为主**：变量、参数、返回值默认按值传递；`ptr` 是无类型裸地址，
  需要显式使用。
- **无运行时反射**：没有装箱、没有运行期类型标签、没有解引用运算符。
  容器是「一个头部 + 一块堆内存」，元素类型在编译期已知。
- **标准库即源码**：`lib/std/*.rio` 是标准库的**唯一真相**，没有隐藏的内置实现。
  你可以读它、改它、复制它。

### 1.2 文档目的与受众

面向想用 Rio 写程序的人，以及想读懂 RCC 标准库实现的人。

本文档**不**描述 RCC 的内部架构（见 `docs/architecture_plan.md`）、内存模型的
形式化规则（见 `docs/memory-model-spec-full.md`）、或各阶段开发计划
（见 `docs/p12_plan.md` 等）。

### 1.3 文档版本与更新记录

| 版本 | 日期 | 说明 |
|---|---|---|
| 对应 RCC v1.22.0 | 2026-11-17 | 按 P12 重写后的语言与标准库全面重写（旧版 75 行全是 `...` 占位） |

### 1.4 快速索引

| 想做什么 | 看哪里 |
|---|---|
| 装好环境跑第一个程序 | 2.1、2.2 |
| 知道有哪些类型 | 3.2 |
| 用列表 / 字典 | 3.6、3.7 |
| 写函数、泛型、重载 | 4.1、4.4、4.5 |
| 写类与继承 | 5.1、5.2 |
| 抛异常、捕获 | 6.1 |
| 用标准库 | 第七章 |
| 踩到坑了 | 第八章 |

---

## 二、快速入门

### 2.1 环境搭建

**依赖**：

| 依赖 | 用途 | 备注 |
|---|---|---|
| CMake ≥ 3.20 | 构建 | |
| MinGW-w64（`g++` / `as` / `ld`） | 汇编与链接 | 必须在 `PATH` 里 |
| `mingw32-make` | 构建 | 随 MinGW 提供 |

**构建**：

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -C build -j 10
```

产物：`release\RCC.exe`。标准库 `lib/std/*.rio` 会被自动拷贝到
`release\Lib\builtin\`（构建产物，不签进仓库）。

**编译并运行一个程序**：

```bat
release\RCC.exe -build --path hello.rio --output hello.exe
hello.exe
```

或用便捷脚本：

```bat
scripts\run.bat -i tests\test_1_builtins
```

**跑全部测试**：

```bat
scripts\test.bat
```

### 2.2 第一个 Rio 程序

```rio
import(io="io")

sout("Hello, Rio!")
```

输出：

```
Hello, Rio!
```

几点说明：

- **顶层语句会按顺序执行**，不需要包在 `main` 里。写了 `fun main()` 也不会被
  自动调用 —— 想执行就显式写 `main()`。
- `sout` 是编译器特化的输出原语（不是 `io` 模块的函数），可接受多个参数，
  自动转成字符串并拼接。
- `import(io="io")` 把 `io` 模块内联进来。模块是**编译期内联**的，不是运行期加载。

---

## 三、Rio 语言基础

### 3.1 标识符与关键字

标识符由字母、数字、下划线组成，不能以数字开头。区分大小写。

**关键字**：

| 类别 | 关键字 |
|---|---|
| 声明 | `var` `const` `fun` `class` `ctor` |
| 类型 | `int` `float` `bool` `str` `char` `void` `list` `dict` `ptr` `funi` `any` |
| 控制流 | `if` `else` `while` `for` `break` `continue` `ret` |
| 异常 | `try` `catch` `finally` `throw` |
| 类 | `this` `super` `static` `override` |
| 模块 | `import` `encapsulated` |

**注释**：`//` 单行，`/* ... */` 块注释，`/** ... */` 文档注释。

### 3.2 数据类型

| 类型 | 说明 | 字面量示例 |
|---|---|---|
| `int` | 64 位有符号整数 | `42`、`-7`、`0` |
| `float` | 64 位双精度浮点 | `3.14`、`-0.5`、`1.0` |
| `bool` | 布尔 | `true`、`false` |
| `str` | 字符串（UTF-8 字节序列） | `"hello"`、`""` |
| `char` | 单字符（本质是小整数） | `'A'` |
| `void` | 无返回值 | — |
| `list[T]` | 元素类型为 `T` 的列表 | `[1, 2, 3]`、`[]` |
| `dict` | 键为 `str`、值任意的字典 | `{"k": 1}`、`{}` |
| `ptr` | 无类型裸地址 | 由 `core_alloc` 等获得 |
| `funi` | 函数值 | `(x: int) -> { ret x }` |
| `any` | 指针族值（`str`/`list`/`dict`/`ptr`），见 8.7 | — |

**零值**：声明但不初始化时，得到该类型的零值。

| 类型 | 零值 |
|---|---|
| `int` | `0` |
| `float` | `0.0` |
| `bool` | `false` |
| `str` | `null`（长度为 0，打印为空） |
| `list[T]` | `null`（长度为 0） |
| `dict` | `null`（长度为 0） |

> `str` 的零值是 `null` 而不是 `""`。两者在**长度和打印**上表现一致
> （`core_str_len(null)` 返回 0，`sout(null)` 输出空），但 `null` 参与字符串
> 运算时依赖编译器的判空处理 —— 这在 v1.22.0 已修好（见 8.3）。

**类型推导**：`var x = 1` 推导为 `int`，`var s = "a"` 推导为 `str`，
`var l = [1, 2]` 推导为 `list[int]`。需要精确类型时显式标注。

### 3.3 变量与常量

```rio
var a = 1                 // 推导为 int
var b: float = 1.5        // 显式标注
var c: int                // 零值 0
const PI: float = 3.141593 // 常量，不可重新赋值
```

**顶层 `var` 是全局变量**，在所有函数之外声明，有独立的全局槽：

```rio
var counter: int = 0

fun bump(): void {
    counter += 1
}
```

**赋值与复合赋值**：

```rio
var x = 10
x = 20      // 普通赋值
x += 5      // 等价于 x = x + 5
x -= 3
x *= 2
x /= 4
x %= 3
```

`+=` 在 `str` 与 `list` 上有特殊语义（拼接）：

```rio
var s = "a"
s += "b"            // s == "ab"

var l = [1, 2]
l += [3]            // l == [1, 2, 3]
```

### 3.4 运算符

| 类别 | 运算符 | 说明 |
|---|---|---|
| 算术 | `+` `-` `*` `/` `%` | 整数除法截断；`%` 是取余 |
| 比较 | `==` `!=` `<` `<=` `>` `>=` | `str` 按字典序比较（逐字节） |
| 逻辑 | `&&` `\|\|` `!` | 短路求值 |
| 位运算 | `&` `\|` `^` `<<` `>>` `~` | 仅 `int` |
| 拼接 | `+` | `str + str`、`list + list` |

**`str + str` 是真拼接**（不是整数加法）。`"a" + "b"` 得到 `"ab"`。

**`list + list` 返回新列表**，不修改原列表：

```rio
var a = [1, 2]
var b = a + [3]     // b == [1, 2, 3]，a 仍是 [1, 2]
```

### 3.5 控制流

**`if` / `else`**（条件不需要括号）：

```rio
if x > 0 {
    sout("positive")
} else if x < 0 {
    sout("negative")
} else {
    sout("zero")
}
```

**`while`**：

```rio
var i = 0
while i < 5 {
    sout(i)
    i += 1
}
```

**`for`**（C 风格三段式，三段都用花括号包起来）：

```rio
for ({var i = 0; i < 5; i += 1}) {
    sout(i)
}
```

> **注意**：`for` 的三段必须写在 `({ ... })` 里。`for x in xs` 这种
> **for-in 语法不存在**，用 `while` + 下标代替。

**`break` / `continue`**：跳出 / 继续当前循环。

**`ret`**：返回值。`ret` 不带表达式表示返回 `void`。

```rio
fun abs(x: int): int {
    if x < 0 {
        ret -x
    }
    ret x
}
```

### 3.6 列表

```rio
var xs: list[int] = [1, 2, 3]
var empty: list[int] = []
var names = ["a", "b"]        // 推导为 list[str]
```

**下标读取与写入**：

```rio
sout(xs[0])       // 1
xs[0] = 99        // 写入
xs[1] += 10       // 复合赋值
sout(xs)          // [99, 12, 3]
```

越界读写会被**边界检查拦截**（静默跳过写入，读取返回零值），不会破坏内存。

**长度**用 `str.size`（见 7.2）：

```rio
import(sm="str")
sout(size(xs))    // 3
```

**列表操作**用 `ds` 模块（见 7.1）：

```rio
import(ds="ds")
var doubled = map(xs, (v: int) -> { ret v * 2 })
var evens = filter(xs, (v: int) -> { ret v % 2 == 0 })
var sorted = sort(xs)
var total = sum(xs)
```

### 3.7 字典

键必须是 `str`，值可以是任意类型：

```rio
var d: dict = {"name": "Rio", "age": 18}
sout(d["name"])     // Rio
d["age"] = 19       // 写入
d["new"] = 1        // 新增键
```

**空字典**：`var d: dict = {}`。

遍历字典用 `ds.dictKeys` / `ds.dictValues` 取出键值列表，再用 `while` 循环：

```rio
import(ds="ds", sm="str")
var d: dict = {"a": 1, "b": 2}
var ks: list[str] = dictKeys(d)
var i = 0
while i < size(ks) {
    sout(ks[i])
    i += 1
}
```

> 字典内部是**线性数组**，遍历顺序 = 插入顺序。键只读，不能通过返回的列表改回字典。

### 3.8 字符串

```rio
var s = "hello"
sout(s[0])          // 104（char 的整数值，不是 "h"）
sout(size(s))       // 5
```

**`s[i]` 读取支持，`s[i] = c` 写入不支持**（编译期报错）。原仓库的 `str` 写入
是静默失效的 bug，本项目不复制该行为。

字符串操作见 7.2。

---

## 四、函数

### 4.1 基本定义

```rio
fun add(a: int, b: int): int {
    ret a + b
}

fun greet(name: str): void {
    sout("hello, ", name)
}
```

**参数不写类型标注时默认为 `int`**：

```rio
fun double(x) { ret x * 2 }   // x 是 int
```

### 4.2 默认参数

```rio
fun greet(name: str, greeting: str = "hello"): void {
    sout(greeting, ", ", name)
}

greet("Rio")             // hello, Rio
greet("Rio", "hi")       // hi, Rio
```

默认参数必须在参数列表**末尾**。

### 4.3 变长参数

```rio
fun sumAll(*args: int): int {
    ...
}
```

> **限制**：变长参数目前只能**声明**，`*args` 在函数体内取不到实参集合
> （G1b 延期）。标准库里 `prog.entry(arg: any, *args: any)` 就是这种情况 ——
> 它存在但函数体为空。**不要依赖 `*args` 做实际工作**。

### 4.4 泛型

用 `[T]` 声明类型参数，在**调用点单态化**（为每组实参类型生成一份特化代码）：

```rio
fun identity[T](x: T): T {
    ret x
}

sout(identity(42))        // int 版本
sout(identity("hi"))      // str 版本
```

**多个类型参数**：`fun pair[T, K](a: T, b: K): ...`。

**泛型约束不了「按类型分派」**：`T` 在函数体内是**已确定的具体类型**，
所以不能写「如果 T 是 int 就……」。需要按类型分派时用**重载**（4.5）。

> 这也是 `str.toStr` 为什么用重载而不是泛型：泛型版会把 `2.5` 截断成 `2`。

### 4.5 重载

同名函数按**参数类型**分派，在调用点静态决议：

```rio
fun toStr(arg: int): str { ... }
fun toStr(arg: float): str { ... }
fun toStr(arg: bool): str { ... }
```

**决议优先级**：精确匹配 > 同族匹配 > 拓宽匹配。

| 层级 | 说明 |
|---|---|
| 精确匹配 | 实参类型与形参类型完全一致 |
| 同族匹配 | 数值族内部（`int`/`float`/`bool`/`char`）的亲和度梯度 |
| 拓宽匹配 | `bool`/`char` → `int`（无损拓宽） |

**方法也能重载**（同一类里同名不同参数）。

> 参数个数不同的重载是支持的；**返回值类型不能用于区分重载**。

### 4.6 匿名函数与函数值

```rio
var f = (x: int) -> { ret x * 2 }
sout(f(5))          // 10
```

作为参数传递：

```rio
import(ds="ds")
var doubled = map([1, 2, 3], (v: int) -> { ret v * 2 })
```

类型标注是 `funi`，可带签名：`funi[int][int]` 表示「收 int 返回 int」。

> **限制**：闭包是**按值捕获**，回调里**不能写回外层变量**（G6 延期）。
> 所以 `ds.iter` 这类「靠回调累加」的设计被去掉了，改用 `map`/`filter`
> 返回新值，或 `forInRange` 只做副作用。

### 4.7 递归

```rio
fun fib(n: int): int {
    if n <= 1 {
        ret n
    }
    ret fib(n - 1) + fib(n - 2)
}
```

---

## 五、类

### 5.1 基本定义

```rio
class Point {
    var x: int, y: int        // 成员用逗号分隔

    ctor(a: int, b: int) {    // 构造函数名固定为 ctor
        this.x = a
        this.y = b
    }

    fun len2(): int {         // 方法
        ret this.x * this.x + this.y * this.y
    }
}

var p = Point(3, 4)
sout(p.x)          // 3
sout(p.len2())     // 25
```

**静态成员**：`var count: int static = 0`，通过 `类名.成员` 访问。

### 5.2 继承

```rio
class Base {
    var val: int

    ctor(v: int) {
        this.val = v
    }
}

class Derived: Base {
    var extra: int

    ctor(v: int, e: int) {
        super(v)              // 调用父类构造
        this.extra = e
    }
}

var d = Derived(42, 99)
sout(d.val)       // 42
sout(d.extra)     // 99
```

**方法重写**用 `override`：

```rio
class Derived: Base {
    override fun show(): void { ... }
}
```

### 5.3 对象布局

类实例在栈上（局部变量）或堆上（`core_alloc`）按**字段顺序**紧凑布局，
没有虚表、没有运行期类型标签。继承时父类字段在前。

---

## 六、异常

### 6.1 抛出与捕获

```rio
fun check(x: int): void {
    if x < 0 {
        throw "negative not allowed"
    }
}

try {
    check(-1)
} catch(e) {
    sout("caught: ", e)
} finally {
    sout("cleanup")
}
```

`throw` 接受一个 `str` 作为异常信息。`catch(e)` 中的 `e` 是异常信息字符串。

**`finally` 总会执行**，无论是否抛出、是否被捕获。

### 6.2 内置检查

标准库里有几处会主动 `throw`：

| 位置 | 条件 |
|---|---|
| `str.charToInt(s: str)` | 字符串长度不等于 1 |

---

## 七、标准库

标准库是 `lib/std/*.rio` 六个模块，**全部是 Rio 源码**，没有隐藏的内置实现。

**导入方式**：

```rio
import(ds="ds")                              // 单个
import(ds="ds", sm="str", m="math")          // 多个
```

`import` 把模块**内联**进当前编译单元，模块内的函数进入同一命名空间，
所以可以直接用裸名调用（`sort(xs)` 而不是 `ds.sort(xs)`）。

**约定**：模块私有函数带前缀（`_ds`、`_str`），不会与其它模块撞名。

> **嵌套 import 会平铺符号**：如果 `ds` 内部 import 了 `str`，那么
> `import ds` 之后你也能直接调 `str` 的函数。这是现有语义，不是 bug，
> 但**不要依赖它** —— 明确写出你要用的模块。

### 7.1 `ds` —— 容器操作（22 个）

```rio
import(ds="ds")
```

| 函数 | 签名 | 说明 |
|---|---|---|
| `isEmpty` | `(arg: any): bool` | 空判断，对 `str`/`list`/`dict` 通用 |
| `top` | `[T](xs: list[T]): T` | 末元素；空列表返回类型零值 |
| `copy` | `[T](xs: list[T]): list[T]` | 浅拷贝 |
| `repStr` | `(s: str, times: int): str` | 重复字符串 |
| `repList` | `[T](x: T, times: int): list[T]` | 重复元素成列表 |
| `listAppend` | `[T](xs: list[T], item: T): list[T]` | **返回新列表**，不改原列表 |
| `listRemove` | `[T](xs: list[T], index: int): void` | **原地**删除，越界忽略 |
| `dictRemove` | `(d: dict, key: str): void` | **原地**删除 |
| `dictKeys` | `(d: dict): list[str]` | 键列表，插入顺序 |
| `dictValues` | `(d: dict): list` | 值列表，插入顺序 |
| `contains` | `[T](xs: list[T], t: T): bool` | 是否包含 |
| `indexOf` | `[T](xs: list[T], t: T): int` | 首次出现的下标，没有返回 `-1` |
| `map` | `[T](xs: list[T], h: funi): list[T]` | 映射 |
| `filter` | `[T](xs: list[T], h: funi): list[T]` | 过滤 |
| `reverse` | `[T](xs: list[T]): list[T]` | 反转，返回新列表 |
| `slice` | `[T](xs: list[T], start: int, end: int): list[T]` | 切片，下标自动钳制 |
| `concat` | `[T](a: list[T], b: list[T]): list[T]` | 拼接 |
| `sum` | `(xs: list[int]): int` | 求和 |
| `sort` | `[T](xs: list[T], reverse: bool = false): list[T]` | 排序，返回新列表 |
| `join` | `[T](xs: list[T], sep: str = ""): str` | 用分隔符拼成字符串 |
| `repeat` | `(times: int, h: funi): void` | 调用回调 `times` 次，回调收到下标 |
| `forInRange` | `(start: int, end: int, h: funi, sep: int = 1): void` | 按步长遍历 |

**注意 `ds` 不导出 `size`** —— 用 `str.size`（7.2）。两个模块都导出 `size`
会造成导入冲突。

**`listAppend` 为什么返回新列表**：索引赋值有边界检查，无法原地追加。
返回新列表是当前架构下的正确做法，等 P13 容器机制补全后可能改回原地追加。

### 7.2 `str` —— 字符串（15 个函数 / 22 个重载入口）

```rio
import(sm="str")
```

| 函数 | 签名 | 说明 |
|---|---|---|
| `size` | `(arg: any): int` | 长度，对 `str`/`list`/`dict` 通用 |
| `toStr` | `(arg: int\|float\|bool\|str\|char): str` | 转字符串（5 个重载） |
| `intToStr` | `(v: int): str` | 整数转字符串 |
| `floatToStr` | `(v: float): str` | 浮点转字符串 |
| `toChar` | `(arg: str): int` / `(arg: int): int` | 取字符的整数值 |
| `charToInt` | `(c: int\|char\|str): int` | 字符转整数（3 个重载） |
| `intToChar` | `(i: int): char` | 整数转字符 |
| `subStr` | `(content: str, startIndex: int = 0, length: int = -1): str` | 子串；`length=-1` 到末尾 |
| `find` | `(content: str, part: str, startIndex: int = 0): int` | 查找，没有返回 `-1` |
| `toLowerCase` | `(data: str): str` | 转小写（**只接受 `str`**） |
| `toUpperCase` | `(data: str): str` | 转大写（**只接受 `str`**） |
| `replace` | `(content: str, old: str, new: str, once: bool = false): str` | 替换 |
| `split` | `(content: str, del: str, keepDel: bool = false, keepEmpty: bool = false): list[str]` | 分割 |
| `chunk` | `(content: str, num: int): list[str]` | 按长度切块 |
| `trim` | `(content: str, extraChars: str = " ", onLeft: bool = true, onRight: bool = true): str` | 去两端字符 |

**边界行为**（都有回归用例）：

| 调用 | 结果 |
|---|---|
| `subStr("hello", 99, 3)` | `""`（起点越界） |
| `subStr("hello", -1, 2)` | `"he"`（负起点钳到 0） |
| `subStr("hello", 0, 0)` | `""` |
| `find("hello", "xyz")` | `-1` |
| `find("abc", "")` | `0`（空模式串匹配在起点） |
| `trim("xxhixx", "")` | `"xxhixx"`（空字符集 = 什么都不去） |
| `charToInt("true")` | 抛异常（长度 ≠ 1） |

**UTF-8**：`str` 是字节序列，`size` 返回**字节数**。`subStr` 按字节切，
切在多字节字符中间会得到无效 UTF-8 —— 切中文时长度要用 3 的倍数。

### 7.3 `math` —— 数学（7 个）

```rio
import(m="math")
```

| 名称 | 签名 | 说明 |
|---|---|---|
| `abs` | `[T](arg: T): T` | 绝对值 |
| `max` | `[T](a: T, b: T): T` | 较大值 |
| `min` | `[T](a: T, b: T): T` | 较小值 |
| `opp` | `[T](arg: T): T` | 相反数 |
| `pow` | `[T](a: T, times: int): T` | 幂（`times` 次方） |
| `pi` | `float` 常量 | 3.141593 |
| `e` | `float` 常量 | 2.718282 |

`pow` 的第二个参数是**整数次数**，不是浮点指数。

### 7.4 `ops` —— 惰性与三元（4 个）

```rio
import(o="ops")
```

| 函数 | 签名 | 说明 |
|---|---|---|
| `ternary` | `[T](condition: bool, trueValue: T, falseValue: T): T` | 三元选择（两个值都求值） |
| `ternaryLazy` | `[T](condition: bool, trueFuni: funi, falseFuni: funi): T` | 三元选择（只求值选中的分支） |
| `orVal` | `[T](fallback: T, spare: T): T` | `fallback` 为 `null` 时取 `spare` |
| `orLazy` | `[T](front: T, back: funi): T` | `front` 为 `null` 时调 `back` 求值 |

```rio
import(o="ops")
sout(ternary(true, "a", "b"))       // a
sout(orVal(null, 10))               // 10
sout(orVal(5, 10))                  // 5
sout(orLazy(null, () -> { ret 7 })) // 7
```

### 7.5 `io` —— 输入（2 个）

```rio
import(io="io")
```

| 函数 | 签名 | 说明 |
|---|---|---|
| `input` | `(prompt: str = ""): str` | 读一行输入 |
| `sin` | `(): str` | 读一行输入（无提示） |

> `sout` **不属于** `io` 模块 —— 它是编译器特化的输出原语，无需 import 即可用。

### 7.6 `prog` —— 程序控制（2 个）

```rio
import(prog="prog")
```

| 函数 | 签名 | 说明 |
|---|---|---|
| `entry` | `(arg: any, *args: any): void` | 当前**空实现**，需 ≥1 个实参 |
| `breakpoint` | `(): void` | 断点标记，无副作用 |

> `entry` 的设计有四个结构性缺陷（argv 无来源、`*args` 取不到实参等），
> P12 不动它，登记待重新设计。**不要用它接收命令行参数**。

### 7.7 底层原语

以下函数由 `lib/core/core_win64.s` 提供，无需 import：

| 函数 | 签名 | 说明 |
|---|---|---|
| `core_str_len` | `(s: str): int` | 长度，`null` 返回 0 |
| `core_mem_copy` | `(dst: ptr, src: ptr, n: int): void` | 内存拷贝 |
| `core_mem_set` | `(p: ptr, v: int, n: int): void` | 内存填充（只写 `v` 的低 8 位） |
| `core_mem_eq` | `(a: ptr, b: ptr, n: int): bool` | 内存比较 |
| `core_alloc` | `(n: int): ptr` | 分配 |
| `core_free` | `(p: ptr): void` | 释放 |

> 这些是**实现标准库用的**，用户代码一般不该直接调用。
> `core_mem_set` 每次只写 1 字节（`n=1`），写多字节要循环 —— 标准库的
> `_dsSetLen` / `_strSetLen` 就是干这个的。

---

## 八、已知限制

本章列出**当前版本确实做不到**的事。不是「不推荐」，是「做不到」。

### 8.1 无 for-in 语法

`for x in xs` / `for k in d` 都是语法错误。用 `while` + 下标。

### 8.2 闭包不能写回外层变量

```rio
var total = 0
repeat(3, (i: int) -> {
    total += i        // 无效：改的是闭包自己的副本
})
// total 仍是 0
```

G6 延期到后续版本。需要累加时用 `map`/`filter`/`sum` 返回新值。

### 8.3 `null` 字符串的运算

`str` 的零值是 `null`。`"" + null`、`null == ""`、`null < "a"` 在 v1.22.0
已正确处理（按空串），但这依赖编译器的判空分支 —— 如果你自己写
`core_str_len` 之外的原语直接读长度头，仍可能崩。

### 8.4 变长参数取不到值

`*args` 能声明、能传，但**函数体内取不到实参集合**（G1b 延期）。

### 8.5 无析构、无自动释放

`core_free` 从不会被自动调用。`list`/`dict` 一旦分配就不会释放。
长驻进程反复分配容器会持续增长。这是 P13 内存模型改造的范围。

### 8.6 浮点打印精度

`sout(float)` 走 `rt_print_float`，最多打印 **6 位小数**并去掉尾随零：

```rio
sout(0.1 + 0.2)    // 0.3
sout(1.000001)     // 1.0
```

浮点**字面量**本身是精确的（v1.22.0 修好了 6 位有效数字截断），
但**打印**受这个限制。

### 8.7 `any` 是「指针族」，不是「任意类型」

`any` **可以**用于形参和返回值标注（v1.22.0 起支持）：

```rio
fun len(arg: any): int {
    ret core_str_len(arg)      // 对 str / list / dict 都成立
}

fun pass(x: any): any {
    ret x
}
```

但它的真实含义是**「指针族值」**（`str` / `list` / `dict` / `ptr`），
**不是**「任何值」。编译器不会替你区分类型，也不会装箱：

| 用法 | 结果 |
|---|---|
| `len("abc")` → 3 | 正常 |
| `len([1,2])` → 2 | 正常 |
| `len({"k":1})` → 1 | 正常 |
| `len(5)` | **运行时崩溃**（把 5 当地址读） |

`pass(x: any): any` 同样不做类型转换：`var s: str = pass("hi")` 可以，
但 `var s: str = pass(7)` 是编译期类型错误，`pass([1,2])[0]` 也是编译期错误
（返回值是 `any`，不是容器，不能下标）。

**实践建议**：只在「长度、判空、原样透传」这类**不需要知道具体类型**的地方用
`any`。需要按类型做不同的事，用**重载**（4.5）或泛型（4.4）。

### 8.8 `str` 写入不支持

`s[i] = c` 编译期报错。需要改字符串时用 `subStr` + 拼接重建。

### 8.9 参数个数上限

后端 `emitArgsToRegisters` 对整数寄存器传参有上限。超过时会在编译期失败，
不会静默截断。

### 8.10 `ds.join` 的元素类型

`join` 内部用 `toStr` 转换元素，支持 `int`/`float`/`bool`/`str`/`char`。
元素是 `list`/`dict`/`ptr` 时行为未定义（`toStr` 没有对应重载）。

---

## 九、附录

### 9.1 完整示例

```rio
import(ds="ds", sm="str", m="math")

fun main(): void {
    var xs: list[int] = [3, 1, 4, 1, 5]

    var sorted: list[int] = sort(xs)
    sout("sorted: ", join(sorted, ", "))

    var evens: list[int] = filter(sorted, (v: int) -> { ret v % 2 == 0 })
    sout("evens: ", join(evens, ", "))

    sout("sum: ", sum(sorted))
    sout("max: ", max(4, 9))
    sout("upper: ", toUpperCase("done"))
}

main()
```

输出：

```
sorted: 1, 1, 3, 4, 5
evens: 4
sum: 14
max: 9
upper: DONE
```

### 9.2 命令行参考

| 选项 | 说明 |
|---|---|
| `-build` | 编译到可执行文件 |
| `--path <file>` | 输入 `.rio` 文件 |
| `--output <file>` | 输出路径 |
| `-emit-lib` | 库模式（不发射 `main`） |
| `-s` | 导出符号表 |
| `--p <file>` | `-s` 的输入 |
| `--fmt <text\|json\|html\|ctags\|dot\|markdown>` | `-s` 的输出格式 |
| `-exp` | 只列导出符号 |
| `-v` / `--version` | 版本信息 |
| `-h` / `--help` | 帮助 |

### 9.3 与旧版本的差异

从 v1.21.x 迁移到 v1.22.0 需要改源码的地方，见
`docs/p12_plan.md` §7.1.5 的迁移对照表。主要变化：

| 旧写法 | 新写法 |
|---|---|
| `import stdio` | `import(io="io")` |
| `ds.size(x)` | `str.size(x)` |
| `ds.unpack(x)` | 直接用 `x` |
| `ds.iter(xs, cb)` | `ds.map` / `ds.filter` / `ds.forInRange` |
| `ds.joins(a, b)` | `ds.join([a, b], sep)` |
| `lowlevel.*` | 不提供 |
| `datatype` 模块 | 不提供 |
| `RCC -ir --ir-o` | `RCC -build --output` |
