//
// rcc_symbol_export.h —— `-s` 符号表导出（P12 重写）
//
// 设计目标（2026-11-16 用户裁决，取代旧的「与 legacy 逐字节一致」要求）：
//   1. 多种导出格式，覆盖不同消费者
//        text     终端里人眼扫（对齐表格、按种类分组）
//        json     程序消费的规范形态（带 schemaVersion，字段稳定）
//        html     单文件自包含，带搜索/筛选/分组，给开发者浏览符号表
//        ctags    Universal Ctags 格式，供编辑器做符号跳转
//        dot      Graphviz：继承图 + 调用图，供依赖分析
//        markdown 可直接嵌进 docs / README
//   2. 可扩展：加一种格式 = 写一个 emitter 函数 + 在注册表里加一行，
//      不改 CLI、不改模型、不改其它格式。
//
// 与 legacy `-s` 的关键差异（都是修掉旧实现的缺陷，不是新语言特性）：
//   * 不依赖 `ast::CompileVisitor`（P12 删除），直接走新 AST
//   * 不依赖 codegen 成功 —— 编辑器索引需要能处理「类型还没写完」的源码，
//     所以本模块**只读 AST**，类型标注按原文报出，不做解析
//   * 符号 id 不再是 legacy 里那个不可复现的全局计数器（`name_vidN`），
//     改为**由源码位置派生**的稳定 id，同样源码永远同样 id
//   * 顶层键顺序稳定（按种类 + 名字 + 位置排序），不再是 unordered_map 的
//     任意迭代序
//

#ifndef RCC_SYMBOL_EXPORT_H
#define RCC_SYMBOL_EXPORT_H

#include <memory>
#include <string>
#include <vector>

namespace ast
{
    class ProgramNode;
}

namespace analyzer
{
    // 支持的导出格式。加新格式时在这里加一项 + 在注册表里加一行。
    enum class SymbolFormat
    {
        TEXT,
        JSON,
        HTML,
        CTAGS,
        DOT,
        MARKDOWN,
    };

    // 格式名 -> 枚举。认不出来返回 false，错误信息写进 errorMsg。
    bool parseSymbolFormat(const std::string& name, SymbolFormat& out, std::string& errorMsg);

    // 所有格式名，逗号分隔（用于 --help 与报错提示）。
    std::string symbolFormatList();

    // 默认输出文件名后缀（`--output` 给的是目录时用得上）。
    const char* symbolFormatExtension(SymbolFormat format);

    // 一个形参。
    struct SymbolParam
    {
        std::string name;
        // 类型标注的原文。空串 = 没写标注。
        std::string type;
        // 变长形参（`*args`）
        bool variadic = false;
        // 关键字形参（有默认值）
        bool keyword = false;
        // 默认值原文。空串 = 没有默认值。
        std::string defaultValue;
    };

    // 符号种类。
    enum class SymbolKind
    {
        FUNCTION,
        METHOD,
        CONSTRUCTOR,
        CLASS,
        VARIABLE,
    };

    const char* symbolKindName(SymbolKind kind);

    // 一条符号记录。所有格式都从这份模型渲染，因此加格式不需要重新遍历 AST。
    struct Symbol
    {
        SymbolKind kind = SymbolKind::FUNCTION;
        // 源码里的名字（`Box`、`get`、`x`、`top`）
        std::string name;
        // 稳定 id：`<kind>:<file>:<line>:<column>:<name>` 的短哈希，可复现。
        std::string id;
        // 真实汇编符号。类/变量等没有汇编符号的为空。
        std::string asmSymbol;
        // 所属类（方法 / 构造器 / 字段）。顶层符号为空。
        std::string owner;

        // 位置
        std::string file;
        size_t line = 0;
        size_t column = 0;

        // 类型
        std::string returnType;            // 函数 / 方法的返回类型原文
        std::string type;                  // 变量 / 字段的类型原文
        std::vector<SymbolParam> params;   // 函数 / 方法 / 构造器的形参
        std::vector<std::string> genericParams;

        // 类专用
        std::string baseClass;
        std::vector<std::string> members;      // 字段名
        std::vector<std::string> methods;      // 方法名
        bool hasConstructor = false;

        // 变量的初始值原文
        std::string initializer;

        // 函数体里提到被调用的名字（依赖 / 调用图用）。
        // 不做符号决议：导出器没有类型系统，索引工具要的是「提到了哪些名字」。
        std::vector<std::string> calls;

        // 实现由运行时（rt.o）提供的 `{ encapsulated }` 函数。
        // 这类函数符号是裸名，且不参与重载计数。
        bool encapsulated = false;

        // 是否被 `export(...)` 列出
        bool exported = false;

        // 是否来自被 import 内联进来的模块文件（而非主文件）
        bool fromModule = false;
    };

    // 导出结果：符号表 + 附带信息（文件清单、模块名）。
    struct SymbolTable
    {
        std::vector<Symbol> symbols;
        std::string mainFile;
        std::vector<std::string> files;
        std::vector<std::string> moduleNames;
        // 该名字有多个定义（重载）—— 导出器据此决定是否报修饰名。
        std::vector<std::string> overloadedNames;
    };

    // 从（已完成模块内联的）ProgramNode 收集符号。
    // 只读 AST：不做类型解析、不要求 codegen 通过。
    SymbolTable collectSymbols(const std::shared_ptr<ast::ProgramNode>& program,
                               const std::string& mainFile,
                               const std::vector<std::string>& files,
                               const std::vector<std::string>& moduleNames);

    // 从**合并前**的原始 AST 里扫出 `export(...)` 列出的名字。
    //
    // 必须单独走一趟：模块内联（`ModuleLoader::expandProgram`）会把
    // `import` 和 `export` 语句一起删掉 —— 它们对 codegen 没有意义，
    // 但 `-s -exp` 恰恰需要这份信息，等合并完就再也找不到了。
    std::vector<std::string> collectExportNames(
        const std::shared_ptr<ast::ProgramNode>& program);

    // 给符号表打上 exported 标记（按 collectExportNames 的结果）。
    void markExported(SymbolTable& table, const std::vector<std::string>& names);

    // 渲染成指定格式。
    std::string renderSymbols(const SymbolTable& table, SymbolFormat format);

    // 只保留被 export(...) 列出的符号（`--export`）。
    void filterExportedOnly(SymbolTable& table);

    // 只保留指定名字的符号（`--spec-symbol`）。缺失的名字写进 missing。
    void filterByName(SymbolTable& table, const std::vector<std::string>& names,
                      std::vector<std::string>& missing);
}

#endif //RCC_SYMBOL_EXPORT_H
