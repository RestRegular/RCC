//
// rcc_toolchain.cpp —— 工具链驱动实现
//

#include "../../include/backend/rcc_toolchain.h"
#include "../../include/rcc_base.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#define RCC_POPEN _popen
#define RCC_PCLOSE _pclose
#else
#define RCC_POPEN popen
#define RCC_PCLOSE pclose
#endif

namespace backend
{
    namespace
    {
        namespace fs = std::filesystem;

        // 执行命令并捕获 stdout+stderr
        //
        // 必须经 cmd /c 转发：_popen 在 Windows 上直接 CreateProcess，
        // 不解析引号包裹的带空格路径（例如 "D:/soft/Clion/CLion 2026.../as.exe"），
        // 会把路径在空格处截断。
        //
        // 同时把 TMP/TEMP 指向输出目录：某些受限环境下系统临时目录不可写，
        // 汇编器/链接器会因 "Cannot create temporary file" 失败。
        std::string runCommand(const std::string& command, const std::string& tmpDir, int& exitCode)
        {
            std::string full;
            if (!tmpDir.empty())
            {
                std::error_code ec;
                fs::create_directories(tmpDir, ec);
                full = "cmd /c \"set TMP=" + tmpDir + "&& set TEMP=" + tmpDir +
                       "&& " + command + " 2>&1\"";
            } else
            {
                full = "cmd /c \"" + command + " 2>&1\"";
            }

            std::string output;
            if (FILE* pipe = RCC_POPEN(full.c_str(), "r"))
            {
                std::array<char, 4096> buffer{};
                while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
                {
                    output += buffer.data();
                }
                exitCode = RCC_PCLOSE(pipe);
            } else
            {
                exitCode = -1;
                output = "Failed to launch command: " + full;
            }
            return output;
        }

        std::string quote(const std::string& path)
        {
            return "\"" + path + "\"";
        }

        bool fileExists(const std::string& path)
        {
            std::error_code ec;
            return fs::exists(path, ec) && fs::is_regular_file(path, ec);
        }
    }

    ToolchainConfig detectToolchain()
    {
        ToolchainConfig config;

        // 优先用环境变量覆盖，便于 CI 或自定义安装
        if (const char* env = std::getenv("RCC_ASSEMBLER"); env && *env)
        {
            config.assembler = env;
        }
        if (const char* env = std::getenv("RCC_LINKER"); env && *env)
        {
            config.linker = env;
        }

        // 常见 MinGW 安装位置（按优先级）
        static const std::vector<std::string> candidates = {
            "D:/soft/Clion/CLion 2026.2.3.1/bin/mingw/bin",
            "C:/msys64/mingw64/bin",
            "C:/mingw64/bin",
            "D:/msys64/mingw64/bin",
        };

        for (const auto& dir : candidates)
        {
            if (config.assembler.empty())
            {
                const auto as = dir + "/as.exe";
                if (fileExists(as)) config.assembler = as;
            }
            if (config.linker.empty())
            {
                const auto gxx = dir + "/g++.exe";
                if (fileExists(gxx)) config.linker = gxx;
            }
            if (!config.assembler.empty() && !config.linker.empty()) break;
        }

        // 退化为依赖 PATH
        if (config.assembler.empty()) config.assembler = "as";
        if (config.linker.empty()) config.linker = "g++";

        // core 目标文件：位于 RCC 可执行文件同级的 lib/core 下
        const auto coreObj = utils::getAbsolutePath("./lib/core/core_win64.o", utils::getRCCDir());
        config.coreObject = coreObj;

        // rt 目标文件：同上。缺失时不报错——用户程序可能根本不用 sout 的数字打印，
        // 真正需要时由链接器报 undefined reference，信息更准确。
        const auto rtObj = utils::getAbsolutePath("./lib/rt/rt.o", utils::getRCCDir());
        if (fileExists(rtObj)) config.rtObject = rtObj;

        return config;
    }

    bool Toolchain::assemble(const std::string& asmPath, const std::string& objPath,
                             std::string& errorOutput) const
    {
        const std::string tmpDir = fs::path(objPath).parent_path().string();
        const std::string cmd = quote(_config.assembler) + " -o " + quote(objPath) + " " + quote(asmPath);
        int exitCode = 0;
        errorOutput = runCommand(cmd, tmpDir, exitCode);
        return exitCode == 0 && fileExists(objPath);
    }

    bool Toolchain::link(const std::vector<std::string>& objects, const std::string& exePath,
                         std::string& errorOutput) const
    {
        const std::string tmpDir = fs::path(exePath).parent_path().string();
        std::ostringstream cmd;
        cmd << quote(_config.linker) << " -o " << quote(exePath);
        for (const auto& obj : objects)
        {
            cmd << " " << quote(obj);
        }
        for (const auto& lib : _config.platformLibs)
        {
            cmd << " " << lib;
        }

        int exitCode = 0;
        errorOutput = runCommand(cmd.str(), tmpDir, exitCode);
        return exitCode == 0 && fileExists(exePath);
    }
}
