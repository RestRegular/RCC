//
// Created by RestRegular on 2025/10/1.
//

// rcc_function_call_compiler.cpp
// �� visitFunctionCallNode ���߼���ְ���ֵ����˽�� helper �У�����ά���Ͳ��ԡ�
// �뽫���ļ����빹��ϵͳ��CMakeLists.txt / Makefile / VS project����

#include <queue>
#include <unordered_map>
#include <vector>
#include <string>

#include "../../include/visitors/rcc_compile_visitor.h"
#include "../../include/components/symbol/rcc_symbol.h"
#include "../../include/builtin/rcc_builtin.h"

namespace ast
{
    using namespace symbol;

    // ---------- �����Ⱥ�������ԭʼ visitFunctionCallNode ���߼���֯���ø� helper ----------
    void CompileVisitor::processFunctionCallNode(const FunctionCallNode& node)
    {
        // ��ǰ׼����λ�ò����������������������
        std::queue<OpItem> posArgs;
        std::unordered_map<std::string, OpItem> namedArgs;
        std::vector<std::pair<std::string, OpItem>> orderedArgs;

        // ������մ�����õ� ra ��ʾ������������
        std::vector<std::string> fullProcessedArgs{};
        // ���ԭʼ������Ϣ��name, raw��
        std::vector<std::pair<std::string, std::string>> originalArgs{};
        // ��š��봦��������ֻ�� raVal��
        std::vector<std::string> halfProcessedArgs{};

        // classifyFuncArgs Ϊ���еķ������������ posArgs/namedArgs/orderedArgs
        classifyFuncArgs(node, posArgs, namedArgs, orderedArgs);

        // ���� posArgs �� namedArgs�����ں������ callInfos
        const auto posArgsCopied = posArgs;
        const auto namedArgsCopied = namedArgs;

        // �� orderedArgs תΪ originalArgs / halfProcessedArgs
        prepareOrderedArgs(orderedArgs, originalArgs, halfProcessedArgs);

        // ���ҵ��õĺ�������
        std::shared_ptr<FunctionSymbol> funcSymbol = nullptr;

        // �������ñ��ʽ����ࣩ
        node.getLeftNode()->acceptVisitor(*this);
        const auto& funcNameOpItem = rPopOpItem();

        // customTypeVid ��ʼֵͨ������ funcNameOpItem �� raVal
        std::string customTypeVid = funcNameOpItem.getRaVal(symbolTable);

        // resolveFunctionSymbol �᳢�ԴӸ�����Դ������ funcSymbol������Ϊ���캯�������õ��ĺ����ȣ�
        funcSymbol = resolveFunctionSymbol(funcNameOpItem, customTypeVid, fullProcessedArgs,
                                           posArgs, namedArgs, orderedArgs, node);

        // ���δ������ funcSymbol�����԰����������������� func/funi ���͵����ͱ�ǩ�ȣ�
        if (!funcSymbol)
        {
            // ����÷�֧�ڲ�����ɹ������� CALL/IVOK������ֱ�ӷ���
            handleNoFuncSymbolCase(funcNameOpItem, halfProcessedArgs, node);
            return;
        }

        // �����funcSymbol �Ѿ������������״�
        // �Ҳ������ RANGER �ڵ㣬����ԭ������
        if (node.getRightNode()->getRealType() != NodeType::RANGER)
        {
            throw RCCCompilerError::typeMissmatchError(node.getRightNode()->getPos().toString(),
                                                       getCodeLine(node.getRightNode()->getPos()),
                                                       "The type of the right node of the " + getNodeTypeFormatName(
                                                           NodeType::CALL) + " must be the "
                                                       + getNodeTypeFormatName(NodeType::RANGER) + ".",
                                                       getNodeTypeFormatName(NodeType::RANGER),
                                                       getNodeTypeFormatName(node.getRightNode()->getRealType()),
                                                       {});
        }

        // ���ݺ��������б����� posArgs / namedArgs����� fullProcessedArgs
        processParameters(funcSymbol, posArgs, namedArgs, orderedArgs, fullProcessedArgs, node.getPos());

        // ��װ builtin::CallInfos �� builtin ����ʹ�ã���ԭ���뱣��һ�£�
        const builtin::CallInfos& callInfos{
            fullProcessedArgs, originalArgs, posArgsCopied,
            namedArgsCopied, node.getPos(), orderedArgs,
            std::make_shared<FunctionCallNode>(node)
        };

        // ���ո��� funcSymbol ������ѡ������ builtin ���û���ͨ����
        emitFinalCall(funcSymbol, callInfos);
    }

    // ---------- helper: �� orderedArgs תΪ originalArgs / halfProcessedArgs ----------
    void CompileVisitor::prepareOrderedArgs(const std::vector<std::pair<std::string, OpItem>>& orderedArgs,
                                            std::vector<std::pair<std::string, std::string>>& originalArgs,
                                            std::vector<std::string>& halfProcessedArgs) const
    {
        // ���� orderedArgs������ԭʼֵ�Ͱ봦��ֵ��raVal��
        for (const auto& [name, argItem] : orderedArgs)
        {
            originalArgs.push_back({name, argItem.getVal()});
            halfProcessedArgs.push_back(raVal(argItem));
        }
    }

    // ---------- helper: ����/Ѱ�ұ����ú������� ----------
    // �ú������ԣ�
    //  - ���� funcNameOpItem.getBelonging() ���������Ա�󶨡��๹��ȣ�
    //  - ��� customType �Ƿ�Ϊ�Զ������Ͳ���ȡ���캯��
    //  - �� funcNameOpItem ���õ� symbol������� symbol �����ж�
    //  - �������ڷ��ű��в��ҳɹ���ͬ������
    std::shared_ptr<FunctionSymbol> CompileVisitor::resolveFunctionSymbol(const OpItem& funcNameOpItem,
                                                                          std::string& customTypeVid,
                                                                          std::vector<std::string>& fullProcessedArgs,
                                                                          const std::queue<OpItem>& posArgs,
                                                                          const std::unordered_map<std::string, OpItem>&
                                                                          namedArgs,
                                                                          const std::vector<std::pair<
                                                                              std::string, OpItem>>& orderedArgs,
                                                                          const FunctionCallNode& node) const
    {
        std::shared_ptr<FunctionSymbol> funcSymbol = nullptr;

        // ԭʵ���ж� belongingSymbol��referenceSymbol �Ĵ����߼�
        if (const auto& belongingSymbol = funcNameOpItem.getBelonging())
        {
            if (const auto& referenceSymbol = getReferenceTargetSymbol(funcNameOpItem);
                referenceSymbol && referenceSymbol->is(SymbolType::FUNCTION))
            {
                funcSymbol = std::static_pointer_cast<FunctionSymbol>(referenceSymbol);
                if (!funcSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::STATIC) &&
                    !funcSymbol->getLabelMarkManager().isLifeCycleLabelMarked(LifeCycleLabel::ORDINARY))
                {
                    fullProcessedArgs.push_back(belongingSymbol->getRaVal());
                }
            }
            else if (referenceSymbol && referenceSymbol->is(SymbolType::CLASS))
            {
                customTypeVid = referenceSymbol->getRaVal();
            }
            else
            {
                fullProcessedArgs.push_back(belongingSymbol->getRaVal());
            }
        }
        else if (const auto& reference = getReferenceTargetSymbol(funcNameOpItem))
        {
            if (reference->is(SymbolType::CLASS))
            {
                customTypeVid = reference->getRaVal();
            }
        }

        // ����� custom type��Ѱ�ҹ��캯��
        if (TypeLabelSymbol::isCustomType(customTypeVid))
        {
            const auto& classSymbol = TypeLabelSymbol::getCustomClassSymbol(customTypeVid);
            funcSymbol = getCtorSymbol(classSymbol, posArgs, namedArgs, node.getPos(), orderedArgs);
            return funcSymbol;
        }

        // ��� funcNameOpItem ֱ��������ĳ�� symbol
        if (const auto& symbol = funcNameOpItem.getReferencedSymbol())
        {
            if (symbol->is(SymbolType::FUNCTION))
            {
                funcSymbol = std::static_pointer_cast<FunctionSymbol>(symbol);
            }
            else if (symbol->is(SymbolType::VARIABLE))
            {
                if (const auto& refSymbol = getReferenceTargetSymbol(std::static_pointer_cast<VariableSymbol>(symbol));
                    refSymbol && refSymbol->is(SymbolType::FUNCTION))
                {
                    funcSymbol = std::static_pointer_cast<FunctionSymbol>(refSymbol);
                }
            }
            return funcSymbol;
        }

        // �������ڷ��ű��а����ֲ���
        if (const auto& [fst, snd] = symbolTable.findByName(funcNameOpItem.getVal()); snd)
        {
            if (snd->is(SymbolType::FUNCTION))
            {
                funcSymbol = std::static_pointer_cast<FunctionSymbol>(snd);
            }
            else if (snd->is(SymbolType::VARIABLE))
            {
                if (const auto& refSymbol = getReferenceTargetSymbol(std::static_pointer_cast<VariableSymbol>(snd));
                    refSymbol && refSymbol->is(SymbolType::FUNCTION))
                {
                    funcSymbol = std::static_pointer_cast<FunctionSymbol>(refSymbol);
                }
            }
        }

        return funcSymbol;
    }

    // ---------- helper: ���Ҳ��� funcSymbol ʱ�Ĵ�������ԭ�߼��� ----------
    // ����ܹ��������� CALL/IVOK/���������� true ��ʾ processFunctionCallNode �ѽ�����
    // ���׳��쳣���ֱ�������ס�
    void CompileVisitor::handleNoFuncSymbolCase(const OpItem& funcNameOpItem,
                                                const std::vector<std::string>& halfProcessedArgs,
                                                const FunctionCallNode& node)
    {
        if (const auto& funcNameSymbol = getSymbolFromOpItem(funcNameOpItem);
            !funcNameSymbol)
        {
            throw RCCCompilerError::symbolNotFoundError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                        funcNameOpItem.toString(), "", {});
        }
        else if (const auto& funcType = getTypeLabelFromSymbol(funcNameSymbol);
            funcType->is("func"))
        {
            // �������ö������� func ���ͣ����ͱ�ǩ��������ֱ�� CALL
            raCodeBuilder << ri::CALL(funcNameSymbol->getRaVal(), halfProcessedArgs);
        }
        else if (funcType->is("funi"))
        {
            // funi ���ͣ����ò�������ʱ�Ի�÷���ֵ
            pushTemOpVarItemWithRecord(node.getPos(), getBuiltinTypeSymbol(Pos::UNKNOW_POS, B_ANY));
            raCodeBuilder << ri::IVOK(funcNameSymbol->getRaVal(), halfProcessedArgs, topOpRaVal());
        }
        else
        {
            throw RCCCompilerError::typeMissmatchError(node.getPos().toString(), getCodeLine(node.getPos()),
                                                       "It is not clear whether the called function has a return value.",
                                                       getListFormatString({"func", "funi"}), funcType->getVal(), {
                                                           "Please ensure that the called function can clearly indicate whether there is a return value."
                                                       });
        }
    }

    // ---------- helper: ����������������������� fullProcessedArgs�� ----------
    // �÷�������Ϊԭ�����б��� parameters �����ݣ��߼�δ�ı䣬����ɷ�����
    void CompileVisitor::processParameters(const std::shared_ptr<FunctionSymbol>& funcSymbol,
                                           std::queue<OpItem>& posArgs,
                                           std::unordered_map<std::string, OpItem>& namedArgs,
                                           const std::vector<std::pair<std::string, OpItem>>& orderedArgs,
                                           std::vector<std::string>& fullProcessedArgs,
                                           const Pos& callPos)
    {
        for (const auto& parameters = funcSymbol->getParameters();
            const auto& param : parameters)
        {
            if (param->getParamType() == ParamType::PARAM_POSITIONAL || param->getParamType() ==
                ParamType::PARAM_KEYWORD)
            {
                if (const auto& it = namedArgs.find(param->getVal());
                    it != namedArgs.end())
                {
                    fullProcessedArgs.push_back(it->second.getRaVal(symbolTable));
                    namedArgs.erase(it);
                }
                else if (posArgs.size() > 0)
                {
                    if (!checkTypeMatch(param, posArgs.front()))
                    {
                        const auto& [fst, snd] = getTypesFromOpItem(posArgs.front());
                        throw RCCCompilerError::typeMissmatchError(callPos.toString(),
                                                                   getCodeLine(callPos),
                                                                   std::vector{
                                                                       "Error symbol: " + param->toString(),
                                                                       "Type mismatched value: " + posArgs.front().
                                                                       toString()
                                                                   }, param->getTypeLabel()->toString(),
                                                                   fst
                                                                       ? fst->toString()
                                                                       : snd
                                                                       ? snd->toString()
                                                                       : RCC_UNKNOWN_CONST,
                                                                   {
                                                                       "You can try using the `any` type to set the parameter types more loosely."
                                                                   });
                    }
                    fullProcessedArgs.push_back(posArgs.front().getRaVal(symbolTable));
                    posArgs.pop();
                }
                else if (param->getDefaultValue().has_value())
                {
                    fullProcessedArgs.push_back(param->getDefaultValue().value());
                }
                else
                {
                    // ����ȱʧ���׳���ԭʵ��һ�µ� argumentError
                    throw RCCCompilerError::argumentError(callPos.toString(), getCodeLine(callPos),
                                                          {
                                                              "The number of arguments passed when calling the function does not match.",
                                                              "Function position: " + (
                                                                  builtin::isPureBuiltinFunction(funcSymbol->getVal())
                                                                      ? "'" + funcSymbol->getVal() +
                                                                      "' is a pure built-in function, and its position cannot be viewed."
                                                                      : funcSymbol->getPos().toString()),
                                                              "                 | " + (
                                                                  builtin::isPureBuiltinFunction(funcSymbol->getVal())
                                                                      ? "The definition of pure built-in function cannot be viewed."
                                                                      : getCodeLine(funcSymbol->getPos())),
                                                              "Function params: " + getListFormatString([parameters]
                                                              {
                                                                  StringVector result{};
                                                                  for (const auto& item : parameters)
                                                                  {
                                                                      result.push_back(item->toString());
                                                                  }
                                                                  return result;
                                                              }()),
                                                              "Arguments passed: " + getListFormatString([orderedArgs]
                                                              {
                                                                  StringVector result{};
                                                                  for (const auto& item : orderedArgs |
                                                                       std::views::values)
                                                                  {
                                                                      result.push_back(item.toString());
                                                                  }
                                                                  return result;
                                                              }()),
                                                              "Missing param: " + param->toString()
                                                          }, {
                                                              "Please ensure that the number of parameters passed in when calling the function matches."
                                                          });
                }
            }
            else if (param->getParamType() == ParamType::PARAM_VAR_LEN_POSITIONAL)
            {
                std::vector<std::string> items;
                items.reserve(posArgs.size());
                bool needAutoPack = true;
                while (!posArgs.empty())
                {
                    const auto& argItem = posArgs.front();
                    // �ȼ������ƥ��
                    if (!checkTypeMatch(param, argItem))
                    {
                        const auto& [fst, snd] = getTypesFromOpItem(argItem);
                        throw RCCCompilerError::typeMissmatchError(callPos.toString(),
                                                                   getCodeLine(callPos),
                                                                   std::vector{
                                                                       "Error symbol: " + param->toString(),
                                                                       "Type mismatched value: " + argItem.toString()
                                                                   }, param->getTypeLabel()->toString(),
                                                                   fst
                                                                       ? fst->toString()
                                                                       : snd
                                                                       ? snd->toString()
                                                                       : RCC_UNKNOWN_CONST,
                                                                   {
                                                                       "You can try using the `any` type to set the parameter types more loosely."
                                                                   });
                    }
                    // ���ݲ�ͬ���ʹ������
                    std::string item;
                    switch (argItem.getType())
                    {
                    case OpItemType::IDENTIFIER:
                        {
                            const auto& argSymbol = getSymbolFromOpItem(argItem);
                            checkExists(argItem, callPos);
                            item = argSymbol->getRaVal();
                            if (argSymbol->is(SymbolType::VARIABLE))
                            {
                                if (const auto& varSymbol = std::static_pointer_cast<VariableSymbol>(argSymbol);
                                    varSymbol->getTypeLabel()->is("series") ||
                                    varSymbol->getValueType()->is("series"))
                                {
                                    // ���������� series ���ͱ���������Ϊ�û��ֶ�������������Զ����
                                    needAutoPack = false;
                                    pushTemOpVarItemWithRecord(Pos::UNKNOW_POS,
                                                               TypeLabelSymbol::listTypeSymbol(
                                                                   Pos::UNKNOW_POS, curScopeLevel()));
                                    raCodeBuilder
                                        << ri::COPY(raVal(posArgs.front()), topOpRaVal())
                                        << ri::TP_SET(topOpItem().getValueType()->getRaVal(), topOpRaVal());
                                }
                            }
                        }
                        break;
                    case OpItemType::LITERAL_VALUE:
                        {
                            item = raVal(argItem);
                        }
                        break;
                    default:
                        throw RCCCompilerError::typeMissmatchError(callPos.toString(), getCodeLine(callPos),
                                                                   "The type of the parameter operation item passed to the function call is incorrect.",
                                                                   getListFormatString({
                                                                       opItemTypeToFormatString(OpItemType::IDENTIFIER),
                                                                       opItemTypeToFormatString(
                                                                           OpItemType::LITERAL_VALUE)
                                                                   }), opItemTypeToFormatString(argItem.getType()), {
                                                                       "Please ensure that the types of the arguments passed to the function call are legal."
                                                                   });
                    }
                    items.push_back(std::move(item));
                    posArgs.pop();
                }
                if (needAutoPack)
                {
                    pushTemOpVarItemWithRecord(callPos);
                    raCodeBuilder
                        << ri::TP_SET(
                            TypeLabelSymbol::listTypeSymbol(callPos, symbolTable.curScopeLevel())->getRaVal(),
                            topOpRaVal());
                    if (!items.empty())
                    {
                        raCodeBuilder << ri::ITER_APND(items, topOpRaVal());
                    }
                }
                fullProcessedArgs.push_back(rPopOpItemRaVal());
            }
            else if (param->getParamType() == ParamType::PARAM_VAR_LEN_KEYWORD)
            {
                std::vector<std::string> items{};
                for (const auto& [name, opItem] : namedArgs)
                {
                    if (!checkTypeMatch(param, opItem))
                    {
                        const auto& [fst, snd] = getTypesFromOpItem(opItem);
                        throw RCCCompilerError::typeMissmatchError(callPos.toString(),
                                                                   getCodeLine(callPos),
                                                                   std::vector{
                                                                       "Error symbol: " + param->toString(),
                                                                       "Type mismatched value: " + opItem.toString()
                                                                   }, param->getTypeLabel()->toString(),
                                                                   fst
                                                                       ? fst->toString()
                                                                       : snd
                                                                       ? snd->toString()
                                                                       : RCC_UNKNOWN_CONST,
                                                                   {
                                                                       "You can try using the `any` type to set the parameter types more loosely."
                                                                   });
                    }
                    pushTemOpVarItemWithRecord(callPos);
                    raCodeBuilder
                        << ri::PAIR_SET(StringManager::toStringFormat(name), opItem.getRaVal(symbolTable),
                                        topOpRaVal());
                    items.push_back(rPopOpItemRaVal());
                }
                pushTemOpVarItemWithRecord(callPos);
                raCodeBuilder
                    << ri::TP_SET(
                        TypeLabelSymbol::dictTypeSymbol(callPos, symbolTable.curScopeLevel())->getRaVal(),
                        topOpRaVal());
                if (!items.empty())
                {
                    raCodeBuilder << ri::ITER_APND(items, topOpRaVal());
                }
                fullProcessedArgs.push_back(rPopOpItemRaVal());
            }
        }
    }

    // ---------- helper: ���� funcSymbol �����������յ���ָ�builtin / normal�� ----------
    void CompileVisitor::emitFinalCall(const std::shared_ptr<FunctionSymbol>& funcSymbol,
                                       const builtin::CallInfos& callInfos)
    {
        // ����Ƿ� builtin �������������ڽ���
        if ((funcSymbol->is(TypeOfBuiltin::BUILTIN) ||
                funcSymbol->is(TypeOfBuiltin::PURE_BUILTIN)) &&
            builtin::isBuiltin(funcSymbol->getVal()))
        {
            // ���� builtin::callBuiltinFunction ����
            auto callInfosCopy = callInfos;
            callInfosCopy.processedArgs = callInfos.processedArgs;
            const auto& raCode = builtin::callBuiltinFunction(*this, funcSymbol->getVal(), callInfosCopy);
            raCodeBuilder << raCode;
        }
        else
        {
            // ��ͨ�������ã������Ƿ��з���ֵ���� IVOK / CALL
            if (funcSymbol->hasReturnValue())
            {
                pushTemOpVarItemWithRecord(callInfos.callPos, funcSymbol->getReturnType());
                raCodeBuilder << ri::IVOK(funcSymbol->getRaVal(), callInfos.processedArgs, topOpRaVal());
            }
            else
            {
                raCodeBuilder << ri::CALL(funcSymbol->getRaVal(), callInfos.processedArgs);
            }
        }
    }
} // namespace ast
