//
// Created by RestRegular on 2025/7/20.
//

#ifndef RCC_COMPILER_VISITOR_H
#define RCC_COMPILER_VISITOR_H

#include <queue>

#include "../analyzer/rcc_lexer.h"
#include "../components/ri/rcc_ri.h"
#include "../components/symbol/rcc_symbol.h"
#include "../interfaces/rcc_compile_interface.h"

namespace builtin
{
    struct CallInfos;
}

namespace ast
{
    class ContentBuilder
    {
        std::stack<std::stringstream> raCodeStack{};
        std::string raCode;

    public:
        ContentBuilder();
        ~ContentBuilder() = default;
        std::string buildAll();
        ContentBuilder& appendCode(const std::string& code);
        ContentBuilder& operator<<(const std::string& code);
        ContentBuilder& operator<<(const ri::RI& ri);
        void enterScope();
        void exitScope();
        ContentBuilder& buildCurScope(std::string& target);
        ContentBuilder& buildCurScope(ContentBuilder& builder);
        ContentBuilder& operator>>(std::string& target);
        ContentBuilder& operator>>(ContentBuilder& builder);
        [[nodiscard]] std::string getCurScopeResult() const;
    };

    // 变量ID封装类
    class VarID final : public Object
    {
        static size_t _varId; // 变量ID计数器
        static std::string rccVarPrefixField; // 变量前缀
        size_t id = 0; // 当前变量ID
        std::string fileField, scopeField, nameField; // 文件域、作用域、名称
        std::string vid; // 变量唯一标识
        size_t scopeLevel = 0;
        [[nodiscard]] std::string _toVarID(); // 生成变量ID字符串

    public:
        // 构造函数
        VarID(const std::string& name,
              const std::string& fileField,
              const std::string& scopeField,
              const size_t& scopeLevel);

        // 字符串转换
        [[nodiscard]] std::string toString() const override;

        // Getter方法
        [[nodiscard]] std::string getNameField() const;
        [[nodiscard]] std::string getScopeField() const;
        [[nodiscard]] std::string getFileField() const;
        [[nodiscard]] size_t getScopeLevel() const;
        [[nodiscard]] size_t getId() const;
        [[nodiscard]] std::string getVid() const;

        // 输出运算符重载
        friend std::ostream& operator<<(std::ostream& os, const VarID& varID)
        {
            os << varID.vid;
            return os;
        }
    };

    // 标签ID封装类
    class SetID final : public Object
    {
        static size_t _setId; // 集合ID计数器
        static std::string rccSetPrefixField; // 集合前缀
        size_t id = 0; // 当前集合ID
        std::string fileField, scopeField, nameField; // 文件域、作用域、名称
        std::string sid; // 集合唯一标识
        [[nodiscard]] std::string _toSetID(); // 生成集合ID字符串

    public:
        // 构造函数
        SetID(const std::string& name,
              const std::string& fileField,
              const std::string& scopeField);

        // 字符串转换
        [[nodiscard]] std::string toString() const override;

        // Getter方法
        [[nodiscard]] std::string getNameField() const;
        [[nodiscard]] std::string getScopeField() const;
        [[nodiscard]] std::string getFileField() const;
        [[nodiscard]] size_t getId() const;
        [[nodiscard]] std::string getSid() const;

        // 输出运算符重载
        friend std::ostream& operator<<(std::ostream& os, const SetID& setID)
        {
            os << setID.sid;
            return os;
        }
    };

    // 操作数项类
    class OpItem final:
    public Object,
    public IRCCOpItemInterface
    {
        std::string value; // 原始值
        std::string raValue; // RA代码值
        OpItemType type; // 操作数类型
        std::shared_ptr<symbol::TypeLabelSymbol> typeLabel; // 类型标签
        std::shared_ptr<symbol::TypeLabelSymbol> valueType; // 值类型
        std::shared_ptr<symbol::Symbol> belonging; // 归属符号
        std::string belongAttrRaValue; // 属性名称
        std::shared_ptr<symbol::Symbol> referencedSymbol; // 引用符号
        Pos pos;

    public:
        // 构造函数
        explicit OpItem(
            const OpItemType& type,
            const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabelSymbol,
            const std::string& value,
            const std::string& raValue = "",
            const std::shared_ptr<symbol::TypeLabelSymbol>& valueType = nullptr,
            const std::shared_ptr<symbol::Symbol>& referencedSymbol = nullptr,
            const Pos &pos = getUnknownPos());

        [[nodiscard]] bool is(const OpItemType& opItemType) const;
        [[nodiscard]] bool isNot(const OpItemType& opItemType) const;
        [[nodiscard]] std::string toString() const override;

        // Getter方法
        [[nodiscard]] Pos getPos() const;
        void setPos(const Pos &pos_);
        [[nodiscard]] std::string getVal() const;
        [[nodiscard]] OpItemType getType() const;
        [[nodiscard]] std::string getRaVal(const symbol::SymbolTableManager& table,
                                           const bool& needSearch = true) const;
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getTypeLabel() const;
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getValueType() const;
        [[nodiscard]] std::shared_ptr<symbol::Symbol> getBelonging() const;
        [[nodiscard]] std::string getBelongAttrRaValue() const;
        [[nodiscard]] std::shared_ptr<symbol::Symbol> getReferencedSymbol() const;

        // Setter方法
        void setTypeLabel(const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabelSymbol);
        void setValueType(const std::shared_ptr<symbol::TypeLabelSymbol>& valueTypeSymbol);
        void setBelonging(const std::shared_ptr<symbol::Symbol>& belongingSymbol,
                          const std::string& belongAttrRaVal);
        void setReferencedSymbol(const std::shared_ptr<symbol::Symbol>& symbol);

        bool Is(const OpItemType& opItemType) override;
        bool IsNot(const OpItemType& opItemType) override;
        const char* ToString() const override;
        const char* GetVal() const override;
        const char* GetRaVal(const symbol::IRCCSymbolTableManagerInterface* table,
            const bool& needSearch) const override;
        OpItemType GetType() const override;
        symbol::IRCCTypeLabelSymbolInterface* GetTypeLabelSymbolI() const override;
        symbol::IRCCTypeLabelSymbolInterface* GetValueTypeSymbolI() const override;
        symbol::IRCCSymbolInterface* GetBelongingSymbolI() const override;
        const char* GetBelongAttrRaValue() const override;
        symbol::IRCCSymbolInterface* GetReferencedSymbol() const override;
        void SetTypeLabel(const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI) override;
        void SetValueType(const symbol::IRCCTypeLabelSymbolInterface* valueTypeSymbolI) override;
        void SetBelonging(const symbol::IRCCSymbolInterface* belongingSymbolI,
            const char* belongAttrRaVal) override;
        void SetReferencedSymbol(const symbol::IRCCSymbolInterface* symbolI) override;
        const OpItem* TransformToOII() const override;
    };

    class CompileVisitor final :
    public Visitor,
    public IRCCCompileInterface
    {
    public:
        // =========================== 标志属性 ============================
        static OutputFormat _output_format;
        static bool _symbol_flag;
        static bool _symbol_flag_only_export_option;

    private:

        // ========================== 静态成员属性 ==========================
        static size_t _temVarId; // 临时变量ID计数器
        static size_t _setId; // 集合标签ID计数器
        static std::unordered_map<std::string,
                                  std::shared_ptr<symbol::ClassSymbol>> extensionMap; // 扩展类映射
        static std::stack<std::shared_ptr<lexer::Lexer>> _lexers; // 词法分析器
        static std::list<std::string> _lexer_paths; // 词法分析器路径向量
        static std::string fileRecord;
        static std::unordered_map<std::string, std::string> extensionPathNameMap;
        static std::stack<std::string> processingExtensionStack;

        // ========================== 成员属性 ==========================
        ContentBuilder raCodeBuilder {}; // RA代码构建器
        ContentBuilder analyzeBuilder {}; // 分析结果构建器
        symbol::SymbolTableManager symbolTable{}; // 符号表管理器

        std::string programEntryFilePath; // 程序入口文件路径
        std::string programTagetFilePath; // 程序目标文件路径
        std::string compileOutputFilePath; // 编译输出文件路径
        std::string currentProcessingFilePath; // 当前正在处理的文件路径
        bool needSaveOutputToFile; // 是否保存编译输出文件

        // 处理栈
        std::stack<std::shared_ptr<symbol::Symbol>> processingSymbolStack{}; // 符号处理栈
        std::stack<std::shared_ptr<OpItem>> opItemStack{}; // 操作数栈
        std::stack<ScopeType> scopeTypeStack{}; // 作用域类型栈
        std::stack<ScopeType> loopScopeStack{}; // 循环作用域栈
        std::unordered_map<std::string, std::shared_ptr<VarID>> varIdMap {};

    public:
        // ======================= constructor ========================
        explicit CompileVisitor(
            const std::string& programEntryFilePath,
            const std::string& programTargetFilePath,
            const std::string& compileOutputFilePath = "",
            const bool& needSaveOutput = true);

        // ====================== getter & setter =====================
        [[nodiscard]] symbol::SymbolTableManager& getSymbolTable();
        [[nodiscard]] ContentBuilder& getRaCodeBuilder();
        [[nodiscard]] ContentBuilder& getAnalyzeBuilder();
        [[nodiscard]] std::stack<std::shared_ptr<symbol::Symbol>>& getProcessingSymbolStack();
        [[nodiscard]] std::stack<std::shared_ptr<OpItem>>& getOpItemStack();
        [[nodiscard]] std::stack<ScopeType>& getScopeTypeStack();
        [[nodiscard]] std::string getProgramEntryFilePath() const;
        [[nodiscard]] std::string getProgramTargetFilePath() const;
        [[nodiscard]] std::string getCompileOutputFilePath() const;
        [[nodiscard]] std::string getCurrentProcessingFilePath() const;
        void setCurrentProcessingFilePath(const std::string& filePath);
        static void enableDebugMode(bool cond);

        // ========================== 公共方法 ==========================
        // 作用域管理
        static std::string scopeTypeToString(ScopeType scopeType); // 作用域类型转字符串
        static std::string scopeTypeToFormatString(ScopeType scopeType);
        [[nodiscard]] std::string curScopeField() const; // 获取当前作用域字段
        [[nodiscard]] ScopeType curScopeType() const; // 获取当前作用域字段
        void enterScope(ScopeType scopeType); // 进入作用域
        void enterScope(size_t scopeLevel); // 进入指定层级作用域
        void enterTopScope(); // 进入顶层作用域
        void enterGlobalScope(); // 进入全局作用域
        void exitScope(ScopeType scopeType); // 退出作用域
        [[nodiscard]] size_t curScopeLevel() const; // 获取当前作用域层级
        void enterLoopScope(); // 进入循环作用域
        void exitLoopScope(); // 退出循环作用域
        [[nodiscard]] bool isInLoopScope() const; // 是否在循环作用域中

        // 类型检查与转换
        [[nodiscard]] static bool checkTypeMatch(
            const std::shared_ptr<symbol::TypeLabelSymbol>& leftTypeSymbol,
            const std::shared_ptr<symbol::TypeLabelSymbol>& rightTypeSymbol,
            const bool& restrict = true); // 检查类型匹配
        [[nodiscard]] static bool checkTypeMatch(
            const std::shared_ptr<symbol::Symbol>& leftSymbol,
            const std::shared_ptr<symbol::Symbol>& rightSymbol); // 检查符号类型匹配
        static Pos currentPos();
        static void pushProcessingPos(const Pos& pos);
        static void popProcessingPos();
        static std::stack<Pos> _currentProcessingPos;
        [[nodiscard]] bool checkTypeMatch(
            const std::shared_ptr<symbol::Symbol>& leftSymbol,
            const OpItem& rightOpItem) const; // 检查符号与操作数类型匹配
        void processTypeAutoChange(
            const std::shared_ptr<symbol::Symbol>& sourceSymbol,
            const OpItem& targetOpItem) const; // 处理类型自动转换
        void processTypeAutoChange(
            const std::shared_ptr<symbol::Symbol>& sourceSymbol,
            const std::shared_ptr<OpItem>& targetOpItem) const; // 处理类型自动转换（智能指针版）

        // 节点处理
        static std::vector<std::shared_ptr<ExpressionNode>> visitParallelNode(
            const std::shared_ptr<ExpressionNode>& node); // 访问并行节点
        std::unordered_set<std::shared_ptr<symbol::LabelSymbol>> processLabelNodes(
            const std::vector<std::shared_ptr<LabelNode>>& labels); // 处理标签节点
        std::vector<std::shared_ptr<symbol::LabelSymbol>> processLabelNodesOnOrder(
            const std::vector<std::shared_ptr<LabelNode>>& labels);

        // 符号栈操作
        void pushNewProcessingSymbol(const std::shared_ptr<symbol::Symbol>& symbol); // 压入新符号
        std::shared_ptr<symbol::Symbol> topProcessingSymbol(); // 获取栈顶符号
        void popProcessingSymbol(); // 弹出栈顶符号
        std::shared_ptr<symbol::Symbol> rPopProcessingSymbol(); // 弹出并返回栈顶符号
        symbol::SymbolType topProcessingSymbolType(); // 获取栈顶符号类型
        std::string topProcessingSymbolRaVal(); // 获取栈顶符号RA值
        std::string topProcessingSymbolVal(); // 获取栈顶符号值
        [[nodiscard]] bool isProcessingSymbol() const; // 判断栈内是否有符号

        // 操作数栈操作
        void pushOpItem(const std::shared_ptr<OpItem>& opItem); // 压入操作数（智能指针）
        void pushOpItem(
            // 压入操作数（参数构造）
            const OpItemType& type,
            const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabelSymbol,
            const std::string& value,
            const std::string& racode = "",
            const std::shared_ptr<symbol::Symbol>& referencedSymbol = nullptr,
            const std::shared_ptr<symbol::TypeLabelSymbol>& valueTypeSymbol = nullptr,
            const Pos& pos = getUnknownPos());
        void pushOpItem( // 压入操作数（名称构造）
            const std::string& name,
            const std::string& fileField,
            const std::string& scopeField,
            const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabelSymbol);
        void pushIdentItem(
            // 压入标识符操作数
            const VarID& varID,
            const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabelSymbol,
            const std::shared_ptr<symbol::TypeLabelSymbol>& valueType = nullptr,
            const std::shared_ptr<symbol::Symbol>& referencedSymbol = nullptr,
            const Pos& pos = getUnknownPos());
        OpItem rPopOpItem(); // 弹出并返回操作数
        void popOpItem(); // 弹出操作数
        [[nodiscard]] OpItem topOpItem() const; // 获取栈顶操作数
        [[nodiscard]] std::shared_ptr<OpItem> topOpItemPtr() const; // 获取栈顶操作数（智能指针）
        [[nodiscard]] std::string topOpRaVal() const; // 获取栈顶操作数RA值
        [[nodiscard]] std::string rPopOpItemRaVal(); // 弹出并返回操作数RA值
        [[nodiscard]] std::string rPopOpItemVal(); // 弹出并返回操作数原始值
        [[nodiscard]] bool hasNextOpItem() const; // 检查是否有下一个操作数

        // 临时变量/集合生成
        [[nodiscard]] static std::string getNewTempVarName(); // 获取新临时变量名
        [[nodiscard]] static std::string getNewSetLabelName(); // 获取新集合标签名
        OpItem pushTemOpVarItemWithRecord(
            const Pos& pos,
            const std::shared_ptr<symbol::TypeLabelSymbol>& valueType = nullptr,
            const std::shared_ptr<symbol::Symbol>& referencedSymbol = nullptr,
            const bool& sysDefined = {},
            const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabel = nullptr); // 生成并压入临时变量操作数
        OpItem pushTemOpSetItem(const Pos& pos); // 生成并压入临时集合操作数
        [[nodiscard]] OpItem newTemOpSetItem(const Pos& pos) const; // 生成临时集合操作数（不压栈）
        [[nodiscard]] VarID getThisFieldVarID(const Pos& pos) const;
        [[nodiscard]] std::shared_ptr<symbol::VariableSymbol> getThisFieldSymbol(
            const std::shared_ptr<symbol::ClassSymbol>& classSymbol) const;

        // 词法分析器管理方法
        static void pushLexer(const std::shared_ptr<lexer::Lexer>& lexer);
        static void popLexer();
        static std::shared_ptr<lexer::Lexer> topLexer();
        static std::string topLexerPath();
        static std::list<std::string> getLexerFilePaths();
        static bool checkIsRecursiveImportByLexerPath(const std::string& extPath);

        // 工具方法
        [[nodiscard]] std::shared_ptr<symbol::Symbol> getSymbolFromOpItem(
            const OpItem& opItem) const; // 从操作数获取符号
        [[nodiscard]] std::string raVal(const OpItem& opItem) const; // 获取操作数 RA 值
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getBuiltinTypeSymbol(
            const Pos& pos, const BuiltinType& type) const; // 获取内置类型符号
        [[nodiscard]] std::pair<std::shared_ptr<symbol::TypeLabelSymbol>, std::shared_ptr<symbol::TypeLabelSymbol>>
        getTypesFromOpItem(const OpItem& opItem) const;
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol>
        getDefiniteTypeLabelSymbolFromOpItem(const OpItem& opItem) const;
        static std::string formatAttrField(const std::string& field); // 格式化属性字段
        void annotatePos(const Pos& pos);
        static std::shared_ptr<symbol::Symbol> getReferenceTargetSymbol(const OpItem& opItem);
        static std::shared_ptr<symbol::Symbol> getReferenceTargetSymbol(
            const std::shared_ptr<symbol::VariableSymbol>& varSymbol);
        static void registerExtension(const std::string& extensionPath,
                                      const std::shared_ptr<symbol::ClassSymbol>& extensionClass);
        static std::shared_ptr<symbol::ClassSymbol> getRegisteredExtension(const std::string& extensionPath);
        static bool isExtensionRegistered(const std::string& extensionPath);
        static void setSymbolBuiltinType(const std::shared_ptr<symbol::Symbol>& processingSymbol,
                                         const symbol::TypeOfBuiltin& type);
        [[nodiscard]] bool checkSymbolExists(const OpItem& opItem) const;
        [[nodiscard]] bool checkSymbolExists(const std::shared_ptr<OpItem>& opItem) const;
        [[nodiscard]] bool checkSymbolExists(const std::shared_ptr<symbol::Symbol>& processingSymbol) const;
        void checkExists(const OpItem& opItem, const std::optional<Pos>& pos = std::nullopt) const;
        void checkExists(const std::shared_ptr<OpItem>& opItem, const Pos& pos) const;
        void checkExists(const std::shared_ptr<symbol::Symbol>& processingSymbol) const;
        static std::string getListFormatString(const std::vector<std::string>& list);
        static std::string getCodeLine(const Pos& pos);
        static std::shared_ptr<symbol::TypeLabelSymbol> getTypeLabelFromSymbol(
            const std::shared_ptr<symbol::Symbol>& symbol);
        static void recordProcessingExtension(const std::string &extensionPath, const std::string& extensionName);
        static void popProcessingExtension();
        static bool checkIsProcessedExtension(const std::string &extensionPath);
        static std::string getExtensionName(const std::string &extensionPath);
        static std::string topProcessingExtensionPath();
        void processSymbolFlagOperation();

        // 将原 visitFunctionCallNode 的逻辑迁移到这里（主调度器）
        void processFunctionCallNode(const FunctionCallNode& node);

        // 将 orderedArgs 转为 originalArgs / halfProcessedArgs
        void prepareOrderedArgs(const std::vector<std::pair<std::string, OpItem>>& orderedArgs,
                                std::vector<std::pair<std::string, std::string>>& originalArgs,
                                std::vector<std::string>& halfProcessedArgs) const;

        // 解析/寻找被调用函数（可能是构造函数、引用到函数、名称查找等），
        // 可能会修改 customTypeVid 和 fullProcessedArgs（当属于某个对象时）
        std::shared_ptr<symbol::FunctionSymbol> resolveFunctionSymbol(const OpItem& funcNameOpItem,
                                                                      std::string& customTypeVid,
                                                                      std::vector<std::string>& fullProcessedArgs,
                                                                      const std::queue<OpItem>& posArgs,
                                                                      const std::unordered_map<std::string, OpItem>&
                                                                      namedArgs,
                                                                      const std::vector<std::pair<std::string, OpItem>>&
                                                                      orderedArgs,
                                                                      const FunctionCallNode& node) const;
        static std::vector<std::shared_ptr<symbol::LabelSymbol>> extractParamTableLabelDes(
            const std::vector<std::shared_ptr<symbol::ParameterSymbol>>& params);
        void handleFuncTypeLabel(const std::shared_ptr<symbol::VariableSymbol>& varSymbol,
                                 const std::shared_ptr<symbol::LabelSymbol>& typeLabel,
                                 std::shared_ptr<symbol::FunctionSymbol>& funcSymbol) const;
        void handleFuniTypeLabel(const std::shared_ptr<symbol::VariableSymbol>& varSymbol,
                                 const std::shared_ptr<symbol::LabelSymbol>& typeLabel,
                                 std::shared_ptr<symbol::FunctionSymbol>& funcSymbol) const;
        [[nodiscard]] std::vector<std::shared_ptr<symbol::ParameterSymbol>> createParameters(
            const std::vector<std::shared_ptr<symbol::LabelSymbol>>& desList,
            const std::shared_ptr<symbol::VariableSymbol>& varSymbol) const;
        static symbol::ParamType getParamTypeFromLabel(const std::shared_ptr<symbol::TypeLabelSymbol>& typeLabel);
        static void handleFuniReturnType(const std::vector<std::shared_ptr<symbol::LabelSymbol>>& secondDes,
                                  std::shared_ptr<symbol::TypeLabelSymbol>& returnType) ;
        static void throwRCCLabelDesError(const std::shared_ptr<symbol::VariableSymbol>& varSymbol,
                                          const std::shared_ptr<symbol::LabelSymbol>& typeLabel, size_t currentCount,
                                          const std::string& expected);
        static void throwRCCLabelDesError(const std::shared_ptr<symbol::VariableSymbol>& varSymbol,
                                          const std::shared_ptr<symbol::LabelSymbol>& typeLabel, size_t currentCount,
                                          size_t expected);
        static void throwRCCTypeMissmatchError(const std::shared_ptr<symbol::LabelSymbol>& des,
                                        const std::shared_ptr<symbol::LabelSymbol>& typeLabel,
                                        symbol::LabelType expectedType) ;

        // 当找不到 funcSymbol 时的处理（会在内部生成 CALL/IVOK 等并返回 true 表示已处理）
        void handleNoFuncSymbolCase(const OpItem& funcNameOpItem,
                                    const std::vector<std::string>& halfProcessedArgs,
                                    const FunctionCallNode& node);

        // 按照 funcSymbol 的参数列表处理传入的 posArgs/namedArgs，填充 fullProcessedArgs
        void processParameters(const std::shared_ptr<symbol::FunctionSymbol>& funcSymbol,
                               std::queue<OpItem>& posArgs,
                               std::unordered_map<std::string, OpItem>& namedArgs,
                               const std::vector<std::pair<std::string, OpItem>>& orderedArgs,
                               std::vector<std::string>& fullProcessedArgs,
                               const Pos& callPos);

        // 最终根据 funcSymbol 类型生成调用相关的 RA 指令（builtin 与普通函数分支）
        void emitFinalCall(const std::shared_ptr<symbol::FunctionSymbol>& funcSymbol,
                           const builtin::CallInfos& callInfos);

        static void processLabelDesNode(const std::shared_ptr<ExpressionNode>& labelNode, std::vector<std::shared_ptr<LabelNode>>& desLabelNodes);

        bool isVisitingProgramEntry() const;

        bool compile();

        std::string getCompileResult();

        // ===================== Node visitor ==========================

        void visitProgramNode(ProgramNode& node) override;

        void visitBlockRangerNode(BlockRangerNode& node) override;

        void processFunctionParams(const std::vector<std::shared_ptr<ExpressionNode>>& paramItems,
                                   std::vector<std::shared_ptr<symbol::ParameterSymbol>>& paramSymbols,
                                   std::vector<std::string>& paramIdents,
                                   std::vector<std::shared_ptr<symbol::LabelSymbol>>& paramLabelDes);

        void visitBracketExpressionNode(BracketExpressionNode& node) override;

        void visitLiteralNode(LiteralNode& node) override;

        void visitNullLiteralNode(NullLiteralNode& node) override;

        void visitStringLiteralNode(StringLiteralNode& node) override;

        void visitNumberLiteralNode(NumberLiteralNode& node) override;

        void visitIntegerLiteralNode(IntegerLiteralNode& node) override;

        void visitFloatLiteralNode(FloatLiteralNode& node) override;

        void visitBooleanLiteralNode(BooleanLiteralNode& node) override;

        void visitCharacterLiteralNode(CharacterLiteralNode& node) override;

        void visitIdentifierNode(IdentifierNode& node) override;

        void visitParameterNode(ParameterNode& node) override;

        void visitArgumentNode(ArgumentNode& node) override;

        void visitFunctionDeclarationNode(FunctionDeclarationNode& node) override;

        void visitConstructorDefinitionNode(ConstructorDefinitionNode& node) override;

        void visitClassDeclarationNode(ClassDeclarationNode& node) override;

        std::string compileConstructorNode(const std::shared_ptr<symbol::FunctionSymbol>& ctorSymbol);

        void compileClassMembers(const std::string& classIdent, const std::shared_ptr<symbol::SymbolTable>& members,
                                 const symbol::LifeCycleLabel&
                                 memberType);

        void visitClassDefinitionNode(ClassDefinitionNode& node) override;

        void classifyFuncArgs(
            const FunctionCallNode& node, std::queue<OpItem>& posArgs,
            std::unordered_map<std::string, OpItem>& namedArgs,
            std::vector<std::pair<std::string, OpItem>>& orderedArgs);

        [[nodiscard]] std::shared_ptr<symbol::FunctionSymbol> getCtorSymbol(
            const std::shared_ptr<symbol::ClassSymbol>& classSymbol,
            const std::queue<OpItem>& posArgs,
            const std::unordered_map<std::string, OpItem>& namedArgs, const Pos& ctorCallPos,
            const std::vector<std::pair<std::
                                        string, OpItem>>& orderedArgs) const;

        void visitFunctionCallNode(FunctionCallNode& node) override;

        void visitInfixNode(InfixExpressionNode& node) override;

        void visitUnaryExpressionNode(UnaryExpressionNode& node) override;

        void visitExpressionStatementNode(ExpressionStatementNode& node) override;

        void visitPrefixExpressionNode(PrefixExpressionNode& node) override;

        void visitPostfixNode(PostfixExpressionNode& node) override;

        void visitParenRangerNode(ParenRangerNode& node) override;

        void visitFunctionDefinitionNode(FunctionDefinitionNode& node) override;

        void visitLabelNode(LabelNode& node) override;

        void visitBranchNode(BranchNode& node) override;

        void visitConditionNode(ConditionNode& node) override;

        void visitLoopNode(LoopNode& node) override;

        void visitForLoopNode(ForLoopNode& node) override;

        void visitPassExpressionNode(PassExpressionNode& node) override;

        void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode& node) override;

        void visitReturnExpressionNode(ReturnExpressionNode& node) override;

        void visitBreakExpressionNode(BreakExpressionNode& node) override;

        void visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode& node) override;

        void visitPairExpressionNode(PairExpressionNode& node) override;

        void visitDictionaryExpressionNode(DictionaryExpressionNode& node) override;

        void visitListExpressionNode(ListExpressionNode& node) override;

        void visitIndexExpressionNode(IndexExpressionNode& node) override;

        void visitVariableDefinitionNode(VariableDefinitionNode& node) override;

        void visitAssignmentNode(AssignmentNode& node) override;

        symbol::IRCCSymbolTableManagerInterface* GetSymbolTableManagerI() override;
        void WriteRaCode(const char* raCode) override;
        const char* GetProgramEntryFilePath() const override;
        const char* GetProgramTargetFilePath() const override;
        const char* GetCompileOutputFilePath() const override;
        const char* GetCurrentProcessingFilePath() const override;
        void SetCurrentProcessingFilePath(const char* filepath) override;
        const char* CurScopeField() const override;
        ScopeType CurScopeType() const override;
        void EnterScope(const ast::ScopeType& scopeType) override;
        void EnterScope(const std::size_t& level) override;
        void EnterTopScope() override;
        void EnterGlobalScope() override;
        void ExitScope(const ast::ScopeType& scopeType) override;
        std::size_t CurScopeLevel() const override;
        void EnterLoopScope() override;
        void ExitLoopScope() override;
        bool IsInLoopScope() const override;
        bool CheckTypeMatch(const symbol::IRCCSymbolInterface* leftSymbolI,
            const IRCCOpItemInterface* rightOpItemI) const override;
        void ProcessingTypeAutoChange(const symbol::IRCCSymbolInterface* sourceSymbolI,
            const IRCCOpItemInterface* targetOpItemI) const override;
        void PushNewProcessingSymbol(const symbol::IRCCSymbolInterface* symbolI) override;
        symbol::IRCCSymbolInterface* TopProcessingSymbolI() override;
        void PopProcessingSymbolI() override;
        symbol::IRCCSymbolInterface* RPopProcessingSymbolI() override;
        symbol::SymbolType TopProcessingSymbolType() override;
        const char* TopProcessingSymbolVal() override;
        const char* TopProcessingSymbolRaVal() override;
        bool IsProcessingSymbol() const override;
        void PushOpItem(const IRCCOpItemInterface* opItemI) override;
        void PushOpItem(const OpItemType& opItemType, const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI,
            const char* value, const char* raValue, const symbol::IRCCSymbolInterface* referencedSymbolI,
            const symbol::IRCCTypeLabelSymbolInterface* valueTypeSymbolI) override;
        void PushOpItem(const char* name, const char* fileField, const char* scopeField,
            const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI) override;
        IRCCOpItemInterface* RPopOpItemI() override;
        void PopOpItem() override;
        IRCCOpItemInterface* TopOpItemI() const override;
        const char* TopOpRaVal() const override;
        const char* RPopOpItemRaVal() override;
        const char* RPopOpItemVal() override;
        bool HasNextOpItem() const override;
        void PushTemOpVarItemWithRecord(const IRCCPosInterface* posI,
            const symbol::IRCCTypeLabelSymbolInterface* valueTypeSymbolI,
            const symbol::IRCCSymbolInterface* referencedSymbolI, const bool& sysDefined,
            const symbol::IRCCTypeLabelSymbolInterface* typeLabelSymbolI) override;
        IRCCOpItemInterface* PushTemOpSetItem(const IRCCPosInterface* posI) override;
        IRCCOpItemInterface* NewTemOpSetItem(const IRCCPosInterface* posI) const override;
        const char* GetThisFieldRaVal(const IRCCPosInterface* posI) const override;
        symbol::IRCCVariableSymbolInterface* GetThisFieldSymbol(
            const symbol::IRCCClassSymbolInterface* classSymbolI) const override;
        const char* GetNewRaValCorrespondingToNewVal(const char* val) override;
        const char* RaVal(const IRCCOpItemInterface* opItemI) const override;
        symbol::IRCCTypeLabelSymbolInterface*
        GetTypeLabelSymbolIByStr(const char* name, std::size_t scopeLevel) override;
        void FreeTypeLabelSymbolI(symbol::IRCCTypeLabelSymbolInterface*& symbol) override;
        const char* GetNewTempVarName() override;
        void FreeCharP(const char* data) override;
        const char* GetNewSetLabelName() override;
        bool CheckTypeMatch(const symbol::IRCCTypeLabelSymbolInterface* leftTypeSymbolI,
            const symbol::IRCCTypeLabelSymbolInterface* rightTypeSymbolI, const bool& restrict) const override;
        utils::IRCCPosInterface* CurrentPos() const override;
        void SetCurrentPos(const utils::IRCCPosInterface* pos) override;
        void ResetCurrentPos() override;
        void FreeOpItemI(IRCCOpItemInterface*& opItemI) override;
        IRCCPosInterface* GetUnknownPosI() const override;
        static const char* CreateCharP(const std::string& data);
    };
}

#endif //RCC_COMPILER_VISITOR_H
