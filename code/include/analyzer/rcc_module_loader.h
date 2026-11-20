//
// rcc_module_loader.h —— 编译期模块内联（P8）
//
// Rio 的 import 不是「链接一个二进制扩展」，而是把被导入文件的源码
// 在编译期读进来，与主文件合并成**一个 ProgramNode**，再统一走 codegen。
// 这样后端完全不需要知道模块的存在：内联后的模块函数就是普通全局函数。
//
// 识别写法（lexer 没有 TOKEN_IMPORT，import 不是关键字）：
//   1) `import "str"` / `import lib = "lib"`
//        -> 两条相邻语句：ExpressionStatement(IdentifierNode "import")
//                          后跟 StringLiteral 或 AssignmentNode(Identifier, StringLiteral)
//   2) `import("str", tlib="./path")`
//        -> 一条 FunctionCallNode，op 文本为 "import"，右节点是 ParenRangerNode，
//           参数是 StringLiteral 或 AssignmentNode(IdentifierNode 名, StringLiteral)
//
// 合并规则：
//   * 被导入文件的顶层语句按依赖顺序插到主文件语句之前（保证函数/类在调用前登记）
//   * import 语句本身不进入合并结果
//   * 用规范化绝对路径 + 私有符号前缀去重（同一个模块被不同别名导入会各生成副本）
//   * 递归栈做环检测，闭环直接报错
//
// 符号可见性（2026-11-20）：
//   * export(...) = 对外可见白名单；无 export 则全部顶层符号默认可见
//   * hidden 标签 = 该符号对外不可见（模块内部仍可用）；hidden ∩ export → 报错
//   * 不可见符号在内联前按 token 改名（加 __模块名_ 前缀），定义与模块内引用同步；
//     带别名的导入把**全部**符号改名，外部只能通过 alias.foo 访问（见 aliasPrefixes）
//

#ifndef RCC_MODULE_LOADER_H
#define RCC_MODULE_LOADER_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rcc_ast_components.h"

namespace analyzer
{
    struct ModuleLoadResult
    {
        // 合并后的程序（没有 import 语句，被导入文件的语句排在前面）
        std::shared_ptr<ast::ProgramNode> program;
        // 被导入的模块名（去重、按首次出现顺序）—— codegen 用它识别 `mod.fn` 限定调用
        std::vector<std::string> moduleNames;
        // `import(u="utilx")` 里声明的别名（u）。codegen 同样靠它识别
        // `u.twice(21)` 这种限定调用，但别名不是模块名，不该混进 moduleNames。
        std::vector<std::string> moduleAliases;
        // 符号可见性改造（2026-11-20）：别名 -> 该模块私有符号前缀。
        // 带别名的导入会把模块的**全部顶层符号**改名为 "<prefix>_<name>"，
        // 外部只能通过 alias.name 访问；codegen 据此把 alias.foo 解析成
        // "<prefix>_foo"。无别名导入的私有符号用 "<prefix>_<name>" 改名，
        // 导出符号保持原名（全局可见）。
        std::vector<std::pair<std::string, std::string>> aliasPrefixes;
        // 实际参与内联的文件绝对路径（第一个是主文件）
        std::vector<std::string> files;
    };

    // 把 program 里的 import 语句递归展开成一个 ProgramNode。
    // sourcePath 必须是主文件的绝对路径（相对路径的 import 相对它所在目录解析）。
    // 解析失败 / 找不到模块 / 循环导入都抛 std::runtime_error。
    ModuleLoadResult loadProgramWithModules(const std::string& sourcePath,
                                            const std::shared_ptr<ast::ProgramNode>& program);

    // 只解析单个 .rio 文件为 ProgramNode（不展开其中的 import），失败抛 std::runtime_error。
    std::shared_ptr<ast::ProgramNode> parseRioFile(const std::string& filePath);
}

#endif //RCC_MODULE_LOADER_H
