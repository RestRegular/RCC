cmake_minimum_required(VERSION 3.31)
project(RCC)

set(CMAKE_CXX_STANDARD 20)

add_executable(RCC code/src/rcc_main.cpp
        code/src/parser/rcc_lexer.cpp
        code/include/parser/rcc_lexer.h
        code/src/rcc_core.cpp
        code/include/rcc_core.h
        code/src/lib/rcc_utils.cpp
        code/include/lib/rcc_utils.h
        code/src/rcc_base.cpp
        code/include/rcc_base.h
        code/src/parser/rcc_ast.cpp
        code/include/parser/rcc_ast.h
        code/src/parser/rcc_ast_conponents.cpp
        code/include/parser/rcc_ast_components.h
        code/src/parser/rcc_parser.cpp
        code/include/parser/rcc_parser.h
)
