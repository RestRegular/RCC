//
// rcc_ast_dump.h —— AST 树状 dump（调试用）
//
// 不实现 Visitor 接口，直接按具体节点类型递归 —— Visitor 有几十个纯虚函数，
// 而调试时只需要「树 + 每个节点的具体类型 + 关键字段」。
//
// 用法：rcc --dump-ast --path foo.rio
//

#ifndef RCC_AST_DUMP_H
#define RCC_AST_DUMP_H

#include <memory>
#include <string>

namespace ast
{
    class ProgramNode;
    class ExpressionNode;

    // 把整棵语法树渲染成缩进文本。节点用「具体 C++ 类型名」标注，
    // 因为 NodeType 枚举与实际类型并不总是一致（容器节点就是典型例子）。
    [[nodiscard]] std::string dumpAstTree(const std::shared_ptr<ProgramNode>& program);
}

#endif //RCC_AST_DUMP_H
