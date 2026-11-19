//
// rcc_sema_types.h —— 新语义层的类型表示
//
// 设计要点：
//   * 静态语言，每个表达式有确定的类型，不存在 any 的运行时 tag
//   * 值表示与 Win64 ABI 对齐（见 docs/architecture_plan.md §4.2）
//   * str / list / dict 一律用单指针表示（指向带头的堆对象），
//     避免 16 字节结构在 MS x64 下按引用传递带来的额外间接寻址
//

#ifndef RCC_SEMA_TYPES_H
#define RCC_SEMA_TYPES_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sema
{
    enum class TypeKind
    {
        TK_VOID,
        TK_BOOL,
        TK_CHAR,
        TK_INT,
        TK_FLOAT,
        TK_STR,
        TK_LIST,
        TK_DICT,
        TK_FUNC,
        TK_CLASS,
        TK_STRUCT,    // A：值语义结构体（docs/pending_designs.md §A）
        TK_PTR,
        TK_GENERIC,   // P10：泛型形参占位（`fun f[T](x: T)` 里的 T）
        TK_MEMORY,    // P13-2/L3：内存模型特殊类型（Body/Content/Ref/Mut/Take）
        TK_POOL,      // P13-2/L3-A：Pool 类型（标准库普通类型，管理对象出生地）
        TK_LINK,      // P13-7：Link 成对引用类型（Link[T][peer][card]）
        TK_I32,       // P14-4：32 位整数（原子 i32 宽度；Rio 的 int 是 64 位）
        TK_LOCKED,    // P14-4：显式锁 Locked[T]（内部可变三类型之一）
        TK_ATOMIC,    // P14-4：原子 Atomic[T]（内部可变三类型之一）
        TK_CHAN,      // P14-5：通道 Chan[T]（内部可变三类型之一，承载 Content[T]）
        TK_NULLABLE,  // B：可空类型 `T?`（只有引用型可空）
        TK_UNKNOWN,   // 尚未推导出，用于报错前占位
    };

    // 值在内存/寄存器中的分类，决定用哪套寄存器与指令
    enum class ValueClass
    {
        VC_INTEGER,   // 走 GP 寄存器（int/bool/char/ptr/所有指针型）
        VC_SSE,       // 走 XMM 寄存器（float）
        VC_VOID,
    };

    class Type;
    using TypePtr = std::shared_ptr<Type>;

    class Type
    {
    public:
        explicit Type(TypeKind kind) : _kind(kind) {}

        [[nodiscard]] TypeKind getKind() const { return _kind; }

        [[nodiscard]] bool isVoid() const { return _kind == TypeKind::TK_VOID; }
        [[nodiscard]] bool isInteger() const
        {
            return _kind == TypeKind::TK_INT || _kind == TypeKind::TK_I32;
        }
        [[nodiscard]] bool isFloat() const { return _kind == TypeKind::TK_FLOAT; }
        [[nodiscard]] bool isPointerLike() const
        {
            return _kind == TypeKind::TK_STR || _kind == TypeKind::TK_LIST ||
                   _kind == TypeKind::TK_DICT || _kind == TypeKind::TK_CLASS ||
                   _kind == TypeKind::TK_PTR || _kind == TypeKind::TK_MEMORY ||
                   _kind == TypeKind::TK_POOL || _kind == TypeKind::TK_LINK ||
                   _kind == TypeKind::TK_LOCKED || _kind == TypeKind::TK_ATOMIC ||
                   _kind == TypeKind::TK_CHAN;
        }
        [[nodiscard]] bool isNumeric() const { return isInteger() || isFloat(); }

        [[nodiscard]] ValueClass getValueClass() const
        {
            if (_kind == TypeKind::TK_VOID) return ValueClass::VC_VOID;
            if (_kind == TypeKind::TK_FLOAT) return ValueClass::VC_SSE;
            return ValueClass::VC_INTEGER;
        }

        // 在栈上/寄存器中占用的字节数。
        //
        // 值语义 struct 的大小不是固定值，要由后端按字段布局算出来，所以
        // 这一项是 virtual：StructType 覆盖它返回登记时写入的字节数。
        [[nodiscard]] virtual size_t getSize() const
        {
            switch (_kind)
            {
            case TypeKind::TK_VOID:  return 0;
            case TypeKind::TK_BOOL:  return 1;
            case TypeKind::TK_CHAR:  return 4;
            case TypeKind::TK_I32:   return 4;   // P14-4：32 位整数
            default:              return 8;   // int/float/所有指针型
            }
        }

        [[nodiscard]] virtual std::string toString() const
        {
            switch (_kind)
            {
            case TypeKind::TK_VOID:  return "void";
            case TypeKind::TK_BOOL:  return "bool";
            case TypeKind::TK_CHAR:  return "char";
            case TypeKind::TK_INT:   return "int";
            case TypeKind::TK_FLOAT: return "float";
            case TypeKind::TK_STR:   return "str";
            case TypeKind::TK_LIST:  return "list";
            case TypeKind::TK_DICT:  return "dict";
            case TypeKind::TK_FUNC:  return "func";
            case TypeKind::TK_CLASS: return "class";
            case TypeKind::TK_STRUCT: return "struct";
            case TypeKind::TK_PTR:   return "ptr";
            case TypeKind::TK_MEMORY: return "memory";
            case TypeKind::TK_POOL:   return "Pool";
            case TypeKind::TK_LINK:   return "Link";
            case TypeKind::TK_I32:    return "i32";
            case TypeKind::TK_LOCKED: return "Locked";
            case TypeKind::TK_ATOMIC: return "Atomic";
            case TypeKind::TK_CHAN:   return "Chan";
            case TypeKind::TK_NULLABLE: return "nullable";
            default:              return "unknown";
            }
        }

        [[nodiscard]] virtual bool equals(const TypePtr& other) const
        {
            return other && _kind == other->_kind;
        }

    protected:
        TypeKind _kind;
    };

    // ---------- 预定义基础类型（Meyers singleton，避免静态初始化顺序问题） ----------
    inline TypePtr getVoidType()  { static auto t = std::make_shared<Type>(TypeKind::TK_VOID);  return t; }
    inline TypePtr getBoolType()  { static auto t = std::make_shared<Type>(TypeKind::TK_BOOL);  return t; }
    inline TypePtr getCharType()  { static auto t = std::make_shared<Type>(TypeKind::TK_CHAR);  return t; }
    inline TypePtr getIntType()   { static auto t = std::make_shared<Type>(TypeKind::TK_INT);   return t; }
    inline TypePtr getFloatType() { static auto t = std::make_shared<Type>(TypeKind::TK_FLOAT); return t; }
    inline TypePtr getStrType()   { static auto t = std::make_shared<Type>(TypeKind::TK_STR);   return t; }
    inline TypePtr getPtrType()   { static auto t = std::make_shared<Type>(TypeKind::TK_PTR);   return t; }
    inline TypePtr getUnknownType(){ static auto t = std::make_shared<Type>(TypeKind::TK_UNKNOWN); return t; }

    // ---------- 容器类型（P5） ----------
    //
    // list/dict 与 str 一样用**单指针**表示，指向带头的堆对象（§4.2）。
    // 元素类型只在编译期有意义：它决定索引表达式的静态类型
    // （`sout(dic["name"])` 必须推成 str、`sout(dic["age"])` 必须推成 int），
    // 运行时的元素槽一律是 8 字节。
    //
    // 为什么需要专门的子类而不是只留 TK_LIST/TK_DICT 两个枚举：
    // 没有元素类型，索引表达式推不出结果类型，sout 就无法在编译期
    // 选对 rt_print_* —— 而动态 tag 方案已经在 P1 被彻底废弃。
    class ListType final : public Type
    {
    public:
        explicit ListType(TypePtr elem)
            : Type(TypeKind::TK_LIST), _elem(std::move(elem)) {}

        [[nodiscard]] const TypePtr& getElemType() const { return _elem; }

        [[nodiscard]] std::string toString() const override
        {
            return "list[" + (_elem ? _elem->toString() : std::string("?")) + "]";
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_LIST) return false;
            const auto o = std::static_pointer_cast<ListType>(other);
            if (!_elem || !o->_elem) return !_elem && !o->_elem;
            return _elem->equals(o->_elem);
        }

    private:
        TypePtr _elem;
    };

    class DictType final : public Type
    {
    public:
        DictType(TypePtr key, TypePtr val)
            : Type(TypeKind::TK_DICT), _key(std::move(key)), _val(std::move(val)) {}

        [[nodiscard]] const TypePtr& getKeyType() const { return _key; }
        [[nodiscard]] const TypePtr& getValueType() const { return _val; }

        // ---------- 字面量键的类型精化 ----------
        //
        // 静态语言里 `{"name": "Rio", "age": 18}` 这类异构字典的正确解法是
        // 和类型（any），那是 P6/P10 的事。P5 用一条更窄但确定的规则：
        // 字典**字面量**的每个字符串键都记下它自己那个值的静态类型，
        // 于是 `dic["name"]` 能推成 str、`dic["age"]` 能推成 int，
        // sout 也就能在编译期选对 rt_print_*。
        //
        // 只在键是字符串字面量时登记；键是变量或表达式时退回 _val
        // （异构字面量的 _val 统一退化成 int，因为运行时槽位恒为 8 字节）。
        // 这张表是纯编译期信息，不参与 equals —— 两个 dict 类型相等与否
        // 只看键类型和值类型，避免精化表让本来相同的类型变得不相等。
        void setKeyTypes(std::map<std::string, TypePtr> keyTypes)
        {
            _keyTypes = std::move(keyTypes);
        }

        [[nodiscard]] const std::map<std::string, TypePtr>& getKeyTypes() const { return _keyTypes; }

        [[nodiscard]] TypePtr getKeyTypeOf(const std::string& key) const
        {
            const auto found = _keyTypes.find(key);
            return found == _keyTypes.end() ? nullptr : found->second;
        }

        [[nodiscard]] std::string toString() const override
        {
            return "dict[" + (_key ? _key->toString() : std::string("?")) + ", " +
                   (_val ? _val->toString() : std::string("?")) + "]";
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_DICT) return false;
            const auto o = std::static_pointer_cast<DictType>(other);
            const bool keyEq = (!_key || !o->_key) ? (!_key && !o->_key) : _key->equals(o->_key);
            const bool valEq = (!_val || !o->_val) ? (!_val && !o->_val) : _val->equals(o->_val);
            return keyEq && valEq;
        }

    private:
        TypePtr _key;
        TypePtr _val;
        std::map<std::string, TypePtr> _keyTypes;
    };

    // ---------- 类类型（P6） ----------
    //
    // 对象是**单指针**（与 str/list/dict 一致），指向一块扁平的 8 字节槽数组：
    //   { field0, field1, ..., fieldN-1 }   第 i 个字段在 +8*i
    // 基类的字段排在派生类字段之前，所以 `Derived` 的对象可以安全地当作
    // `Base` 用（字段偏移一致），这也是 super(...) 能把同一个 this 传给基类
    // 构造器的前提。
    //
    // ClassType 只携带**类名**，不携带布局：布局（字段偏移、方法表）属于
    // 后端，放在 CodeGen 的类登记表里。sema 层不该依赖 backend，
    // 所以这里只留一个名字，equals 按名字比较。
    class ClassType final : public Type
    {
    public:
        explicit ClassType(std::string name)
            : Type(TypeKind::TK_CLASS), _name(std::move(name)) {}

        [[nodiscard]] const std::string& getName() const { return _name; }

        [[nodiscard]] std::string toString() const override
        {
            return _name.empty() ? std::string("class") : _name;
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_CLASS) return false;
            return _name == std::static_pointer_cast<ClassType>(other)->_name;
        }

    private:
        std::string _name;
    };

    // ---------- A：值语义 struct 类型 ----------
    //
    // 与 ClassType 的关键差别（docs/pending_designs.md §A）：
    //   * 对象**不是**指针，而是按字段顺序内联的一块值；`var b = a` 逐字节拷贝
    //   * 因此它**不是** isPointerLike，不参与指针族互通，也不进 Content 状态格
    //   * 大小由后端按字段布局算出来（getSize 返回登记时的字节数），
    //     而不是固定的 8 字节 —— 这是 Type::getSize 变成 virtual 的唯一原因
    // sema 层只带名字和大小，字段表（偏移/类型）在后端 CodeGen 的 _structs 里。
    class StructType final : public Type
    {
    public:
        StructType(std::string name, size_t size)
            : Type(TypeKind::TK_STRUCT), _name(std::move(name)), _size(size) {}

        [[nodiscard]] const std::string& getName() const { return _name; }

        [[nodiscard]] size_t getSize() const override { return _size; }

        void setSize(size_t size) { _size = size; }

        [[nodiscard]] std::string toString() const override
        {
            return _name.empty() ? std::string("struct") : _name;
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_STRUCT) return false;
            return _name == std::static_pointer_cast<StructType>(other)->_name;
        }

    private:
        std::string _name;
        size_t _size;
    };

    // ---------- 函数类型 ----------
    class FunctionType final : public Type
    {
    public:
        FunctionType(std::vector<TypePtr> params, TypePtr ret)
            : Type(TypeKind::TK_FUNC), _params(std::move(params)), _ret(std::move(ret)) {}

        [[nodiscard]] const std::vector<TypePtr>& getParams() const { return _params; }
        [[nodiscard]] const TypePtr& getReturnType() const { return _ret; }

        [[nodiscard]] std::string toString() const override
        {
            std::string s = "fun(";
            for (size_t i = 0; i < _params.size(); ++i)
            {
                if (i) s += ", ";
                s += _params[i] ? _params[i]->toString() : "?";
            }
            s += "): ";
            s += _ret ? _ret->toString() : "void";
            return s;
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_FUNC) return false;
            const auto o = std::static_pointer_cast<FunctionType>(other);
            if (_params.size() != o->_params.size()) return false;
            for (size_t i = 0; i < _params.size(); ++i)
            {
                if (!_params[i]->equals(o->_params[i])) return false;
            }
            return _ret->equals(o->_ret);
        }

    private:
        std::vector<TypePtr> _params;
        TypePtr _ret;
    };

    // ---------- 泛型形参占位（P10） ----------
    //
    // `fun identity[T](x: T): T` 在**声明**阶段，T 不是一个真实类型，而是
    // 「待代入的洞」。这里用一个带名字的占位类型表示它：
    //   * 声明登记时：paramTypes 里出现 GenericType("T")，不做校验、不分配槽位
    //   * 单态化时：substituteGenerics() 把 T 换成实参类型，得到具体签名
    //
    // 之所以要有独立的 TypeKind 而不是复用 TK_UNKNOWN：TK_UNKNOWN 的语义是
    // 「解析失败」，到处都按「回退 int」处理；而 T 是**合法**的未知，
    // 必须参与 equals/substitute 而不是被当成错误吞掉。
    class GenericType final : public Type
    {
    public:
        explicit GenericType(std::string name)
            : Type(TypeKind::TK_GENERIC), _name(std::move(name)) {}

        [[nodiscard]] const std::string& getName() const { return _name; }

        [[nodiscard]] std::string toString() const override { return _name; }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_GENERIC) return false;
            return _name == std::static_pointer_cast<GenericType>(other)->_name;
        }

    private:
        std::string _name;
    };

    // ---------- P13-2/L3-A：Pool 类型 ----------
    // Pool 是标准库普通类型（决议 §1.1），不做关键字。这里只承载类型身份，
    // 具体分配策略（fixed/boxed/chunked）由内建静态方法处理（codegen）。
    class PoolType final : public Type
    {
    public:
        PoolType() : Type(TypeKind::TK_POOL) {}
        [[nodiscard]] std::string toString() const override { return "Pool"; }
        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            return other && other->getKind() == TypeKind::TK_POOL;
        }
    };
    inline TypePtr getPoolType()
    {
        static auto t = std::make_shared<PoolType>();
        return t;
    }

    // ---------- P14-4：i32 / Locked[T] / Atomic[T] ----------
    // i32 只在原子宽度与符号扩展上有别于 int（int 是 64 位），值类别同为整数。
    class I32Type final : public Type
    {
    public:
        I32Type() : Type(TypeKind::TK_I32) {}
        [[nodiscard]] std::string toString() const override { return "i32"; }
        [[nodiscard]] size_t getSize() const override { return 4; }
        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            return other && other->getKind() == TypeKind::TK_I32;
        }
    };
    inline TypePtr getI32Type()
    {
        static auto t = std::make_shared<I32Type>();
        return t;
    }

    // Locked[T] / Atomic[T] 都是「包装一个 T」的指针族类型，值本身是指向
    // { lockword; T value } 的堆对象句柄（Atomic 无 lockword，仅值槽）。
    class SyncWrapperType final : public Type
    {
    public:
        SyncWrapperType(TypeKind kind, TypePtr inner)
            : Type(kind), _inner(std::move(inner)) {}
        [[nodiscard]] const TypePtr& getInnerType() const { return _inner; }
        [[nodiscard]] std::string toString() const override
        {
            const char* n = _kind == TypeKind::TK_LOCKED ? "Locked"
                          : _kind == TypeKind::TK_CHAN   ? "Chan" : "Atomic";
            return std::string(n) + "[" + (_inner ? _inner->toString() : "?") + "]";
        }
        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != _kind) return false;
            const auto o = std::static_pointer_cast<SyncWrapperType>(other);
            if (!_inner || !o->_inner) return !_inner && !o->_inner;
            return _inner->equals(o->_inner);
        }
    private:
        TypePtr _inner;
    };
    inline TypePtr getLockedType(const TypePtr& inner)
    {
        return std::make_shared<SyncWrapperType>(TypeKind::TK_LOCKED, inner);
    }
    inline TypePtr getAtomicType(const TypePtr& inner)
    {
        return std::make_shared<SyncWrapperType>(TypeKind::TK_ATOMIC, inner);
    }
    // P14-5：Chan[T] 与 Locked/Atomic 同构（包装一个元素体类型的指针族）。
    inline TypePtr getChanType(const TypePtr& inner)
    {
        return std::make_shared<SyncWrapperType>(TypeKind::TK_CHAN, inner);
    }

    // ---------- P13-7：Link 成对引用类型 ----------
    //
    // `Link[T][peer][cardinality]`：对称关系（反向边、父子、双向链表）的
    // 唯一内建原语。elem 是对端元素类型，peer 是对端字段名，cardinality
    // 是基数（opt 至多一个 / one 连接后恰好一个 / many 任意多个）。
    //
    // 决议（p13_7_syntax_audit.md §6）：
    //   Q2=A 独立 LinkType 子类（不动 MemoryType）；
    //   Q3=A equals/mangle 只比 elem，peer/card 不入签名，对偶校验在 struct 层做。
    enum class LinkCardinality
    {
        OPT,
        ONE,
        MANY,
    };

    class LinkType final : public Type
    {
    public:
        LinkType(TypePtr elem, std::string peer, LinkCardinality card)
            : Type(TypeKind::TK_LINK), _elem(std::move(elem)),
              _peer(std::move(peer)), _card(card) {}

        [[nodiscard]] const TypePtr& getElemType() const { return _elem; }
        [[nodiscard]] const std::string& getPeer() const { return _peer; }
        [[nodiscard]] LinkCardinality getCardinality() const { return _card; }

        [[nodiscard]] static const char* cardinalityName(LinkCardinality card)
        {
            switch (card)
            {
            case LinkCardinality::OPT:  return "opt";
            case LinkCardinality::ONE:  return "one";
            case LinkCardinality::MANY: return "many";
            }
            return "?";
        }

        [[nodiscard]] std::string toString() const override
        {
            return "Link[" + (_elem ? _elem->toString() : "?") + "][" + _peer +
                   "][" + cardinalityName(_card) + "]";
        }

        // Q3=A：只有 elem 参与类型相等；peer/card 是对偶关系元数据，
        // 不影响函数签名形状，对偶一致性在 struct 定义层校验。
        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_LINK) return false;
            const auto o = std::static_pointer_cast<LinkType>(other);
            if (!_elem || !o->_elem) return !_elem && !o->_elem;
            return _elem->equals(o->_elem);
        }

    private:
        TypePtr _elem;
        std::string _peer;
        LinkCardinality _card;
    };

    // ---------- P13-2/L3：内存模型特殊类型 ----------
    //
    // Body[T]/Content[T]/Ref[T]/Mut[T]/Take[T] 都是「包装一个 T」的类型，
    // 行为同构，用同一个 TypeKind + 角色枚举表示，避免五个几乎相同的子类。
    // 决议 §4：Player（裸类名）与 Body[Player] 是同一类型（别名），
    // 所以裸类名仍解析成 ClassType；只有显式写 Body[...] 才得到 MemoryType。
    enum class MemoryRole
    {
        BODY,
        CONTENT,
        REF,
        MUT,
        TAKE,
    };

    class MemoryType final : public Type
    {
    public:
        MemoryType(MemoryRole role, TypePtr inner)
            : Type(TypeKind::TK_MEMORY), _role(role), _inner(std::move(inner)) {}

        [[nodiscard]] MemoryRole getRole() const { return _role; }
        [[nodiscard]] const TypePtr& getInnerType() const { return _inner; }

        [[nodiscard]] std::string toString() const override
        {
            const char* name = "?";
            switch (_role)
            {
            case MemoryRole::BODY:    name = "Body";    break;
            case MemoryRole::CONTENT: name = "Content"; break;
            case MemoryRole::REF:     name = "Ref";     break;
            case MemoryRole::MUT:     name = "Mut";     break;
            case MemoryRole::TAKE:    name = "Take";    break;
            }
            return std::string(name) + "[" + (_inner ? _inner->toString() : "?") + "]";
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_MEMORY) return false;
            const auto o = std::static_pointer_cast<MemoryType>(other);
            if (_role != o->_role) return false;
            if (!_inner || !o->_inner) return !_inner && !o->_inner;
            return _inner->equals(o->_inner);
        }

    private:
        MemoryRole _role;
        TypePtr _inner;
    };

    // 决议 B：可空类型 `T?`。
    //
    // 语义：这个位置上可能「有值」，也可能是 null（缺失）。只有**引用型**
    // 才允许可空 —— 用户 2026-11-17 裁决（memory 388a73e0）：值类型没有
    // 「空」这个状态，硬造一个零值等于把 0 和「没有」混为一谈。
    //
    // 在代码生成层面它和 inner 完全一样（指针就是指针，null 就是 0），
    // 单独一个类型只是为了让 `if var p = r.get()` 能校验 RHS 确实是 T?，
    // 并让 `T?` 与 `T` 不相等（否则可空校验形同虚设）。
    class NullableType final : public Type
    {
    public:
        explicit NullableType(TypePtr inner)
            : Type(TypeKind::TK_NULLABLE), _inner(std::move(inner)) {}

        [[nodiscard]] const TypePtr& getInnerType() const { return _inner; }

        [[nodiscard]] size_t getSize() const override
        {
            return _inner ? _inner->getSize() : 8;
        }

        [[nodiscard]] std::string toString() const override
        {
            return (_inner ? _inner->toString() : std::string("?")) + "?";
        }

        [[nodiscard]] bool equals(const TypePtr& other) const override
        {
            if (!other || other->getKind() != TypeKind::TK_NULLABLE) return false;
            const auto o = std::static_pointer_cast<NullableType>(other);
            if (!_inner || !o->_inner) return !_inner && !o->_inner;
            return _inner->equals(o->_inner);
        }

    private:
        TypePtr _inner;
    };

    inline TypePtr getGenericType(const std::string& name)
    {
        return std::make_shared<GenericType>(name);
    }
}

#endif //RCC_SEMA_TYPES_H
