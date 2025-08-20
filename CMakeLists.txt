cmake_minimum_required(VERSION 3.31)
project(RCC)

set(CMAKE_CXX_STANDARD 20)

add_executable(RCC code/src/rcc_main.cpp
        code/src/analyzer/lexer/rcc_lexer.cpp
        code/include/analyzer/rcc_lexer.h
        code/src/rcc_core.cpp
        code/include/rcc_core.h
        code/src/lib/rcc_utils.cpp
        code/include/lib/rcc_utils.h
        code/src/lib/newrcc.cpp
        code/include/lib/newrcc.h
        code/src/rcc_base.cpp
        code/include/rcc_base.h
        code/src/analyzer/ast/rcc_ast.cpp
        code/include/analyzer/rcc_ast.h
        code/src/analyzer/ast/rcc_ast_components.cpp
        code/include/analyzer/rcc_ast_components.h
        code/src/analyzer/parser/rcc_parser.cpp
        code/include/analyzer/rcc_parser.h
        code/src/analyzer/parser/builder/rcc_prefix_builder.cpp
        code/src/analyzer/parser/builder/rcc_infix_builder.cpp
        code/src/analyzer/parser/builder/rcc_postfix_builder.cpp
        code/src/analyzer/parser/rcc_parser_error.cpp
        code/include/visitors/rcc_base_visitor.h
        code/src/visitors/rcc_print_visitor.cpp
        code/src/visitors/rcc_json_visitor.cpp
        code/src/lib/RJson/RJson.cpp
        code/include/lib/RJson/RJson.h
        code/src/lib/RJson/RJson_error.cpp
        code/include/lib/RJson/RJson_error.h
        code/src/visitors/rcc_compile_visitor.cpp
        code/src/components/symbol/rcc_symbol.cpp
        code/include/components/symbol/rcc_symbol.h
        code/src/visitors/rcc_collect_symbol_visitor.cpp
        code/include/lib/RLogSystem/rlog_system.h
        code/src/lib/RLogSystem/rlog_system.cpp
        code/include/components/ri/rcc_ri.h
        code/src/components/ri/rcc_ri.cpp
        code/include/visitors/rcc_compiler_visitor.h
        code/include/visitors/rcc_collect_symbol_visitor.h
        code/include/visitors/rcc_json_visitor.h
        code/include/visitors/rcc_print_visitor.h
        code/include/visitors/rcc_visitors.h
        code/include/builtin/rcc_builtin.h
        code/src/builtin/rcc_builtin.cpp
)
