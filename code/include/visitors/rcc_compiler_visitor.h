//
// Created by RestRegular on 2025/7/20.
//

#ifndef RCC_COMPILER_VISITOR_H
#define RCC_COMPILER_VISITOR_H

#include <queue>

#include "rcc_base_visitor.h"
#include "../components/ri/rcc_ri.h"

namespace ast {

#define ANONYMOUS_FUNCTION_PREFIX "anon_func_"

    class RaCodeBuilder
    {
        std::stack<std::stringstream> raCodeStack{};
        std::string raCode;
    public:
        RaCodeBuilder();
        ~RaCodeBuilder() = default;
        std::string buildAll();
        RaCodeBuilder &appendCode(const std::string &code);
        RaCodeBuilder &operator<<(const std::string &code);
        RaCodeBuilder &operator<<(const ri::RI &ri);
        void enterScope();
        void exitScope();
        RaCodeBuilder &buildCurScope(std::string &target);
        RaCodeBuilder &buildCurScope(RaCodeBuilder &builder);
        RaCodeBuilder &operator>>(std::string &target);
        RaCodeBuilder &operator>>(RaCodeBuilder &builder);
    };

    class CompileVisitor final : public Visitor
    {
    public:
        // ========================== Ƕ��ö������ ==========================
        // ����������ö��
        enum class ScopeType : int {
            TEMPORARY = 0,        // ��ʱ������
            GLOBAL,               // ȫ��������
            PROGRAM,              // ����������
            EXPRESSION,           // ���ʽ������
            VAR_DEF_EXPRESSION,   // ����������ʽ������
            CLASS,                // ��������
            FUNCTION,             // ����������
            ANONYMOUS,            // ����������
            LOOP,                 // ѭ��������
            CONDITION             // ����������
        };

        // ����������ö��
        enum class OpItemType : int {
            IDENTIFIER,       // ��ʶ��
            LITERAL_VALUE,    // ������ֵ
            SET_LABEL         // ���ϱ�ǩ
        };

        // ��������ö��
        enum BuiltinType : int {
            B_ANY = 0,    // ��������
            B_BOOL,       // ��������
            B_CHAR,       // �ַ�����
            B_DICT,       // �ֵ�����
            B_FLOAT,      // ��������
            B_INT,        // ��������
            B_LIST,       // �б�����
            B_STR,        // �ַ�������
            B_VOID,       // ������
            B_NUL,        // ��ֵ����
            B_FLAG,       // ��־����
            B_FUNC,       // �շ��غ�������
            B_FUNI,       // ʵ���غ�������
            B_CLAS,       // �Զ���������
        };

        // ========================== Ƕ���� ==========================
        // ����ID��װ��
        class VarID final : public Object {
            static size_t _varId;                  // ����ID������
            static std::string rccVarPrefixField;  // ����ǰ׺
            size_t id = 0;                         // ��ǰ����ID
            std::string fileField, scopeField, nameField;  // �ļ�������������
            std::string vid;                       // ����Ψһ��ʶ
            [[nodiscard]] std::string _toVarID();  // ���ɱ���ID�ַ���

        public:
            // ���캯��
            VarID(const std::string &name,
                  const std::string &fileField,
                  const std::string &scopeField);

            // �ַ���ת��
            [[nodiscard]] std::string toString() const override;

            // Getter����
            [[nodiscard]] std::string getNameField() const;
            [[nodiscard]] std::string getScopeField() const;
            [[nodiscard]] std::string getFileField() const;
            [[nodiscard]] size_t getId() const;
            [[nodiscard]] std::string getVid() const;

            // ������������
            friend std::ostream &operator<<(std::ostream &os, const VarID &varID) {
                os << varID.vid;
                return os;
            }
        };

        // ����ID��װ��
        class SetID final : public Object {
            static size_t _setId;                  // ����ID������
            static std::string rccSetPrefixField;  // ����ǰ׺
            size_t id = 0;                         // ��ǰ����ID
            std::string fileField, scopeField, nameField;  // �ļ�������������
            std::string sid;                       // ����Ψһ��ʶ
            [[nodiscard]] std::string _toSetID();  // ���ɼ���ID�ַ���

        public:
            // ���캯��
            SetID(const std::string &name,
                  const std::string &fileField,
                  const std::string &scopeField);

            // �ַ���ת��
            [[nodiscard]] std::string toString() const override;

            // Getter����
            [[nodiscard]] std::string getNameField() const;
            [[nodiscard]] std::string getScopeField() const;
            [[nodiscard]] std::string getFileField() const;
            [[nodiscard]] size_t getId() const;
            [[nodiscard]] std::string getSid() const;

            // ������������
            friend std::ostream &operator<<(std::ostream &os, const SetID &setID) {
                os << setID.sid;
                return os;
            }
        };

        // ����������
        class OpItem final : public Object {
            std::string value;                          // ԭʼֵ
            std::string raValue;                        // RA����ֵ
            OpItemType type;                            // ����������
            std::shared_ptr<symbol::TypeLabelSymbol> typeLabel;  // ���ͱ�ǩ
            std::shared_ptr<symbol::TypeLabelSymbol> valueType;  // ֵ����
            std::shared_ptr<symbol::Symbol> belonging;      // ��������
            std::string belongAttrRaValue;                  // ��������
            std::shared_ptr<symbol::Symbol> referencedSymbol; // ���÷���

        public:
            // ���캯��
            explicit OpItem(
                const OpItemType &type,
                const std::shared_ptr<symbol::TypeLabelSymbol> &typeLabelSymbol,
                const std::string &value,
                const std::string &raValue = "",
                const std::shared_ptr<symbol::TypeLabelSymbol> &valueType = nullptr,
                const std::shared_ptr<symbol::Symbol> &referencedSymbol = nullptr);

            bool is(const OpItemType &opItemType) const;
            bool isNot(const OpItemType &opItemType) const;

            // Getter����
            [[nodiscard]] std::string getVal() const;
            [[nodiscard]] OpItemType getType() const;
            [[nodiscard]] std::string getRaVal(const symbol::SymbolTableManager &table) const;
            [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getTypeLabel() const;
            [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getValueType() const;
            [[nodiscard]] std::shared_ptr<symbol::Symbol> getBelonging() const;
            [[nodiscard]] std::string getBelongAttrRaValue() const;
            [[nodiscard]] std::shared_ptr<symbol::Symbol> getReferencedSymbol() const;

            // Setter����
            void setTypeLabel(const std::shared_ptr<symbol::TypeLabelSymbol> &typeLabelSymbol);
            void setValueType(const std::shared_ptr<symbol::TypeLabelSymbol> &valueTypeSymbol);
            void setBelonging(const std::shared_ptr<symbol::Symbol> &belongingSymbol, const std::string &belongAttrRaVal);
            void setReferencedSymbol(const std::shared_ptr<symbol::Symbol> &symbol);
        };

    private:
        // ========================== ��̬��Ա���� ==========================
        static size_t _temVarId;       // ��ʱ����ID������
        static size_t _setId;          // ���ϱ�ǩID������
        static std::unordered_map<std::string,
            std::shared_ptr<symbol::ClassSymbol>> extensionMap;  // ��չ��ӳ��

        // ========================== ��Ա���� ==========================
        RaCodeBuilder raCodeBuilder{};                          // RA���빹����
        symbol::SymbolTableManager symbolTable;                 // ���ű������

        std::string programEntryFilePath;                       // ��������ļ�·��
        std::string programTagetFilePath;                       // ����Ŀ���ļ�·��
        std::string compileOutputFilePath;                      // ��������ļ�·��
        std::string currentProcessingFilePath;                  // ��ǰ���ڴ�����ļ�·��
        bool needSaveOutputToFile;                              // �Ƿ񱣴��������ļ�

        // ����ջ
        std::stack<std::shared_ptr<symbol::Symbol>> processingSymbolStack{};  // ���Ŵ���ջ
        std::stack<std::shared_ptr<OpItem>> opItemStack{};                    // ������ջ
        std::stack<ScopeType> scopeTypeStack{};                               // ����������ջ

    public:
        // ======================= constructor ========================
        explicit CompileVisitor(
            const std::string &programEntryFilePath,
            const std::string &programTargetFilePath,
            const std::string &compileOutputFilePath="",
            const bool &needSaveOutput = true);

        // ====================== getter & setter =====================
        [[nodiscard]] symbol::SymbolTableManager &getSymbolTable();
        [[nodiscard]] std::stack<std::shared_ptr<symbol::Symbol>> &getProcessingSymbolStack();
        [[nodiscard]] std::stack<std::shared_ptr<OpItem>> &getOpItemStack();
        [[nodiscard]] std::stack<ScopeType> &getScopeTypeStack();
        [[nodiscard]] std::string getProgramEntryFilePath() const;
        [[nodiscard]] std::string getProgramTargetFilePath() const;
        [[nodiscard]] std::string getCompileOutputFilePath() const;
        [[nodiscard]] std::string getCurrentProcessingFilePath() const;
        void setCurrentProcessingFilePath(const std::string &filePath = "");

        // ========================== �������� ==========================
        // ���������
        static std::string scopeTypeToString(ScopeType scopeType);  // ����������ת�ַ���
        [[nodiscard]] std::string curScopeField();                  // ��ȡ��ǰ�������ֶ�
        void enterScope(ScopeType scopeType);                       // ����������
        void enterScope(size_t scopeLevel);                         // ����ָ���㼶������
        void enterTopScope();                                       // ���붥��������
        void enterGlobalScope();                                    // ����ȫ��������
        void exitScope(ScopeType scopeType);                        // �˳�������
        size_t curScopeLevel() const;                               // ��ȡ��ǰ������㼶

        // ���ͼ����ת��
        [[nodiscard]] static bool checkTypeMatch(
            const std::shared_ptr<symbol::TypeLabelSymbol> &leftTypeSymbol,
            const std::shared_ptr<symbol::TypeLabelSymbol> &rightTypeSymbol);  // �������ƥ��
        [[nodiscard]] static bool checkTypeMatch(
            const std::shared_ptr<symbol::Symbol> &leftSymbol,
            const std::shared_ptr<symbol::Symbol> &rightSymbol);              // ����������ƥ��
        [[nodiscard]] bool checkTypeMatch(
            const std::shared_ptr<symbol::Symbol> &leftSymbol,
            const OpItem& rightOpItem) const;                                 // �����������������ƥ��
        void processTypeAutoChange(
            const std::shared_ptr<symbol::Symbol> &sourceSymbol,
            const OpItem &targetOpItem) const;                                      // ���������Զ�ת��
        void processTypeAutoChange(
            const std::shared_ptr<symbol::Symbol> &sourceSymbol,
            const std::shared_ptr<OpItem> &targetOpItem) const;                     // ���������Զ�ת��������ָ��棩

        // �ڵ㴦��
        static std::vector<std::shared_ptr<ExpressionNode>> visitParallelNode(
            const std::shared_ptr<ExpressionNode> &node);                     // ���ʲ��нڵ�
        std::unordered_set<std::shared_ptr<symbol::LabelSymbol>> processLabelNodes(
            const std::vector<std::shared_ptr<LabelNode>>& labels);           // �����ǩ�ڵ�

        // ����ջ����
        void pushNewProcessingSymbol(const std::shared_ptr<symbol::Symbol> &symbol);  // ѹ���·���
        std::shared_ptr<symbol::Symbol> topProcessingSymbol();                        // ��ȡջ������
        void popProcessingSymbol();                                                   // ����ջ������
        std::shared_ptr<symbol::Symbol> rPopProcessingSymbol();                       // ����������ջ������
        symbol::SymbolType topProcessingSymbolType();                                 // ��ȡջ����������
        std::string topProcessingSymbolRaVal();                                       // ��ȡջ������RAֵ
        std::string topProcessingSymbolVal();                                         // ��ȡջ������ֵ
        bool isProcessingSymbol() const;                                              // �ж�ջ���Ƿ��з���

        // ������ջ����
        void pushOpItem(const std::shared_ptr<OpItem> &opItem);                   // ѹ�������������ָ�룩
        void pushOpItem(
            // ѹ����������������죩
            const OpItemType &type,
            const std::shared_ptr<symbol::TypeLabelSymbol> &typeLabelSymbol,
            const std::string &value,
            const std::string &racode = "",
            const std::shared_ptr<symbol::Symbol>& referencedSymbol = nullptr,
            const std::shared_ptr<symbol::TypeLabelSymbol>& valueTypeSymbol = nullptr);
        void pushOpItem(                                                          // ѹ������������ƹ��죩
            const std::string &name,
            const std::string &fileField,
            const std::string &scopeField,
            const std::shared_ptr<symbol::TypeLabelSymbol> &typeLabelSymbol);
        void pushIdentItem(                                                       // ѹ���ʶ��������
            const VarID &varID,
            const std::shared_ptr<symbol::TypeLabelSymbol> &typeLabelSymbol,
            const std::shared_ptr<symbol::TypeLabelSymbol> &valueType=nullptr,
            const std::shared_ptr<symbol::Symbol> &referencedSymbol=nullptr);
        OpItem rPopOpItem();                                                      // ���������ز�����
        void popOpItem();                                                         // ����������
        [[nodiscard]] OpItem topOpItem() const;                                   // ��ȡջ��������
        [[nodiscard]] std::shared_ptr<OpItem> topOpItemPtr() const;               // ��ȡջ��������������ָ�룩
        [[nodiscard]] std::string topOpRaVal() const;                             // ��ȡջ��������RAֵ
        [[nodiscard]] std::string rPopOpItemRaVal();                              // ���������ز�����RAֵ
        [[nodiscard]] std::string rPopOpItemVal();                                // ���������ز�����ԭʼֵ
        [[nodiscard]] bool hasNextOpItem() const;                                 // ����Ƿ�����һ��������

        // ��ʱ����/��������
        [[nodiscard]] static std::string getNewTempVarName();                     // ��ȡ����ʱ������
        [[nodiscard]] static std::string getNewSetLabelName();                    // ��ȡ�¼��ϱ�ǩ��
        OpItem pushTemOpVarItemWithRecord(
            const Pos &pos,
            const std::shared_ptr<symbol::TypeLabelSymbol>& valueType = nullptr,
            const std::shared_ptr<symbol::Symbol> &referencedSymbol=nullptr); // ���ɲ�ѹ����ʱ����������
        OpItem pushTemOpSetItem(const Pos &pos);                                  // ���ɲ�ѹ����ʱ���ϲ�����
        OpItem newTemOpSetItem(const Pos &pos);                                   // ������ʱ���ϲ���������ѹջ��
        VarID getThisFieldVarID(const Pos& pos);
        std::shared_ptr<symbol::VariableSymbol> getThisFieldSymbol(const std::shared_ptr<symbol::ClassSymbol>& classSymbol);

        // ���߷���
        [[nodiscard]] std::shared_ptr<symbol::Symbol> getSymbolFromOpItem(
            const OpItem &opItem) const;                                       // �Ӳ�������ȡ����
        [[nodiscard]] std::string raVal(const OpItem &opItem) const;           // ��ȡ������ RA ֵ
        [[nodiscard]] std::shared_ptr<symbol::TypeLabelSymbol> getBuiltinTypeSymbol(
            const Pos &pos, const BuiltinType& type) const;                    // ��ȡ�������ͷ���
        [[nodiscard]] std::pair<std::shared_ptr<symbol::TypeLabelSymbol>, std::shared_ptr<symbol::TypeLabelSymbol>>
        getTypesFromOpItem(const OpItem &opItem) const;
        static std::string formatAttrField(const std::string &field);          // ��ʽ�������ֶ�
        void annotatePos(const Pos& pos);
        static std::shared_ptr<symbol::Symbol> getReferenceTargetSymbol(const OpItem &opItem);
        static std::shared_ptr<symbol::Symbol> getReferenceTargetSymbol(const std::shared_ptr<symbol::VariableSymbol> &varSymbol);
        static void registerExtension(const std::string &extensionPath, const std::shared_ptr<symbol::ClassSymbol> &extensionClass);
        static std::shared_ptr<symbol::ClassSymbol> getRegisteredExtension(const std::string &extensionPath);
        static bool isExtensionRegistered(const std::string &extensionPath);
        static void setSymbolBuiltinType(const std::shared_ptr<symbol::Symbol>& processingSymbol, const symbol::TypeOfBuiltin &type);
        bool checkSymbolExists(const OpItem &opItem) const;
        bool checkSymbolExists(const std::shared_ptr<OpItem> &opItem) const;
        bool checkSymbolExists(const std::shared_ptr<symbol::Symbol>& processingSymbol) const;
        void checkExists(const OpItem &opItem) const;
        void checkExists(const std::shared_ptr<OpItem> &opItem) const;
        void checkExists(const std::shared_ptr<symbol::Symbol>& processingSymbol) const;

        bool compile();

        std::string getCompileResult();

        void visitLiteralNode(LiteralNode &node) override;

        void visitNullLiteralNode(NullLiteralNode& node) override;

        void visitStringLiteralNode(StringLiteralNode &node) override;

        void visitNumberLiteralNode(NumberLiteralNode &node) override;

        void visitIntegerLiteralNode(IntegerLiteralNode &node) override;

        void visitFloatLiteralNode(FloatLiteralNode &node) override;

        void visitBooleanLiteralNode(BooleanLiteralNode &node) override;

        void visitCharacterLiteralNode(CharacterLiteralNode &node) override;

        void visitIdentifierNode(IdentifierNode &node) override;

        void visitParameterNode(ParameterNode &node) override;

        void visitArgumentNode(ArgumentNode &node) override;

        void visitFunctionDeclarationNode(FunctionDeclarationNode &node) override;

        void visitConstructorDefinitionNode(ConstructorDefinitionNode &node) override;

        void visitClassDeclarationNode(ClassDeclarationNode &node) override;

        std::string compileConstructorNode(const std::shared_ptr<symbol::FunctionSymbol> &ctorSymbol);

        void visitClassDefinitionNode(ClassDefinitionNode &node) override;

        void classifyFuncArgs(
            const FunctionCallNode& node, std::queue<OpItem>& posArgs,
            std::unordered_map<std::string, OpItem>& namedArgs, std::vector<OpItem>& orderedArgs);

        std::shared_ptr<symbol::FunctionSymbol> getCtorSymbol(
            const std::shared_ptr<symbol::ClassSymbol> &classSymbol,
            const std::queue<OpItem> &posArgs,
            const std::unordered_map<std::string, OpItem> &namedArgs) const;

        void visitFunctionCallNode(FunctionCallNode &node) override;

        void visitProgramNode(ProgramNode &node) override;

        void visitInfixNode(InfixExpressionNode &node) override;

        void visitUnaryExpressionNode(UnaryExpressionNode &node) override;

        void visitExpressionStatementNode(ExpressionStatementNode &node) override;

        void visitPrefixExpressionNode(PrefixExpressionNode &node) override;

        void visitPostfixNode(PostfixExpressionNode &node) override;

        void visitParenRangerNode(ParenRangerNode &node) override;

        void visitBlockRangerNode(BlockRangerNode &node) override;

        void visitFunctionDefinitionNode(FunctionDefinitionNode &node) override;

        void visitLabelNode(LabelNode &node) override;

        void visitBranchNode(BranchNode &node) override;

        void visitConditionNode(ConditionNode &node) override;

        void visitLoopNode(LoopNode &node) override;

        void visitForLoopNode(ForLoopNode &node) override;

        void visitPassExpressionNode(PassExpressionNode &node) override;

        void visitEncapsulatedExpressionNode(EncapsulatedExpressionNode &node) override;

        void visitReturnExpressionNode(ReturnExpressionNode &node) override;

        void visitBreakExpressionNode(BreakExpressionNode &node) override;

        void visitAnonFunctionDefinitionNode(AnonFunctionDefinitionNode &node) override;

        void visitPairExpressionNode(PairExpressionNode &node) override;

        void visitDictionaryExpressionNode(DictionaryExpressionNode &node) override;

        void visitListExpressionNode(ListExpressionNode &node) override;

        void visitBracketExpressionNode(BracketExpressionNode &node) override;

        void visitIndexExpressionNode(IndexExpressionNode &node) override;

        void visitVariableDefinitionNode(VariableDefinitionNode &node) override;

        void visitAssignmentNode(AssignmentNode &node) override;
    };
}

#endif //RCC_COMPILER_VISITOR_H
