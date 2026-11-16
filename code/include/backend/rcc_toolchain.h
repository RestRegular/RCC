//
// rcc_toolchain.h —— 驱动外部汇编器与链接器
//
// 职责：把 .s 汇编成 .o，再链接成 .exe。
// 平台差异（工具名、参数）收在这里。
//

#ifndef RCC_TOOLCHAIN_H
#define RCC_TOOLCHAIN_H

#include <string>
#include <vector>

namespace backend
{
    struct ToolchainConfig
    {
        std::string assembler;      // as.exe 路径
        std::string linker;         // g++.exe 路径（借用它的 CRT 与链接脚本）
        std::string coreObject;     // core_win64.o 路径
        std::string rtObject;       // rt.o 路径（由 RCC 编译 lib/rt/rt.rio 得到）
        std::vector<std::string> platformLibs;   // 需要显式链接的系统库
        bool keepTemps = false;     // 保留中间文件便于调试
    };

    // 自动探测 MinGW 工具链位置。找不到返回空 config。
    ToolchainConfig detectToolchain();

    class Toolchain
    {
    public:
        explicit Toolchain(ToolchainConfig config) : _config(std::move(config)) {}

        // 汇编：.s -> .o。成功返回 true。
        bool assemble(const std::string& asmPath, const std::string& objPath,
                      std::string& errorOutput) const;

        // 链接：.o + core.o -> .exe。成功返回 true。
        bool link(const std::vector<std::string>& objects, const std::string& exePath,
                  std::string& errorOutput) const;

        [[nodiscard]] const ToolchainConfig& getConfig() const { return _config; }

    private:
        ToolchainConfig _config;
    };
}

#endif //RCC_TOOLCHAIN_H
