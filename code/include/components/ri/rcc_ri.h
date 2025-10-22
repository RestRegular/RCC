//
// Created by RestRegular on 2025/7/16.
//

#ifndef RCC_RI_H
#define RCC_RI_H

#include "../../lib/rcc_utils.h"

namespace ri {

    class RI;

    class RI: public utils::Object {
    public:
        RI() = default;
        [[nodiscard]] virtual std::string toRACode() const = 0;
        ~RI() override = default;

        friend std::ostream &operator<<(std::ostream &os, const RI &ri) {
            os << ri.toRACode();
            return os;
        }
    };

    class ANNOTATION final : public RI
    {
        std::string comment;
    public:
        explicit ANNOTATION(const std::string &comment);
        explicit ANNOTATION(const std::vector<std::string> &comments);
        [[nodiscard]] std::string toRACode() const override;
    };

    class BREAKPOINT final : public RI
    {
    public:
        explicit BREAKPOINT();
        [[nodiscard]] std::string toRACode() const override;
    };

    class FLAG: public RI
    {
        std::string opRI;
        std::string comment;
    public:
        explicit FLAG(const std::string &opRI, const std::string &comment="");
        std::string toRACode() const override;
    };

    class PASS final: public FLAG
    {
    public:
        explicit PASS(const std::string &comment="");
    };

    class PARALLEL: public RI {
        std::string opRI;
        std::vector<std::string> idents;
    public:
        explicit PARALLEL(const std::string &opRI, const std::vector<std::string> &idents);
        [[nodiscard]] std::string toRACode() const override;

        [[nodiscard]] std::vector<std::string> getIdents() const;
    };

    class ALLOT final : public PARALLEL {
    public:
        explicit ALLOT(const std::vector<std::string> &idents);
    };

    class DELETE final : public PARALLEL {
    public:
        explicit DELETE(const std::vector<std::string> &idents);
    };

    class SIN final : public PARALLEL {
    public:
        explicit SIN(const std::vector<std::string> &args);
    };

    class ITER_APND final: public PARALLEL {
    public:
        explicit ITER_APND(std::vector<std::string> idents, const std::string &target);
        [[nodiscard]] std::string toRACode() const override;
    };

    class FUNC final: public PARALLEL
    {
    public:
        explicit FUNC(const std::string& ident, std::vector<std::string> params);
    };

    class FUNI final: public PARALLEL
    {
    public:
        explicit FUNI(const std::string& ident, std::vector<std::string> params);
    };

    class CALL final : public PARALLEL
    {
    public:
        explicit CALL(const std::string& ident, std::vector<std::string> params);
    };

    class IVOK final : public PARALLEL
    {
    public:
        explicit IVOK(const std::string& ident, std::vector<std::string> params, std::string target);
    };

    class SOUT final : public PARALLEL
    {
    public:
        explicit SOUT(const std::string &mode, std::vector<std::string> params);
    };

    class ITER_UNPACK final : public PARALLEL
    {
    public:
        explicit ITER_UNPACK(const std::string &iterItem, std::vector<std::string> args);
    };

    class TP_DEF final : public PARALLEL
    {
    public:
        explicit TP_DEF(const std::string &ident, const std::string &parent="");
    };

    class UNARY : public RI {
        std::string ident;
        std::string opRI;
    public:
        explicit UNARY(const std::string &opRI, const std::string &ident);
        [[nodiscard]] std::string toRACode() const override;
    };

    class SET final : public UNARY {
    public:
        explicit SET(const std::string &ident);
    };

    class JMP final : public UNARY {
    public:
        explicit JMP(const std::string &ident);
    };

    class EXIT final: public UNARY
    {
    public:
        explicit EXIT(const std::string &ident);
    };

    class END final: public UNARY
    {
    public:
        explicit END(const std::string &ident);
    };

    class RET final: public UNARY
    {
    public:
        explicit RET(const std::string &ident = ";");
    };

    class BINARY : public RI {
        std::string lvalue;
        std::string rvalue;
        std::string opRI;
    public:
        BINARY(const std::string &op, const std::string &rvalue, const std::string &lvalue);
        [[nodiscard]] std::string toRACode() const override;

        std::string getRValue() const;
        std::string getLValue() const;
        std::string getOp() const;
    };

    class TP_NEW final : public BINARY {
    public:
        explicit TP_NEW(const std::string &ident, const std::string &lvalue);
    };

    class TP_DERIVE final : public BINARY
    {
    public:
        explicit TP_DERIVE(const std::string &inst, const std::string &derivedType);
    };

    class ITER_SIZE final : public BINARY {
    public:
        explicit ITER_SIZE(const std::string &ident, const std::string &lvalue);
    };

    class PUT final : public BINARY {
    public:
        explicit PUT(const std::string &rvalue, const std::string &lvalue);
    };

    class COPY final : public BINARY {
    public:
        explicit COPY(const std::string &rvalue, const std::string &lvalue);
    };

    class OPP final : public BINARY {
    public:
        explicit OPP (const std::string &rvalue, const std::string &lvalue);
    };

    class JT final : public BINARY {
    public:
        explicit JT(const std::string &rvalue, const std::string &lvalue);
    };

    class JF final : public BINARY {
    public:
        explicit JF(const std::string &rvalue, const std::string &lvalue);
    };

    class TP_SET final : public BINARY {
    public:
        explicit TP_SET(const std::string &rvalue, const std::string &lvalue);
        [[nodiscard]] std::string toRACode() const override;
    };

    class UNTIL final : public BINARY
    {
    public:
        explicit UNTIL(const std::string &cmpData, const std::string &rel);
    };

    class TP_GET final : public BINARY
    {
    public:
        explicit TP_GET(const std::string &rvalue, const std::string &lvalue);
    };

    class ITER_DEL final: public BINARY
    {
    public:
        explicit ITER_DEL(const std::string &iterData, const std::string &index);
    };

    class DICT_KEYS final: public BINARY
    {
    public:
        explicit DICT_KEYS(const std::string &dictData, const std::string &lvalue);
    };

    class DICT_VALUES final: public BINARY
    {
    public:
        explicit DICT_VALUES(const std::string &dictData, const std::string &lvalue);
    };

    class DICT_DEL final: public BINARY
    {
    public:
        explicit DICT_DEL(const std::string &dictData, const std::string &key);
    };

    class TERNARY: public RI {
        std::string opRI, rvalue1, rvalue2, lvalue;
    public:
        explicit TERNARY(
            const std::string &op, const std::string &rvalue1,
            const std::string &rvalue2, const std::string &lvalue);
        [[nodiscard]] std::string toRACode() const override;
    };

    class ADD final : public TERNARY {
    public:
        explicit ADD(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class MUL: public TERNARY {
    public:
        explicit MUL(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class DIV: public TERNARY {
    public:
        explicit DIV(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class MOD: public TERNARY {
    public:
        explicit MOD(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class POW: public TERNARY {
    public:
        explicit POW(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class ROOT final : public TERNARY {
    public:
        explicit ROOT(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class CMP final : public TERNARY {
    public:
        explicit CMP(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class CREL final : public TERNARY {
    public:
        explicit CREL(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue);
    };

    class JR final : public TERNARY {
    public:
        explicit JR(const std::string &cmpData, const std::string &rel, const std::string &lvalue);
    };

    class PAIR_SET final: public TERNARY
    {
    public:
        explicit PAIR_SET(const std::string &key, const std::string &value, const std::string &lvalue);
    };

    class ITER_GET final: public TERNARY
    {
    public:
        explicit ITER_GET(const std::string &ident, const std::string &index, const std::string &target);
    };

    class TP_ADD_TP_FIELD final : public TERNARY
    {
    public:
        explicit TP_ADD_TP_FIELD(const std::string &tp, const std::string &field, const std::string &value);
    };

    class TP_ADD_INST_FIELD final : public TERNARY
    {
    public:
        explicit TP_ADD_INST_FIELD(const std::string &tp, const std::string &field, const std::string &value = "null");
    };

    class TP_GET_FIELD final : public TERNARY
    {
    public:
        explicit TP_GET_FIELD(const std::string &tp, const std::string &field, const std::string &lvalue = "null");
    };

    class TP_SET_FIELD final : public TERNARY
    {
    public:
        explicit TP_SET_FIELD(const std::string &tp, const std::string &field, const std::string &value);
    };

}

#endif //RCC_RI_H
