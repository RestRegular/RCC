//
// Created by RestRegular on 2025/6/29.
//

#include "../../../include/analyzer/rcc_ast.h"

namespace ast {

    std::string getNodeTypeName(NodeType nodeType) {
        switch (nodeType) {
            case NodeType::AND: return "AND";
            case NodeType::ASSIGNMENT: return "ASSIGNMENT";
            case NodeType::ATTRIBUTE_EXPRESSION: return "ATTRIBUTE_EXPRESSION";
            case NodeType::ARGUMENT_ASSIGNMENT: return "ARGUMENT_ASSIGNMENT";
            case NodeType::BLOCK: return "BLOCK";
            case NodeType::BOOL: return "BOOL";
            case NodeType::BREAK: return "BREAK";
            case NodeType::CALL: return "CALL";
            case NodeType::CHAR: return "CHAR";
            case NodeType::CLASS_DEFINITION: return "CLASS_DEFINITION";
            case NodeType::CLASS_DECLARATION: return "CLASS_DECLARATION";
            case NodeType::COMPOUND: return "COMPOUND";
            case NodeType::CONTINUE: return "CONTINUE";
            case NodeType::DECORATOR: return "DECORATOR";
            case NodeType::DICTIONARY: return "DICTIONARY";
            case NodeType::DIVIDE: return "DIVIDE";
            case NodeType::EQUALITY: return "EQUALITY";
            case NodeType::EXPRESSION_STATEMENT: return "EXPRESSION_STATEMENT";
            case NodeType::FLOAT: return "FLOAT";
            case NodeType::FLOOR_DIVIDE: return "FLOOR_DIVIDE";
            case NodeType::FOR: return "FOR";
            case NodeType::FUNCTION: return "FUNCTION";
            case NodeType::GREATER: return "GREATER";
            case NodeType::GREATER_EQUAL: return "GREATER_EQUAL";
            case NodeType::IDENTIFIER: return "IDENTIFIER";
            case NodeType::BRANCH: return "BRANCH";
            case NodeType::IMPORT: return "IMPORT";
            case NodeType::INDEX_ASSIGNMENT: return "INDEX_ASSIGNMENT";
            case NodeType::INDEX_EXPRESSION: return "INDEX_EXPRESSION";
            case NodeType::INFIX: return "INFIX";
            case NodeType::INTEGER: return "INTEGER";
            case NodeType::LESS: return "LESS";
            case NodeType::LESS_EQUAL: return "LESS_EQUAL";
            case NodeType::LIST: return "LIST";
            case NodeType::METHOD: return "METHOD";
            case NodeType::MINUS: return "MINUS";
            case NodeType::MODULE: return "MODULE";
            case NodeType::MODULO: return "MODULO";
            case NodeType::MULTIPLY: return "MULTIPLY";
            case NodeType::NOT: return "NOT";
            case NodeType::NOT_EQUAL: return "NOT_EQUAL";
            case NodeType::NULL_NODE: return "NULL_NODE";
            case NodeType::OR: return "OR";
            case NodeType::PLUS: return "PLUS";
            case NodeType::POWER: return "POWER";
            case NodeType::PROGRAM: return "PROGRAM";
            case NodeType::RETURN: return "RETURN";
            case NodeType::STRING: return "STRING";
            case NodeType::TERNARY: return "TERNARY";
            case NodeType::UNDEFINED: return "UNDEFINED";
            case NodeType::UNKNOWN: return "UNKNOWN";
            case NodeType::WHILE: return "WHILE";
            case NodeType::UNARY: return "UNARY";
            case NodeType::VAR: return "VAR";
            case NodeType::POSTFIX: return "POSTFIX";
            case NodeType::PREFIX: return "PREFIX";
            case NodeType::PARALLEL: return "PARALLEL";
            case NodeType::FUNCTION_DEFINITION: return "FUNCTION_DEFINITION";
            case NodeType::BRACE: return "BRACE";
            case NodeType::BRACKET: return "BRACKET";
            case NodeType::PAREN: return "PARENTHESIS";
            case NodeType::LABEL: return "LABEL";
            case NodeType::RANGER: return "RANGER";
            case NodeType::CONDITION: return "CONDITION";
            case NodeType::COMPARE: return "COMPARE";
            case NodeType::LOGICAL: return "LOGICAL";
            case NodeType::LOOP: return "LOOP";
            case NodeType::DECREMENT: return "DECREMENT";
            case NodeType::INCREMENT: return "INCREMENT";
            case NodeType::PASS: return "PASS";
            case NodeType::FUNCTION_DECLARATION: return "FUNCTION_DECLARATION";
            case NodeType::ENCAPSULATED: return "ENCAPSULATED";
            case NodeType::CONSTRUCTOR: return "CONSTRUCTOR";
            case NodeType::ANON_FUNCTION_DEFINITION: return "ANON_FUNCTION_DEFINITION";
            case NodeType::PAIR: return "PAIR";
            default: return "UNKNOWN_NODE_TYPE";
        }
    }

    std::string getNodeTypeFormatName(NodeType nodeType)
    {
        return "[NodeType: " + getNodeTypeName(nodeType) + "]";
    }
}
