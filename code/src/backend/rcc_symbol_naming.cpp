//
// rcc_symbol_naming.cpp —— 汇编符号命名的唯一实现
//
// 这里的每个函数都是从 CodeGen 的 private static 原样搬出来的，
// 行为**一个字都不能变**：符号名一旦漂移，rt.o / core_win64.o 的既有
// ABI 与所有已编译的目标文件就全废了。搬运时刻意保留原来的分支顺序。
//

#include "../../include/backend/rcc_symbol_naming.h"

namespace backend
{
    namespace naming
    {
        // §4.3 冻结的类型编码表。
        //
        // 2026-11-17（P13 缺陷修复）：**新增一个编码 `A`，只用于
        // 「显式写了 `any` 的类型位置」**，其余编码一个字都没动。
        //
        // 原因：`any` 在语言里等价于「不约束」，sema 层把它解析成
        // TK_UNKNOWN；而 TK_UNKNOWN 在编码表里是 `v`，与「没写标注」
        // 共用同一个字母。于是重载集里
        //     fun f(x: dict[any, any])
        //     fun f(x: dict[str, int])
        // 会拼出同一个符号 `_R1fDKv`（实测报 "Duplicate definition"）——
        // 这两份定义语义上确实不同（前者接受任意字典，后者只收 str→int），
        // 必须能共存。裸 `list` / 裸 `dict` 仍编成 `v`：那才是「没写元素类型」。
        //
        // 怎么区分二者：DictType / ListType 的类型实参为 nullptr 表示「裸容器
        // 标注」，为 TK_UNKNOWN 表示「显式写了 any」。这里据此分流，
        // 编码表其余部分保持不变（P10/P11 的冻结面不受影响）。
        std::string mangleType(const sema::TypePtr& type)
        {
            if (!type) return "v";
            switch (type->getKind())
            {
            case sema::TypeKind::TK_VOID:  return "v";
            case sema::TypeKind::TK_BOOL:  return "b";
            case sema::TypeKind::TK_CHAR:  return "c";
            case sema::TypeKind::TK_INT:   return "i";
            case sema::TypeKind::TK_FLOAT: return "f";
            case sema::TypeKind::TK_STR:   return "K";
            case sema::TypeKind::TK_PTR:   return "P";
            case sema::TypeKind::TK_LIST:
            {
                const auto& elem = std::static_pointer_cast<sema::ListType>(type)->getElemType();
                // 显式 `any` -> `A`（与「裸 list」的 `v` 区分开）
                if (elem && elem->getKind() == sema::TypeKind::TK_UNKNOWN) return "LA";
                return "L" + mangleType(elem);
            }
            case sema::TypeKind::TK_DICT:
            {
                const auto d = std::static_pointer_cast<sema::DictType>(type);
                const auto& k = d->getKeyType();
                const auto& v = d->getValueType();
                // 显式 `any` -> `A`；裸 `dict`（两侧都为空）保持旧编码 `Dvv`
                const std::string ks = (k && k->getKind() == sema::TypeKind::TK_UNKNOWN)
                                           ? std::string("A") : mangleType(k);
                const std::string vs = (v && v->getKind() == sema::TypeKind::TK_UNKNOWN)
                                           ? std::string("A") : mangleType(v);
                return "D" + ks + vs;
            }
            case sema::TypeKind::TK_FUNC:
            {
                const auto f = std::static_pointer_cast<sema::FunctionType>(type);
                std::string s = "F";
                for (const auto& p : f->getParams()) s += mangleType(p);
                s += "_";
                s += mangleType(f->getReturnType());
                return s;
            }
            case sema::TypeKind::TK_CLASS:
            {
                const auto name = std::static_pointer_cast<sema::ClassType>(type)->getName();
                return "N" + std::to_string(name.size()) + name;
            }
            // 决议 A：值语义 struct 的符号编码。必须与 class 的 `N` 分开，
            // 否则同名 struct 与 class 会在重载/mangle 上撞车。
            case sema::TypeKind::TK_STRUCT:
            {
                const auto name = std::static_pointer_cast<sema::StructType>(type)->getName();
                return "S" + std::to_string(name.size()) + name;
            }
            case sema::TypeKind::TK_GENERIC:
                return "T0";
            case sema::TypeKind::TK_POOL:
                return "Q";
            case sema::TypeKind::TK_MEMORY:
            {
                const auto mt = std::static_pointer_cast<sema::MemoryType>(type);
                const char role = mt->getRole() == sema::MemoryRole::BODY ? 'B' :
                                  mt->getRole() == sema::MemoryRole::CONTENT ? 'C' :
                                  mt->getRole() == sema::MemoryRole::REF ? 'R' :
                                  mt->getRole() == sema::MemoryRole::MUT ? 'M' : 'T';
                return std::string("Y") + role + mangleType(mt->getInnerType());
            }
            default:
                return "v";
            }
        }

        // 泛型实例名：`_G<len><name><encoded-类型实参>`。
        //
        // **2026-11-17 用户批准的 ABI 变更**（P10 §2 原文说「任何编码改动
        // 都是 ABI 破坏」，故此项须显式批准）。
        //
        // 为什么必须与重载的 `_R` 分开：两套编码原本共用同一个形状
        // `_R<len><name><encoded>`，只是被编码的东西不同 —— 泛型编**类型实参**，
        // 重载编**形参类型**。撞车实例（实测）：
        //     fun toStr[T](arg: list[T])   T=list[int]  ->  _R5toStr + "Li"
        //     fun toStr(arg: int)                        ->  _R5toStr + "i"
        // 名字拼出来都是 `_R5toStri`，汇编器报
        //     symbol `_R5toStri' is already defined
        // 这不是「编码写得不够细」—— 两套语义本来就该分属不同命名空间，
        // 因为 `T` 的绑定与形参类型毫无关系。
        //
        // 换成 `_G` 前缀后：泛型实例 `_G5toStrLi`，重载 `_R5toStri`，互不干扰。
        // 注意 `_G` 编号码的内容**仍然是类型实参**（语义不变），只是前缀不同。
        std::string mangleGenericInstance(const std::string& base,
                                          const std::vector<sema::TypePtr>& typeArgs)
        {
            std::string s = "_G" + std::to_string(base.size()) + base;
            for (const auto& a : typeArgs) s += mangleType(a);
            return s;
        }

        std::string mangleSignature(const std::string& base,
                                    const std::vector<sema::TypePtr>& args)
        {
            std::string s = "_R" + std::to_string(base.size()) + base;
            for (const auto& a : args) s += mangleType(a);
            return s;
        }

        std::string functionSymbol(const std::string& base,
                                   bool overloaded,
                                   const std::vector<sema::TypePtr>& paramTypes)
        {
            // 唯一名字保持裸名 —— 这是「P11 不改运行时」的硬前提：
            // rt.o / core_win64.o 里的符号全是不带修饰的裸名。
            if (!overloaded) return applyEntryRename(base);
            return mangleSignature(base, paramTypes);
        }

        std::string applyEntryRename(const std::string& base)
        {
            return base == "main" ? std::string(USER_MAIN_SYMBOL) : base;
        }

        std::string methodSymbol(const std::string& className,
                                 const std::string& methodName)
        {
            return className + "." + methodName;
        }

        std::string constructorSymbol(const std::string& className)
        {
            return className + CTOR_SYMBOL_SUFFIX;
        }

        std::string classInstanceName(const std::string& base,
                                      const std::vector<sema::TypePtr>& args)
        {
            // 类实例名用 `_N` 前缀，和泛型函数的 `_G`、重载的 `_R` 三者分开
            // —— 否则 `Box[int]` 也会和某个 `_R3Boxi` 撞车（同一类比缺陷）。
            std::string s = "_N" + std::to_string(base.size()) + base;
            for (const auto& a : args) s += mangleType(a);
            return s;
        }
    }
}
