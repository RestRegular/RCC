cmake_minimum_required(VERSION 3.20)
project(RCC CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../release")

# ==================== 编译选项 ====================
if(MSVC)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# ==================== 源文件 ====================
set(RCC_FRONTEND_SOURCES
        code/src/analyzer/lexer/rcc_lexer.cpp
        code/src/analyzer/ast/rcc_ast.cpp
        code/src/analyzer/ast/rcc_ast_components.cpp
        code/src/analyzer/ast/rcc_ast_dump.cpp
        code/src/analyzer/parser/rcc_parser.cpp
        code/src/analyzer/parser/builder/rcc_prefix_builder.cpp
        code/src/analyzer/parser/builder/rcc_infix_builder.cpp
        code/src/analyzer/parser/builder/rcc_postfix_builder.cpp
        code/src/analyzer/parser/rcc_parser_error.cpp
        code/src/analyzer/rcc_module_loader.cpp
        code/src/analyzer/rcc_symbol_export.cpp
)

set(RCC_LEGACY_SOURCES
)

set(RCC_BACKEND_SOURCES
        code/src/backend/rcc_toolchain.cpp
        code/src/backend/rcc_codegen.cpp
        code/src/backend/rcc_symbol_naming.cpp
)

set(RCC_SUPPORT_SOURCES
        code/src/rcc_core.cpp
        code/src/rcc_base.cpp
        code/src/lib/rcc_utils.cpp
        code/src/lib/newrcc.cpp
        code/src/lib/RJson/RJson.cpp
        code/src/lib/RJson/RJson_error.cpp
        code/src/lib/RLogSystem/rlog_system.cpp
)

add_executable(RCC
        code/src/rcc_main.cpp
        ${RCC_FRONTEND_SOURCES}
        ${RCC_LEGACY_SOURCES}
        ${RCC_BACKEND_SOURCES}
        ${RCC_SUPPORT_SOURCES}
)

target_include_directories(RCC PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/code/include
        ${CMAKE_CURRENT_SOURCE_DIR}/code/declarations
)

# ==================== 平台特定库 ====================
if(UNIX AND NOT APPLE)
    target_link_libraries(RCC PRIVATE dl)
endif()

# ==================== core 层目标文件 ====================
#
# lib/core 是"Rio 表达不了的东西"的唯一落脚点（系统调用、内存原语、平台接口），
# 用汇编实现，编译成一个固定的 core_win64.o，与用户程序一起链接。
#
# 这里在构建时自动汇编它，避免手工执行 as 命令。
# 产物放在 RCC.exe 同级目录下的 lib/core/，detectToolchain() 按
# getRCCDir() + "./lib/core/core_win64.o" 定位。
if(WIN32)
    set(RCC_CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/lib/core/core_win64.s")
    set(RCC_CORE_OUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/core/core_win64.o")

    if(EXISTS "${RCC_CORE_SRC}")
        # 找汇编器：优先用与编译器同目录的 as（MinGW 工具链自带）
        find_program(RCC_AS_EXECUTABLE as
                HINTS "${CMAKE_CXX_COMPILER}" ENV PATH
                DOC "GNU assembler used to build the core layer")

        if(RCC_AS_EXECUTABLE)
            add_custom_command(
                    OUTPUT "${RCC_CORE_OUT}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/core"
                    COMMAND "${RCC_AS_EXECUTABLE}" -o "${RCC_CORE_OUT}" "${RCC_CORE_SRC}"
                    DEPENDS "${RCC_CORE_SRC}"
                    COMMENT "Assembling core layer: core_win64.s -> core_win64.o"
                    VERBATIM)

            add_custom_target(rcc_core ALL DEPENDS "${RCC_CORE_OUT}")
            add_dependencies(RCC rcc_core)
        else()
            message(WARNING "GNU assembler 'as' not found: lib/core/core_win64.o will not be built")
        endif()
    else()
        message(WARNING "Core layer source not found: ${RCC_CORE_SRC}")
    endif()

    # ==================== rt 层目标文件 ====================
    #
    # lib/rt 全部用 Rio 写（这是设计要求：能从 Rio 表达的东西一律下沉到 rt）。
    # 构建时用刚编出来的 RCC 自己把它编译成 rt.o，产物同样放在
    # RCC.exe 同级的 lib/rt/ 下，由 detectToolchain() 定位。
    #
    # 这是第一次"自举前置"验证：RCC 编译 Rio 写的运行时。
    set(RCC_RT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/lib/rt/rt.rio")
    set(RCC_RT_OUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/rt/rt.o")

    if(EXISTS "${RCC_RT_SRC}")
        add_custom_command(
                OUTPUT "${RCC_RT_OUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/rt"
                COMMAND "$<TARGET_FILE:RCC>" -build -emit-lib
                        --path "${RCC_RT_SRC}" --output "${RCC_RT_OUT}"
                DEPENDS RCC "${RCC_RT_SRC}"
                COMMENT "Building rt layer: rt.rio -> rt.o (self-hosted)"
                VERBATIM)

        add_custom_target(rcc_rt ALL DEPENDS "${RCC_RT_OUT}")
        add_dependencies(rcc_rt RCC rcc_core)
    else()
        message(WARNING "Runtime layer source not found: ${RCC_RT_SRC}")
    endif()

    # ==================== std 层（stdlib 源码即发行物） ====================
    #
    # lib/std/*.rio 是**唯一真相**，release/Lib/builtin/ 只是构建产物。
    # 这样做的理由：stdlib 是纯 Rio 源码，被 import 时由模块加载器在编译期
    # 读走，没有任何"编译"步骤。旧架构把它当成随包发行的静态资源直接签进
    # 仓库，结果是"源码改了但没重新拷"这类漂移无法被构建发现。
    #
    # 现在改成构建时拷贝：仓库里只有 lib/std/ 一份，release/Lib/builtin/
    # 进 .gitignore。改了源码必然经过这次拷贝，不会再漂。
    file(GLOB RCC_STD_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lib/std/*.rio")
    set(RCC_STD_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Lib/builtin")

    if(RCC_STD_SOURCES)
        add_custom_command(
                OUTPUT "${RCC_STD_OUT_DIR}/.stamp"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${RCC_STD_OUT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RCC_STD_SOURCES} "${RCC_STD_OUT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E touch "${RCC_STD_OUT_DIR}/.stamp"
                DEPENDS ${RCC_STD_SOURCES}
                COMMENT "Copying std layer: lib/std/*.rio -> release/Lib/builtin/"
                VERBATIM)

        add_custom_target(rcc_std ALL DEPENDS "${RCC_STD_OUT_DIR}/.stamp")
        add_dependencies(RCC rcc_std)
    else()
        message(WARNING "Standard library sources not found: ${CMAKE_CURRENT_SOURCE_DIR}/lib/std/*.rio")
    endif()
endif()

# ==================== 打印配置摘要 ====================
message(STATUS "================================================")
message(STATUS "RCC Compiler Configuration Summary:")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Backend:      x86-64 GAS assembly (no LLVM)")
message(STATUS "  Output Dir:   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "================================================")
