//
// rcc_abi.h —— 调用约定与平台 ABI 抽象
//
// 全部平台差异都收在这个文件里。将来加 Linux/SysV 只需新增一个
// AbiInfo 实现，backend 其余部分不动。
//
// Windows x64（MS ABI）实测确认的规则：
//   * 整型/指针参数：按参数位置依次 RCX, RDX, R8, R9
//   * 浮点参数：按参数位置依次 XMM0..XMM3（不是"第几个 float 用第几个 XMM"）
//   * 超出 4 个的参数压栈，偏移从 40(%rsp) 起（8 返回地址 + 32 shadow）
//   * 调用者必须预留 32 字节 shadow space
//   * 调用前 rsp 必须 16 字节对齐
//   * 返回值：RAX（整数/指针）、XMM0（浮点）
//   * >8 字节结构：调用者分配空间，指针作隐藏首参，后续参数右移
//   * 9..64 字节结构参数：传引用（指针）
//

#ifndef RCC_ABI_H
#define RCC_ABI_H

#include <array>
#include <string>
#include <vector>
#include "../sema/rcc_sema_types.h"

namespace backend
{
    class AbiInfo
    {
    public:
        virtual ~AbiInfo() = default;

        // 影子空间大小（字节），调用者必须预留
        [[nodiscard]] virtual size_t getShadowSpaceSize() const = 0;

        // 栈对齐要求（字节）
        [[nodiscard]] virtual size_t getStackAlignment() const = 0;

        // 可用的整型参数寄存器名（AT&T 语法，带 % 前缀）
        [[nodiscard]] virtual const std::vector<std::string>& getIntArgRegs() const = 0;

        // 可用的浮点参数寄存器名
        [[nodiscard]] virtual const std::vector<std::string>& getFloatArgRegs() const = 0;

        // 整型返回值寄存器
        [[nodiscard]] virtual std::string getIntReturnReg() const = 0;

        // 浮点返回值寄存器
        [[nodiscard]] virtual std::string getFloatReturnReg() const = 0;

        // 第 index 个栈参数的 rsp 偏移（从被调用者进入时的 rsp 算起）
        [[nodiscard]] virtual size_t getStackArgOffset(size_t index) const = 0;

        // 该类型是否按引用传递（超过 8 字节的聚合）
        [[nodiscard]] virtual bool isPassedByReference(const sema::TypePtr& type) const = 0;
    };

    // Windows x64（MS ABI）
    class Win64AbiInfo final : public AbiInfo
    {
    public:
        [[nodiscard]] size_t getShadowSpaceSize() const override { return 32; }
        [[nodiscard]] size_t getStackAlignment() const override { return 16; }

        [[nodiscard]] const std::vector<std::string>& getIntArgRegs() const override
        {
            static const std::vector<std::string> regs = {"%rcx", "%rdx", "%r8", "%r9"};
            return regs;
        }

        [[nodiscard]] const std::vector<std::string>& getFloatArgRegs() const override
        {
            static const std::vector<std::string> regs = {"%xmm0", "%xmm1", "%xmm2", "%xmm3"};
            return regs;
        }

        [[nodiscard]] std::string getIntReturnReg() const override { return "%rax"; }
        [[nodiscard]] std::string getFloatReturnReg() const override { return "%xmm0"; }

        // 8 字节返回地址 + 32 字节 shadow space = 40
        [[nodiscard]] size_t getStackArgOffset(size_t index) const override
        {
            return 40 + index * 8;
        }

        [[nodiscard]] bool isPassedByReference(const sema::TypePtr& type) const override
        {
            // 当前值表示下所有类型都 <= 8 字节（指针化），无需按引用传递。
            // 将来引入聚合类型时在这里加判断。
            (void)type;
            return false;
        }
    };
}

#endif //RCC_ABI_H
