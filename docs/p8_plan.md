# P8 —— 模块 / import（编译期内联）

> 目标：让 `import` 真正可用 —— 在编译期读入被导入的 `.rio` 源码，与主文件
> 合并成**一个** `ProgramNode`，再统一走一次 codegen。
>
> 验收：`test_scripts/test_p8_modules.ps1`（10 个正向 + 3 个负向，13/13 通过）；
> legacy 基线保持 15/17，P0–P7 全部仍通过。

---

## 1. 为什么是「内联」而不是「分模块编译 + 链接」

Rio 是**单文件编译**语言：没有头文件、没有前置声明，函数/类/全局的定义和使用
可以任意顺序出现。若按「每个模块编成一个 .o 再链接」来做，就必须先解决
「跨模块符号可见性 + 声明/定义分离」这一整套东西（P9/P10 的活）。

内联方案的代价小得多：把所有源码拼进一个翻译单元，符号解析交给原有的
「先收集全部定义、再发射」流程 —— codegen 里 `emitProgram` 本来就把
`FunctionDefinitionNode` / `ClassDefinitionNode` 从顶层语句里挑出来平铺发射，
所以拼平之后什么都不用改。

代价是：模块里 `var` 声明的**顶层变量**会落进 `__rio_main` 的栈帧，
没法用 `mod.VAR` 从别的模块读到（见 §5）。

---

## 2. 两种 import 写法在 AST 里的样子

`import` 不是词法关键字，parser 不认识它，所以两种写法落到 AST 上是两种完全
不同的形状。这是实现时最容易踩的坑，必须先 `-dump-ast` 看清。

### 2.1 `import ds`（裸写）

被 parser 拆成**两条相邻的顶层语句**：

```
ProgramNode
  stmt[0]: IdentifierNode("import")
  stmt[1]: IdentifierNode("ds")
```

注意不是 `ExpressionStatementNode`，是裸的 `IdentifierNode`。所以判定要直接看
语句本身是不是标识符，不能只看它里面的表达式。

### 2.2 `import(dt="datatype", ops="ops")`（带别名）

是一条 `FunctionCallNode`：

```
FunctionCallNode  mainToken.value = "import"
  left:  IdentifierNode("import")
  right: ParenRangerNode
           body: InfixExpressionNode(op=",")
                   left:  AssignmentNode(target=IdentifierNode("dt"), value=StringLiteralNode("datatype"))
                   right: AssignmentNode(target=IdentifierNode("ops"), value=StringLiteralNode("ops"))
```

`export(...)` 与它同构（`mainToken.value = "export"`），`export a, b` 则被拆成
`IdentifierNode("export")` + `InfixExpressionNode(',', a, b)` 两条语句。

---

## 3. 实现结构

### 3.1 新增文件

| 文件 | 职责 |
|---|---|
| `code/include/analyzer/rcc_module_loader.h` | 对外接口：`loadProgramWithModules` / `parseRioFile` / `ModuleLoadResult` |
| `code/src/analyzer/rcc_module_loader.cpp` | 模块解析、路径解析、递归内联、去重、环检测 |
| `test_scripts/test_p8_modules.ps1` | 13 个验收用例 |

`ModuleLoadResult` 带回三样东西：

```cpp
struct ModuleLoadResult {
    std::shared_ptr<ast::ProgramNode> program;   // 合并后的程序
    std::vector<std::string> moduleNames;        // 被内联的模块名（含别名）
    std::vector<std::string> files;              // 实际参与编译的文件
};
```

`moduleNames` 是给 codegen 用的：它要靠这份清单区分 `mod.fn(...)`（模块限定
调用，要退化成普通调用）和 `obj.method(...)`（方法调用）。

### 3.2 接入点

`code/src/rcc_main.cpp` 的 `handleBuildFlag()`，在 parse 之后、codegen 之前：

```cpp
analyzer::ModuleLoadResult merged;
try {
    merged = analyzer::loadProgramWithModules(sourcePath, program);
} catch (const std::exception& e) {
    std::cerr << "Error: Module loading failed: " << e.what() << "\n";
    return false;
}
codeGen.setModuleNames(merged.moduleNames);
codeGen.generate(merged.program, codegenError);
```

---

## 4. 模块解析顺序

`resolveModulePath(name, importerPath)` 依次尝试：

1. `RCC_BUILTIN_LIB_DIR/<name>.rio` —— 即 `release/Lib/builtin/<name>.rio`
2. `RCC_LIB_DIR/<name>/ext_info.json` 的 `main` 字段（预留的第三方库约定）
3. 导入方源码所在目录的 `<name>.rio`（相对路径）

找不到就报错，并把**三个搜索位置**都打出来 —— 模块找不到时最需要知道的就是
「我到底找过哪儿」。

---

## 5. 合并规则

对每个模块，`expandProgram` 遍历顶层语句：

- 遇到 `import` 语句 → 先把被导入模块的**全部语句**递归展开插入当前位置，
  再跳过这条 import 本身（依赖先于使用方，符号定义自然就排在前面）
- 遇到 `export` 语句 → 整条丢掉。内联之后所有顶层定义已经是同一翻译单元里的
  普通定义，`export` 没有语义；留着它会变成对未定义符号 `export` 的调用，
  链接直接失败
- 其它语句原样保留

### 5.1 去重与环检测

- 去重键：`canonicalPath`（`weakly_canonical` + 转小写，避免 Windows 上
  大小写不同被当成两个模块）
- 每个模块**只内联一次**，重复 import 返回空列表
- 环检测：`_stack` 记录当前展开链，命中链上元素就抛
  `Recursive import detected: a.rio -> b.rio -> a.rio`

**顺序很重要**：必须先查环、再查去重。主文件一开始就在 `_loaded` 里，
如果先去重，`A -> B -> A` 这个真环会被静默吞掉，看起来编译成功、实际少了一半
函数。

### 5.2 codegen 侧的两处配合

**`mod.fn(...)`**：`emitFunctionCall` 里，被调者是 `.` 中缀表达式时，先看左节点
是不是模块名。是模块名就调 `emitModuleFunctionCall`（退化成直接 `call fn`，
模块里的类则退化成构造调用），否则照旧走 `emitMethodCall`。

**`mod.fn(...)` 的类型**：`inferType` 的 `.` 分支也要认模块名，否则
`var x = mathx.mul(3, 4)` 的类型推不出来，会退回 int 并在需要 float 时出错。

---

## 6. 顺带补齐的 5 个已知缺口

P8 实施时一并处理了之前记录的 5 个缺口，它们都是模块场景下很容易撞上的：

| # | 缺口 | 修法 |
|---|---|---|
| 1 | `collectFreeVariables` 缺 `FunctionCall`/`Index`/`Dictionary`/`List` 分支，`Assignment` 只看右边 | 补上各分支；`Assignment` 左边非裸标识符时也递归一遍。`IndexExpressionNode` 必须排在 `PostfixExpressionNode` 之前判断（它是子类） |
| 2 | `emitArgsToRegisters` 把命名实参 `b=5` 当成普通表达式，报「More than 4 arguments」 | 新增 `normalizeCallArgs`：按形参名把命名实参摆回位置，再补默认值 |
| 3 | 默认形参不支持（`Undefined variable: b`） | 新增 `_functionParamNames` / `_functionDefaults`，调用点用默认值表达式补齐空位 |
| 4 | `collectParamNames` / `collectParamTypesT` 不认 `AssignmentNode` 形态的形参 | 加 `AssignmentNode` 分支，递归进 `target` |
| 5 | `parseTypeName` 只认 void/bool/char/int/float/str/ptr | 加 `list`/`series`/`array` → `ListType<int>`，`dict`/`map` → `DictType<str,int>` |

第 5 条的实际作用是：`fun f(xs: list)` 里 `xs[i]` 知道左边是容器，
不会落到「Index access on a non-container value」。

---

## 7. 已知限制

1. **模块级变量不能限定访问**。`import u` 之后写 `u.GREETING` 会得到明确诊断
   「Module member access is not supported」，而不是含糊的「推不出对象类型」。
   根因：顶层 `var` 内联后落进 `__rio_main` 的栈帧，不是全局符号。
   绕法：模块里用函数暴露状态。
2. **模块名不是命名空间**。内联之后所有符号共享一个平坦命名空间，
   两个模块定义同名函数会撞车（链接期报重复定义）。
3. **不支持选择性导入**。`import(x="m")` 只提供了别名，不提供
   「只导入 m 里的某几个符号」。
4. **`ds.rio` / `datatype.rio` 仍编不过**。它们是旧 LLVM 时代的 `encapsulated`
   桩 + `dt.setType` 调用，属于 P12 重写标准库的范围，不在 P8 内。

---

## 8. 验收

```
powershell -NoProfile -ExecutionPolicy Bypass -File test_scripts/test_p8_modules.ps1
→ 结果：通过 13，失败 0
→ P8 模块验收通过
```

正向 10 个：`single_module`、`qualified_call`、`nested_import`、
`diamond_dedupe`、`alias_import`、`module_class`、`export_ignored`、
`intra_module_calls`、`importer_before_module`、`default_and_named_args`。

负向 3 个：`missing_module`（报错含搜索路径）、`cyclic_import`
（报错含完整链）、`module_syntax_error`（报错指出出错文件）。

回归：legacy 15/17（与 P7 结束时一致），P0–P7 六个脚本全部通过。
