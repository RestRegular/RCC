//
// Created by RestRegular on 2025/6/29.
//

#ifndef RCC_RCC_AST_H
#define RCC_RCC_AST_H

namespace ast {
    enum class NodeType {
        AND, // 逻辑与操作节点
        ANON_FUNCTION_DEFINITION, // 匿名函数定义节点
        ASSIGNMENT, // 赋值操作节点
        ARGUMENT_ASSIGNMENT, // 属性赋值节点
        ATTRIBUTE_EXPRESSION, // 属性访问表达式节点
        BLOCK, // 代码块节点
        BOOL, // 布尔类型字面量节点
        BREAK, // break语句节点
        BRACE, // 大括号节点
        BRACKET, // 方括号节点
        BRANCH, // 分支节点
        CALL, // 函数调用节点
        CHAR, // 字符类型字面量节点
        CLASS_DECLARATION, // 类声明节点
        CLASS_DEFINITION, // 类定义节点
        COMPOUND, // 复合语句（可能用于支持多条语句的组合）
        COMPARE, // 比较运算节点
        CONDITION, // 条件语句
        CONSTRUCTOR, // 构造函数定义节点
        CONTINUE, // continue语句节点
        DECORATOR, // 装饰器节点
        DICTIONARY, // 字典结构节点
        DIVIDE, // 除法运算节点
        ENCAPSULATED, // 封装表达式节点
        EQUALITY, // 等于比较操作节点
        EXPRESSION_STATEMENT, // 表达式作为单独语句的节点
        FLOAT, // 浮点数字面量节点
        FLOOR_DIVIDE, // 向下取整除法运算节点
        FOR, // for循环控制结构节点
        FUNCTION, // 函数定义或声明节点
        FUNCTION_DECLARATION, // 函数声明节点
        FUNCTION_DEFINITION, // 函数体定义节点
        GREATER, // 大于比较操作节点
        GREATER_EQUAL, // 大于等于比较操作节点
        IDENTIFIER, // 标识符引用节点
        IMPORT, // 模块导入语句节点
        INCREMENT,  // 自增操作节点
        DECREMENT,  // 自减操作节点
        INDEX_ASSIGNMENT, // 索引赋值操作节点
        INDEX_EXPRESSION, // 索引访问表达式节点
        INFIX, // 中缀表达式通用节点
        INTEGER, // 整数字面量节点
        LABEL, // 标签节点
        LESS, // 小于比较操作节点
        LESS_EQUAL, // 小于等于比较操作节点
        LIST, // 列表结构节点
        LOGICAL, // 逻辑运算节点
        LOOP, // 循环控制结构节点
        METHOD, // 方法定义节点
        MINUS, // 减法或负号运算节点
        MODULE, // 模块顶层结构节点
        MODULO, // 取模运算节点
        MULTIPLY, // 乘法运算节点
        NOT, // 逻辑非操作节点
        NOT_EQUAL, // 不等于比较操作节点
        NULL_NODE, // 空值(null)字面量节点
        OR, // 逻辑或操作节点
        PARALLEL, // 表示用逗号分隔的并列节点
        PAIR, // 表示用冒号分隔的键值对节点
        PAREN, // 表示括号表达式节点
        PASS, // pass语句节点
        PLUS, // 加法或正号运算节点
        POSTFIX, // 后缀表达式节点
        POWER, // 幂运算节点
        PREFIX, // 前缀表达式节点
        PROGRAM, // 抽象语法树根节点，表示整个程序结构
        RANGER, // 表示范围节点
        RETURN, // return返回语句节点
        STRING, // 字符串字面量节点
        TERNARY, // 三元条件表达式节点
        UNARY, // 一元表达式节点
        UNDEFINED, // 未定义值节点
        UNKNOWN, // 未知类型节点（错误处理或预留）
        VAR, // 变量定义节点
        WHILE, // while循环控制结构节点
        TRY, // try 语句
        THROW, // throw 语句
    };

} // analyzer

#endif //RCC_RCC_AST_H
