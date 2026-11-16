//
// rcc_symbol_naming.h —— 汇编符号命名的**唯一**实现
//
// 为什么要有这个文件：
//   P10/P11 冻结了两条符号规则（类型实参修饰、形参类型修饰），原本以
//   CodeGen 的 private static 形式存在。`-s` 的符号表导出需要报出与
//   真实目标文件**逐字一致**的汇编符号（否则 objdump / perf 的输出对不上
//   符号表，profiling 用途直接失效），因此这套规则必须是公共工具，而不是
//   在导出器里再抄一份 —— 抄一份就一定会漂移。
//
// 规则（docs/architecture_plan.md §4.3 + docs/p11_plan.md §2）：
//   * 名字只有一个定义  -> 符号就是裸名
//   * 名字有多个定义    -> 全部用 `_R<len><name><编码>`，编码的是**形参类型**
//   * 泛型函数实例      -> `_G<len><name><编码>`，编码的是**类型实参**
//   * 泛型类实例        -> `_N<len><name><编码>`，编码的是**类型实参**
//   * 方法 / 构造器      -> `<Class>.<name>` / `<Class>.__init__`
//   * 用户自己写的 main  -> `__rio_user_main`（避开 C 入口 main）
//
// **2026-11-17（用户批准的 ABI 变更）**：泛型实例与重载原本共用 `_R` 前缀，
// 只是「编码的东西」不同（类型实参 vs 形参类型），于是会撞车。实测例子：
//     fun toStr[T](arg: list[T])   T=list[int]  ->  _R5toStr + "Li"
//     fun toStr(arg: int)                       ->  _R5toStr + "i"
// 两个都拼成 `_R5toStri`，汇编器报 symbol already defined。
// 现在泛型用 `_G`、泛型类用 `_N`、重载仍用 `_R`，三者分属各自的命名空间。
// 类型编码表本身一个字都没改。
//
// 类型编码表（冻结）：
//   v=void b=bool c=char i=int f=float K=str P=ptr
//   L<elem>=list  D<key><val>=dict  F<params>_<ret>=func  N<len><name>=class
//   A=显式 any（与「未标注/未知」的 v 区分）   T0=泛型形参占位
//

#ifndef RCC_SYMBOL_NAMING_H
#define RCC_SYMBOL_NAMING_H

#include <string>
#include <vector>

#include "../sema/rcc_sema_types.h"

namespace backend
{
    // 用户自己写的 `fun main()` 的汇编符号。
    // 必须与 C 入口 `main` 区分开，否则汇编器报 symbol already defined。
    inline constexpr const char* USER_MAIN_SYMBOL = "__rio_user_main";

    // 构造器的符号后缀：`<Class>.__init__`
    inline constexpr const char* CTOR_SYMBOL_SUFFIX = ".__init__";

    namespace naming
    {
        // 单个类型的编码。nullptr / 未知 -> "v"。
        [[nodiscard]] std::string mangleType(const sema::TypePtr& type);

        // `_R<len><base><每个形参类型的编码>`。**只用于重载**。
        // 泛型实例请用 mangleGenericInstance（`_G`，编码类型实参），
        // 两者不能再共用前缀 —— 见文件头的 2026-11-17 ABI 变更说明。
        [[nodiscard]] std::string mangleSignature(const std::string& base,
                                                  const std::vector<sema::TypePtr>& args);

        // `_G<len><base><每个类型实参的编码>`。泛型函数实例专用。
        [[nodiscard]] std::string mangleGenericInstance(const std::string& base,
                                                        const std::vector<sema::TypePtr>& typeArgs);

        // 函数的发射符号。
        //   base      源码里的函数名
        //   overloaded 该名字是否有多个定义（决定是否修饰）
        //   paramTypes 形参静态类型（未标注的是 TK_UNKNOWN -> 编成 v）
        [[nodiscard]] std::string functionSymbol(const std::string& base,
                                                 bool overloaded,
                                                 const std::vector<sema::TypePtr>& paramTypes);

        // 用户可见的名字 -> 实际汇编符号（只处理 main 的特例）。
        [[nodiscard]] std::string applyEntryRename(const std::string& base);

        // 方法的符号：`<Class>.<method>`
        [[nodiscard]] std::string methodSymbol(const std::string& className,
                                               const std::string& methodName);

        // 构造器的符号：`<Class>.__init__`
        [[nodiscard]] std::string constructorSymbol(const std::string& className);

        // 泛型类实例的具体类名：`Box[int]` -> `_N3Boxi`。
        [[nodiscard]] std::string classInstanceName(const std::string& base,
                                                    const std::vector<sema::TypePtr>& args);
    }
}

#endif //RCC_SYMBOL_NAMING_H
