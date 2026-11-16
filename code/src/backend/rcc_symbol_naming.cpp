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
                return "L" + mangleType(std::static_pointer_cast<sema::ListType>(type)->getElemType());
            case sema::TypeKind::TK_DICT:
            {
                const auto d = std::static_pointer_cast<sema::DictType>(type);
                return "D" + mangleType(d->getKeyType()) + mangleType(d->getValueType());
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
            case sema::TypeKind::TK_GENERIC:
                return "T0";
            default:
                return "v";
            }
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
            return mangleSignature(base, args);
        }
    }
}
