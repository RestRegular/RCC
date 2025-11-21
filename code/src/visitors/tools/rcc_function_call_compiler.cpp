//
// Created by RestRegular on 2025/10/1.
//

#include <queue>
#include <unordered_map>
#include <vector>
#include <string>

#include "../../../include/visitors/rcc_compile_visitor.h"
#include "../../../include/components/symbol/rcc_symbol.h"
#include "../../../include/builtin/rcc_builtin.h"

namespace ast
{
    using namespace symbol;

    // ---------- 主调度函数：将原始 visitFunctionCallNode 的逻辑组织调用各 helper ----------
    void CompileVisitor::processFunctionCallNode(const FunctionCallNode& node)
    {
        // 提前准备：位置参数、命名参数、有序参数
        std::queue<OpItem> posArgs;
        std::unordered_map<std::string, OpItem> namedArgs;
        std::vector<std::pair<std::string, OpItem>> orderedArgs;

        // 存放最终传入调用的 ra 表示（完整参数）
        std::vector<std::string> fullProcessedArgs{};
        // 存放原始参数信息（name, raw）
        std::vector<std::pair<std::string, std::string>> originalArgs{};
        // 存放“半处理”参数（只做 raVal）
        std::vector<std::string> halfProcessedArgs{};

        // classifyFuncArgs 为已有的方法，负责填充 posArgs/namedArgs/orderedArgs
        classifyFuncArgs(node, posArgs, namedArgs, orderedArgs);

        // 备份 posArgs 和 namedArgs，用于后面填充 callInfos
        const auto posArgsCopied = posArgs;
        const auto namedArgsCopied = namedArgs;

        // 将 orderedArgs 转为 originalArgs / halfProcessedArgs
        prepareOrderedArgs(orderedArgs, originalArgs, halfProcessedArgs);

        // 查找调用的函数符号
        std::shared_ptr<FunctionSymbol> funcSymbol = nullptr;

        // 处理被调用表达式（左侧）
        node.getLeftNode()->acceptVisitor(*this);
        const auto& funcNameOpItem = rPopOpItem();

        // customTypeVid 初始值通常来自 funcNameOpItem 的 raVal
        std::string customTypeVid = funcNameOpItem.getRaVal(symbolTable);

        // resolveFunctionSymbol 会尝试从各种来源解析出 funcSymbol（可能为构造函数、引用到的函数等）
        funcSymbol = resolveFunctionSymbol(funcNameOpItem, customTypeVid, fullProcessedArgs,
                                           posArgs, namedArgs, orderedArgs, node);

        // 如果未解析到 funcSymbol，则尝试按符号名处理（可能是 func/funi 类型的类型标签等）
        if (!funcSymbol)
        {
            // 如果该分支内部处理成功（生成 CALL/IVOK），则直接返回
            handleNoFuncSymbolCase(funcNameOpItem, halfProcessedArgs, node);
            return;
        }

        // 到这里，funcSymbol 已经解析到（或抛错）
        // 右侧必须是 RANGER 节点，保持原语义检查
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

        // 根据函数参数列表，处理 posArgs / namedArgs，填充 fullProcessedArgs
        processParameters(funcSymbol, posArgs, namedArgs, orderedArgs, fullProcessedArgs, node.getPos());

        // 组装 builtin::CallInfos 供 builtin 调用使用（与原代码保持一致）
        const builtin::CallInfos& callInfos{
            fullProcessedArgs, originalArgs, posArgsCopied,
            namedArgsCopied, node.getPos(), orderedArgs,
            std::make_shared<FunctionCallNode>(node)
        };

        // 最终根据 funcSymbol 的类型选择生成 builtin 调用或普通调用
        emitFinalCall(funcSymbol, callInfos);
    }

    // ---------- helper: 将 orderedArgs 转为 originalArgs / halfProcessedArgs ----------
    void CompileVisitor::prepareOrderedArgs(const std::vector<std::pair<std::string, OpItem>>& orderedArgs,
                                            std::vector<std::pair<std::string, std::string>>& originalArgs,
                                            std::vector<std::string>& halfProcessedArgs) const
    {
        // 遍历 orderedArgs，保持原始值和半处理值（raVal）
        for (const auto& [name, argItem] : orderedArgs)
        {
            originalArgs.emplace_back(name, argItem.getVal());
            halfProcessedArgs.push_back(raVal(argItem, true));
        }
    }

    // ---------- helper: 解析/寻找被调用函数符号 ----------
    // 该函数尝试：
    //  - 处理 funcNameOpItem.getBelonging() 的情况（成员绑定、类构造等）
    //  - 检查 customType 是否为自定义类型并获取构造函数
    //  - 若 funcNameOpItem 引用到 symbol，则根据 symbol 类型判断
    //  - 若名称在符号表中查找成功，同样处理
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

        // 原实现中对 belongingSymbol、referenceSymbol 的处理逻辑
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

        // 如果是 custom type，寻找构造函数
        if (TypeLabelSymbol::isCustomType(customTypeVid))
        {
            const auto& classSymbol = TypeLabelSymbol::getCustomClassSymbol(customTypeVid);
            funcSymbol = getCtorSymbol(classSymbol, posArgs, namedArgs, node.getPos(), orderedArgs);
            return funcSymbol;
        }

        // 如果 funcNameOpItem 直接引用了某个 symbol
        if (const auto& symbol = funcNameOpItem.getReferencedSymbol())
        {
            if (symbol->is(SymbolType::FUNCTION))
            {
                funcSymbol = std::static_pointer_cast<FunctionSymbol>(symbol);
            }
            else if (symbol->is(SymbolType::VARIABLE))
            {
                const auto& varSymbol = std::static_pointer_cast<VariableSymbol>(symbol);
                // 处理变量引用的符号是函数的情况
                if (const auto& refSymbol = getReferenceTargetSymbol(varSymbol);
                    refSymbol && refSymbol->is(SymbolType::FUNCTION))
                {
                    funcSymbol = std::static_pointer_cast<FunctionSymbol>(refSymbol);
                }
                // 处理类型标签为"func"、"funi"、"any"的变量
                else
                {
                    const auto& typeLabel = varSymbol->getTypeLabel();
                    if (typeLabel->is("func"))
                    {
                        handleFuncTypeLabel(varSymbol, typeLabel, funcSymbol);
                    }
                    else if (typeLabel->is("funi"))
                    {
                        handleFuniTypeLabel(varSymbol, typeLabel, funcSymbol);
                    }
                    else if (typeLabel->is("any"))
                    {
                        pass();
                    }
                }
            }
            return funcSymbol;
        }

        // 否则尝试在符号表中按名字查找
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

    // 提取参数表的标签描述
    std::vector<std::shared_ptr<LabelSymbol>> CompileVisitor::extractParamTableLabelDes(
        const std::vector<std::shared_ptr<ParameterSymbol>>& params)
    {
        std::vector<std::shared_ptr<LabelSymbol>> paramLabelDes{};
        for (const auto& param : params)
        {
            paramLabelDes.push_back(param->getTypeLabel());
        }
        return paramLabelDes;
    }

    // 提取的辅助函数：处理"func"类型标签
    void CompileVisitor::handleFuncTypeLabel(const std::shared_ptr<VariableSymbol>& varSymbol,
                                             const std::shared_ptr<LabelSymbol>& typeLabel,
                                             std::shared_ptr<FunctionSymbol>& funcSymbol) const
    {
        const auto& labelDesS = typeLabel->getLabelDesS();
        if (labelDesS.empty())
        {
            return;
        }
        if (labelDesS.size() != 1)
        {
            throwRCCLabelDesError(varSymbol, typeLabel, labelDesS.size(), 1);
            return;
        }

        auto params = createParameters(labelDesS[0], varSymbol);
        const auto& signature = TypeLabelSymbol::funcTypeSymbol(getUnknownPos(), curScopeLevel());
        signature->appendLabelDes(extractParamTableLabelDes(params));
        funcSymbol = std::make_shared<FunctionSymbol>(
            nullptr, varSymbol->getPos(), varSymbol->getVal(),
            varSymbol->getRaVal(),
            std::unordered_set<std::shared_ptr<LabelSymbol>>{
                TypeLabelSymbol::voidTypeSymbol(getUnknownPos(), curScopeLevel())
            }, params, signature, curScopeLevel(), TypeOfBuiltin::ORDINARY,
            FunctionType::FUNCTION, nullptr);
    }

    // 提取的辅助函数：处理"funi"类型标签
    void CompileVisitor::handleFuniTypeLabel(const std::shared_ptr<VariableSymbol>& varSymbol,
                                             const std::shared_ptr<LabelSymbol>& typeLabel,
                                             std::shared_ptr<FunctionSymbol>& funcSymbol) const
    {
        auto returnType = TypeLabelSymbol::anyTypeSymbol(getUnknownPos(), curScopeLevel());
        const auto& labelDesS = typeLabel->getLabelDesS();

        if (labelDesS.empty())
        {
            return;
        }
        if (labelDesS.size() > 2)
        {
            throwRCCLabelDesError(varSymbol, typeLabel, labelDesS.size(), "0 ~ 2");
            return;
        }
        if (labelDesS.size() == 2)
        {
            handleFuniReturnType(labelDesS[1], returnType);
        }

        auto params = createParameters(labelDesS[0], varSymbol);
        const auto& signature = TypeLabelSymbol::funiTypeSymbol(getUnknownPos(), curScopeLevel());
        signature->appendLabelDes(extractParamTableLabelDes(params));
        signature->appendLabelDes({returnType});
        funcSymbol = std::make_shared<FunctionSymbol>(
            nullptr, varSymbol->getPos(), varSymbol->getVal(),
            varSymbol->getRaVal(),
            std::unordered_set<std::shared_ptr<LabelSymbol>>{returnType},
            params, signature, curScopeLevel(), TypeOfBuiltin::ORDINARY,
            FunctionType::FUNCTION, nullptr);
    }

    // 提取的辅助函数：创建参数列表
    std::vector<std::shared_ptr<ParameterSymbol>> CompileVisitor::createParameters(
        const std::vector<std::shared_ptr<LabelSymbol>>& desList,
        const std::shared_ptr<VariableSymbol>& varSymbol) const
    {
        std::vector<std::shared_ptr<ParameterSymbol>> params;
        int paramIndex = 0;

        for (const auto& des : desList)
        {
            if (des->getLabelType() != LabelType::TYPE_LABEL)
            {
                throwRCCTypeMissmatchError(des, varSymbol->getTypeLabel(), LabelType::TYPE_LABEL);
            }

            const auto& typeLabel = std::static_pointer_cast<TypeLabelSymbol>(des);
            auto paramType = getParamTypeFromLabel(typeLabel);

            VarID paramVarID{
                "autoParam" + std::to_string(paramIndex), currentPos().getFileField(),
                curScopeField(), curScopeLevel()
            };

            params.push_back(std::make_shared<ParameterSymbol>(
                paramType, typeLabel->getPos(), paramVarID.getNameField(), paramVarID.getVid(),
                std::unordered_set<std::shared_ptr<LabelSymbol>>{
                    paramType == ParamType::PARAM_POSITIONAL
                        ? typeLabel
                        : TypeLabelSymbol::anyTypeSymbol(getUnknownPos(), curScopeLevel())
                }, std::nullopt, curScopeLevel()));
            paramIndex++;
        }
        return params;
    }

    // 提取的辅助函数：获取参数类型
    ParamType CompileVisitor::getParamTypeFromLabel(const std::shared_ptr<TypeLabelSymbol>& typeLabel)
    {
        if (typeLabel->getVal() == "..")
            return ParamType::PARAM_VAR_LEN_POSITIONAL;
        if (typeLabel->getVal() == ".*")
            return ParamType::PARAM_VAR_LEN_KEYWORD;
        return ParamType::PARAM_POSITIONAL;
    }

    // 提取的辅助函数：处理funi返回类型
    void CompileVisitor::handleFuniReturnType(const std::vector<std::shared_ptr<LabelSymbol>>& secondDes,
                                              std::shared_ptr<TypeLabelSymbol>& returnType)
    {
        if (secondDes.size() != 1)
        {
            throw RCCCompilerError::labelDesError(
                "", getCodeLine(currentPos()),
                {
                    "The number of the description part of the main label is wrong.",
                    "Expected number of description parts: 1",
                    "The current number of description parts: " + std::to_string(secondDes.size())
                }, {"Make sure that the number of labels is correct."});
        }

        const auto& returnTypeLabel = secondDes[0];
        if (returnTypeLabel->getLabelType() != LabelType::TYPE_LABEL)
        {
            throwRCCTypeMissmatchError(returnTypeLabel, nullptr, LabelType::TYPE_LABEL);
        }
        returnType = std::static_pointer_cast<TypeLabelSymbol>(returnTypeLabel);
    }

    // 提取的辅助函数：抛出标签描述错误
    void CompileVisitor::throwRCCLabelDesError(const std::shared_ptr<VariableSymbol>& varSymbol,
                                               const std::shared_ptr<LabelSymbol>& typeLabel,
                                               const size_t currentCount, const std::string& expected)
    {
        throw RCCCompilerError::labelDesError(
            typeLabel->getPos().toString(), getCodeLine(currentPos()),
            {
                "The number of the description of the main label is wrong.",
                "Main label: " + typeLabel->toString(),
                "Expected number of descriptions: " + expected,
                "The current number of descriptions: " + std::to_string(currentCount)
            }, {"Make sure that the number of labels is correct."});
    }

    // 重载：处理整数预期值
    void CompileVisitor::throwRCCLabelDesError(const std::shared_ptr<VariableSymbol>& varSymbol,
                                               const std::shared_ptr<LabelSymbol>& typeLabel,
                                               const size_t currentCount, const size_t expected)
    {
        throwRCCLabelDesError(varSymbol, typeLabel, currentCount, std::to_string(expected));
    }

    // 提取的辅助函数：抛出类型不匹配错误
    void CompileVisitor::throwRCCTypeMissmatchError(const std::shared_ptr<LabelSymbol>& des,
                                                    const std::shared_ptr<LabelSymbol>& typeLabel,
                                                    const LabelType expectedType)
    {
        std::string labelStr = typeLabel ? typeLabel->toString() : "";
        throw RCCCompilerError::typeMissmatchError(
            des->getPos().toString(), getCodeLine(des->getPos()),
            {
                "The label description of " + labelStr +
                " must be " + labelTypeToString(expectedType) + "."
            },
            labelTypeToString(expectedType), labelTypeToString(des->getLabelType()),
            {});
    }

    // ---------- helper: 当找不到 funcSymbol 时的处理（复制原逻辑） ----------
    // 如果能够处理（生成 CALL/IVOK/报错），返回 true 表示 processFunctionCallNode 已结束。
    // 若抛出异常则会直接向上抛。
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
            // 当被调用对象本身是 func 类型（类型标签函数），直接 CALL
            raCodeBuilder << ri::CALL(funcNameSymbol->getRaVal(), halfProcessedArgs);
        }
        else if (funcType->is("funi"))
        {
            // funi 类型，调用并推入临时以获得返回值
            pushTemOpVarItemWithRecord(node.getPos(), getBuiltinTypeSymbol(getUnknownPos(), BuiltinType::B_ANY));
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

    // ---------- helper: 处理函数参数（负责完整填充 fullProcessedArgs） ----------
    // 该方法基本为原函数中遍历 parameters 的内容，逻辑未改变，仅抽成方法。
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
                else if (!posArgs.empty())
                {
                    if (!checkTypeMatch(param, posArgs.front()))
                    {
                        const auto& frontArgSymbol = getSymbolFromOpItem(posArgs.front());
                        const auto& [fst, snd] = getTypesFromOpItem(posArgs.front());
                        throw RCCCompilerError::typeMissmatchError(
                            posArgs.front().getPos().toString(),
                            getCodeLine(callPos),
                            std::vector{
                                "Function position: " + (
                                    builtin::isPureBuiltinFunction(
                                        funcSymbol->getVal())
                                        ? "'" + funcSymbol->getVal() +
                                        "' is a pure built-in function, and its position cannot be viewed."
                                        : funcSymbol->getPos().toString()),
                                "                 | " + (
                                    builtin::isPureBuiltinFunction(
                                        funcSymbol->getVal())
                                        ? "The definition of pure built-in function cannot be viewed."
                                        : getCodeLine(funcSymbol->getPos())),
                                "Error param symbol: " + param->toString(),
                                "Type mismatched arg: " + (frontArgSymbol
                                                               ? frontArgSymbol->toString()
                                                               : posArgs.front().toString())
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
                    // 参数缺失，抛出与原实现一致的 argumentError
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
                                                              "Function signature: " + funcSymbol->getSignatureString(),
                                                              "In-passed arguments: " + getListFormatString(
                                                                  [orderedArgs]
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
                    // 先检查类型匹配
                    // if (!checkTypeMatch(param, argItem))
                    // {
                    //     const auto& [fst, snd] = getTypesFromOpItem(argItem);
                    //     throw RCCCompilerError::typeMissmatchError(argItem.getPos().toString(),
                    //                                                getCodeLine(callPos),
                    //                                                std::vector{
                    //                                                    "Error symbol: " + param->toString(),
                    //                                                    "Type mismatched value: " + argItem.toString()
                    //                                                }, param->getTypeLabel()->toString(),
                    //                                                fst
                    //                                                    ? fst->toString()
                    //                                                    : snd
                    //                                                    ? snd->toString()
                    //                                                    : RCC_UNKNOWN_CONST,
                    //                                                {
                    //                                                    "You can try using the `any` type to set the parameter types more loosely."
                    //                                                });
                    // }
                    // 根据不同类型处理参数
                    std::string item;
                    switch (argItem.getType())
                    {
                    case OpItemType::IDENTIFIER:
                        {
                            const auto& argSymbol = getSymbolFromOpItem(argItem);
                            checkExists(argItem, callPos);
                            item = argItem.getBelonging() ?
                                raVal(argItem, true) :
                                argSymbol->getRaVal();
                            if (argSymbol->is(SymbolType::VARIABLE))
                            {
                                if (const auto& varSymbol = std::static_pointer_cast<VariableSymbol>(argSymbol);
                                    varSymbol->getTypeLabel()->is("series") ||
                                    varSymbol->getValueType()->is("series"))
                                {
                                    // 如果传入的是 series 类型变量，则视为用户手动打包，不进行自动打包
                                    needAutoPack = false;
                                    pushTemOpVarItemWithRecord(getUnknownPos(),
                                                               TypeLabelSymbol::listTypeSymbol(
                                                                   getUnknownPos(), curScopeLevel()));
                                    raCodeBuilder
                                        << ri::COPY(raVal(posArgs.front(), true), topOpRaVal())
                                        << ri::TP_SET(topOpItem().getValueType()->getRaVal(), topOpRaVal());
                                }
                            }
                        }
                        break;
                    case OpItemType::LITERAL_VALUE:
                        {
                            item = raVal(argItem, true);
                        }
                        break;
                    default:
                        throw RCCCompilerError::typeMissmatchError(argItem.getPos().toString(), getCodeLine(callPos),
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
                    pushTemOpVarItemWithRecord(callPos, nullptr,
                        nullptr, true);
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
                    // if (!checkTypeMatch(param, opItem))
                    // {
                    //     const auto& [fst, snd] = getTypesFromOpItem(opItem);
                    //     throw RCCCompilerError::typeMissmatchError(opItem.getPos().toString(),
                    //                                                getCodeLine(callPos),
                    //                                                std::vector{
                    //                                                    "Error symbol: " + param->toString(),
                    //                                                    "Type mismatched value: " + opItem.toString()
                    //                                                }, param->getTypeLabel()->toString(),
                    //                                                fst
                    //                                                    ? fst->toString()
                    //                                                    : snd
                    //                                                    ? snd->toString()
                    //                                                    : RCC_UNKNOWN_CONST,
                    //                                                {
                    //                                                    "You can try using the `any` type to set the parameter types more loosely."
                    //                                                });
                    // }
                    pushTemOpVarItemWithRecord(callPos, nullptr,
                        nullptr, true);
                    raCodeBuilder
                        << ri::PAIR_SET(StringManager::toStringFormat(name), opItem.getRaVal(symbolTable),
                                        topOpRaVal());
                    items.push_back(rPopOpItemRaVal());
                }
                pushTemOpVarItemWithRecord(callPos, nullptr,
                    nullptr, true);
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

    // ---------- helper: 根据 funcSymbol 类型生成最终调用指令（builtin / normal） ----------
    void CompileVisitor::emitFinalCall(const std::shared_ptr<FunctionSymbol>& funcSymbol,
                                       const builtin::CallInfos& callInfos)
    {
        // 检查是否 builtin 函数（包含纯内建）
        if ((funcSymbol->is(TypeOfBuiltin::BUILTIN) ||
                funcSymbol->is(TypeOfBuiltin::PURE_BUILTIN)) &&
            builtin::isBuiltin(funcSymbol->getVal()))
        {
            // 交给 builtin::callBuiltinFunction 处理
            auto callInfosCopy = callInfos;
            callInfosCopy.processedArgs = callInfos.processedArgs;
            const auto& raCode = callBuiltinFunction(*this, funcSymbol->getVal(), callInfosCopy);
            raCodeBuilder << raCode;
        }
        else
        {
            // 普通函数调用：根据是否有返回值生成 IVOK / CALL
            if (funcSymbol->hasReturnValue())
            {
                pushTemOpVarItemWithRecord(callInfos.callPos, funcSymbol->getReturnType(), nullptr, true);
                raCodeBuilder << ri::IVOK(funcSymbol->getRaVal(), callInfos.processedArgs, topOpRaVal());
            }
            else
            {
                raCodeBuilder << ri::CALL(funcSymbol->getRaVal(), callInfos.processedArgs);
            }
        }
    }
} // namespace ast
