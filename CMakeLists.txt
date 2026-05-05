cmake_minimum_required(VERSION 3.20)
project(RCC)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../release")

# ==================== 编译选项 ====================
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g -O0")

# ==================== LLVM 配置 ====================
# 设置 LLVM 安装路径（根据你的实际路径修改）
set(LLVM_DIR "D:/repositories/llvm-project/build/lib/cmake/llvm")

# 查找 LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "================================================")
message(STATUS "LLVM Configuration:")
message(STATUS "  LLVM Version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "  LLVM Include Dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "  LLVM Library Dirs: ${LLVM_LIBRARY_DIRS}")
message(STATUS "  LLVM CMake Dir: ${LLVM_DIR}")
message(STATUS "================================================")

# 包含 LLVM 头文件目录
include_directories(${LLVM_INCLUDE_DIRS})

# 添加 LLVM 的编译定义（但过滤掉 MinGW 下有问题的宏）
# 手动添加必要的宏定义
add_compile_definitions(
        __STDC_CONSTANT_MACROS
        __STDC_FORMAT_MACROS
        __STDC_LIMIT_MACROS
)

# ==================== 源文件 ====================
add_executable(RCC
        code/src/rcc_main.cpp
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
        code/include/visitors/rcc_compile_visitor.h
        code/include/visitors/rcc_collect_symbol_visitor.h
        code/include/visitors/rcc_json_visitor.h
        code/include/visitors/rcc_print_visitor.h
        code/include/visitors/rcc_visitors.h
        code/include/builtin/rcc_builtin.h
        code/src/builtin/rcc_builtin.cpp
        code/include/rcc_version.h
        code/declarations/builtin/functions/rcc_builtin_import_dec.h
        code/include/builtin/rcc_builtin_core.h
        code/src/builtin/functions/rcc_builtin_import.cpp
        code/declarations/builtin/functions/rcc_builtin_functions_dec.h
        code/declarations/builtin/functions/rcc_builtin_export_dec.h
        code/src/builtin/functions/rcc_builtin_export.cpp
        code/declarations/builtin/functions/rcc_builtin_io_functions_dec.h
        code/declarations/builtin/functions/rcc_builtin_ds_functions_dec.h
        code/src/builtin/functions/rcc_builtin_ds_functions.cpp
        code/declarations/builtin/functions/rcc_builtin_prog_functions_dec.h
        code/src/builtin/functions/rcc_builtin_prog_functions.cpp
        code/declarations/builtin/functions/rcc_builtin_datatype_functions_dec.h
        code/src/builtin/functions/rcc_builtin_datatype_functions.cpp
        code/src/builtin/functions/rcc_builtin_io_functions.cpp
        code/declarations/builtin/functions/rcc_builtin_lowlevel_functions_dec.h
        code/src/builtin/functions/rcc_builtin_lowlevel_functions.cpp
        code/src/builtin/functions/rcc_builtin_super.cpp
        code/declarations/builtin/functions/rcc_builtin_super_dec.h
        code/src/builtin/rcc_builtin_core.cpp
        code/src/visitors/tools/rcc_function_call_compiler.cpp
        code/declarations/rcc_base_dec.h
        code/declarations/rcc_core_dec.h
        code/declarations/lib/RJson/RJson_dec.h
        code/declarations/analyzer/rcc_ast_dec.h
        code/declarations/analyzer/rcc_parser_dec.h
        code/declarations/builtin/rcc_builtin_dec.h
        code/declarations/components/symbol/rcc_symbol_dec.h
        code/declarations/components/ri/rcc_ri_dec.h
        code/include/internal/rcc_compile_implementation.h
        code/include/interfaces/rcc_interface_dec.h
        code/include/interfaces/rcc_pos_interface.h
        code/include/interfaces/rcc_object_interface.h
        code/include/builtin/dll_extension_manager/rcc_dll_extension_manager.h
        code/declarations/builtin/dll_extension_manager/rcc_dll_extension_manager_dec.h
        code/src/builtin/dll_extension_manager/rcc_dll_extension_manager.cpp
        code/include/interfaces/rcc_builtin_interface.h
        code/include/visitors/rcc_llvm_visitor.h
        code/src/visitors/rcc_llvm_visitor.cpp
)

# ==================== 链接 LLVM 库 ====================
# 获取 LLVM 库列表
llvm_map_components_to_libnames(llvm_libs
        core
        support
)

# 链接 LLVM 库到 RCC
target_link_libraries(RCC PRIVATE ${llvm_libs})

# ==================== 平台特定库 ====================
if(UNIX AND NOT APPLE)
    target_link_libraries(RCC PRIVATE dl)
endif()

# ==================== LLVM 特定的编译设置 ====================
# 不启用 RTTI（LLVM 默认禁用 RTTI，但如果你的项目需要可以开启）
# 你的项目使用 RTTI（因为有 dynamic_cast），所以保持默认

# 设置 LLVM 的编译定义
#target_compile_definitions(RCC PRIVATE
#        ${LLVM_DEFINITIONS}
#)

# ==================== 打印配置摘要 ====================
message(STATUS "================================================")
message(STATUS "RCC Compiler Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  LLVM Libraries: ${llvm_libs}")
message(STATUS "  Output Dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "================================================")
