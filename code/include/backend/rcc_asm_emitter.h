//
// rcc_asm_emitter.h —— x86-64 汇编文本发射器（GNU as / AT&T 语法）
//
// 只负责"拼文本"，不负责语义决策。这样便于：
//   * 单独对拍生成的汇编
//   * 将来插入优化 pass 时不改发射层
//

#ifndef RCC_ASM_EMITTER_H
#define RCC_ASM_EMITTER_H

#include <ios>
#include <sstream>
#include <string>
#include <vector>

namespace backend
{
    class AsmEmitter
    {
    public:
        AsmEmitter() = default;

        // ---------- 段切换 ----------
        void text() { _out << "\n    .text\n"; }
        void data() { _out << "\n    .data\n"; }
        void rodata() { _out << "\n    .section .rdata,\"dr\"\n"; }
        void bss() { _out << "\n    .bss\n"; }

        // ---------- 标签与符号 ----------
        void label(const std::string& name) { _out << name << ":\n"; }
        void global(const std::string& name) { _out << "    .globl  " << name << "\n"; }
        void align(size_t n) { _out << "    .align  " << n << "\n"; }

        // ---------- 指令（AT&T 后缀语法） ----------
        void inst(const std::string& op) { _out << "    " << op << "\n"; }

        void inst(const std::string& op, const std::string& a)
        {
            _out << "    " << op << "    " << a << "\n";
        }

        void inst(const std::string& op, const std::string& a, const std::string& b)
        {
            _out << "    " << op << "    " << a << ", " << b << "\n";
        }

        // ---------- 常用指令快捷方式 ----------
        void ret() { inst("ret"); }
        void push(const std::string& r) { inst("pushq", r); }
        void pop(const std::string& r) { inst("popq", r); }
        void mov(const std::string& src, const std::string& dst) { inst("movq", src, dst); }
        void movl(const std::string& src, const std::string& dst) { inst("movl", src, dst); }
        void lea(const std::string& src, const std::string& dst) { inst("leaq", src, dst); }
        void add(const std::string& src, const std::string& dst) { inst("addq", src, dst); }
        void sub(const std::string& src, const std::string& dst) { inst("subq", src, dst); }
        void imul(const std::string& src, const std::string& dst) { inst("imulq", src, dst); }
        void cmp(const std::string& src, const std::string& dst) { inst("cmpq", src, dst); }
        void call(const std::string& target) { inst("call", target); }
        void jmp(const std::string& target) { inst("jmp", target); }
        void je(const std::string& target) { inst("je", target); }
        void jne(const std::string& target) { inst("jne", target); }

        // 栈帧调整：sub $n, %rsp
        void subRsp(size_t n) { inst("subq", "$" + std::to_string(n), "%rsp"); }
        void addRsp(size_t n) { inst("addq", "$" + std::to_string(n), "%rsp"); }

        // 立即数
        void movImm(long long value, const std::string& dst)
        {
            inst("movq", "$" + std::to_string(value), dst);
        }

        void movImm32(long long value, const std::string& dst)
        {
            inst("movl", "$" + std::to_string(value), dst);
        }

        // ---------- 数据定义 ----------
        void quad(long long v) { _out << "    .quad   " << v << "\n"; }
        void quad(const std::string& symbol) { _out << "    .quad   " << symbol << "\n"; }

        // 字符串字面量：.ascii 不自动加结尾 0，长度由调用方另行给出
        void ascii(const std::string& escaped) { _out << "    .ascii  " << escaped << "\n"; }

        // ---------- 注释 ----------
        void comment(const std::string& text)
        {
            _out << "    # " << text << "\n";
        }

        // 文件头注释
        void header(const std::string& text)
        {
            _out << "/* " << text << " */\n";
        }

        [[nodiscard]] std::string str() const { return _out.str(); }
        void clear() { _out.str(""); _out.clear(); }

        // 新标签生成，保证唯一
        [[nodiscard]] std::string newLabel(const std::string& prefix)
        {
            return ".L" + prefix + std::to_string(_labelCounter++);
        }

        // 字符串字面量池：返回 (标签, 长度)
        std::pair<std::string, size_t> internString(const std::string& bytes)
        {
            const std::string label = ".Lstr" + std::to_string(_stringCounter++);
            _stringLiterals.emplace_back(label, bytes);
            return {label, bytes.size()};
        }

        // 带长度头的字符串字面量（**ABI v2**，P13-1）。内存布局为
        //   [0..7]   len      (8 字节小端)
        //   [8..15]  cap = 0  ← 关键：0 表示「不可释放的静态只读头」
        //   [16.. ]  UTF-8 字节
        //   [末尾]   0（C 字符串兼容，方便调试）
        // 返回 (标签, 长度)，标签指向**长度头**，即该标签就是一个 str 值。
        //
        // 为什么需要头部：str 变量在运行时必须能自描述长度，
        // 否则 rt_print_str 无法知道该打印多少字节。
        // 头部方案也让 str 只用一个指针表示（不用 ptr+len 两个寄存器），
        // 符合 docs/architecture_plan.md 里的 ABI 决定。
        //
        // 为什么 cap 必须是 0（docs/p13_plan.md D11 裁决 A）：
        // 字面量指向 .rdata 里的静态块，对它调 HeapFree 会直接崩。
        // `core_free` 因此约定「cap==0 即静默返回」。这是**值表示的约定**，
        // 不是隐式运行时检查 —— 发布构建里也不做额外判断，只读这个字段。
        std::pair<std::string, size_t> internStrValue(const std::string& bytes)
        {
            const std::string label = ".Lstrh" + std::to_string(_stringCounter++);
            _strValues.emplace_back(label, bytes);
            return {label, bytes.size()};
        }

        // 把所有字符串字面量输出到 .rdata
        void emitStringLiterals()
        {
            if (_stringLiterals.empty() && _strValues.empty()) return;
            rodata();
            for (const auto& [label, bytes] : _stringLiterals)
            {
                label_(label);
                _out << "    .ascii  \"" << escapeAsm(bytes) << "\"\n";
                _out << "    .byte   0\n";   // 方便调试查看，长度另行传递
            }
            // 带长度头的 str 值（ABI v2：{len, cap, bytes}，见 internStrValue）
            for (const auto& [label, bytes] : _strValues)
            {
                _out << "    .align  8\n";
                label_(label);
                _out << "    .quad   " << bytes.size() << "\n";
                // cap = 0：静态只读头，core_free 见 cap==0 直接返回（D11）
                _out << "    .quad   0\n";
                if (!bytes.empty())
                {
                    _out << "    .ascii  \"" << escapeAsm(bytes) << "\"\n";
                }
                _out << "    .byte   0\n";
            }
        }

        // 转义为汇编字符串字面量内容
        static std::string escapeAsm(const std::string& s)
        {
            std::string out;
            out.reserve(s.size() * 2);
            for (const unsigned char c : s)
            {
                switch (c)
                {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                case 0:    out += "\\0";  break;
                default:
                    if (c < 0x20 || c >= 0x7f)
                    {
                        // 非可打印字符用八进制转义，避免编码问题
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\%03o", c);
                        out += buf;
                    } else
                    {
                        out += static_cast<char>(c);
                    }
                }
            }
            return out;
        }

        // 回填：把某个标签占位替换成实际文本。
        // 用于「先发射指令、后确定栈帧大小」的场景。
        //
        // 注意：必须逐行精确匹配标签行（整行只有 "label:"），
        // 用 substring find 会命中其它位置的相同前缀，导致输出被截断。
        void replaceLabel(const std::string& labelName, const std::string& replacement)
        {
            const std::string needle = labelName + ":";
            std::string text = _out.str();

            size_t lineStart = 0;
            while (lineStart < text.size())
            {
                size_t lineEnd = text.find('\n', lineStart);
                if (lineEnd == std::string::npos) lineEnd = text.size();

                std::string line = text.substr(lineStart, lineEnd - lineStart);
                // 去掉行首尾空白后比较
                const auto first = line.find_first_not_of(" \t\r");
                if (first != std::string::npos)
                {
                    line = line.substr(first);
                }
                const auto last = line.find_last_not_of(" \t\r");
                if (last != std::string::npos)
                {
                    line = line.substr(0, last + 1);
                }

                if (line == needle)
                {
                    const size_t eraseLen = (lineEnd < text.size()) ? lineEnd - lineStart + 1
                                                                    : lineEnd - lineStart;
                    text.replace(lineStart, eraseLen, replacement);
                    _out.str(text);
                    // 关键：str(text) 会把写入指针复位到开头，
                    // 不手动挪到末尾的话，后续输出会从头覆盖已有内容。
                    _out.seekp(0, std::ios::end);
                    _out.clear();
                    return;
                }
                lineStart = lineEnd + 1;
            }
        }

    private:
        // 避免与 label(name) 重名
        void label_(const std::string& name) { _out << name << ":\n"; }

        std::ostringstream _out;
        size_t _labelCounter = 0;
        size_t _stringCounter = 0;
        std::vector<std::pair<std::string, std::string>> _stringLiterals;
        // 带长度头的 str 值（布局见 internStrValue）
        std::vector<std::pair<std::string, std::string>> _strValues;
    };
}

#endif //RCC_ASM_EMITTER_H
