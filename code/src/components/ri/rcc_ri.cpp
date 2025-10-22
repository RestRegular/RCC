//
// Created by RestRegular on 2025/7/16.
//

#include "../../../include/components/ri/rcc_ri.h"

namespace ri {
    ANNOTATION::ANNOTATION(const std::string& comment)
        : comment(comment) {}

    ANNOTATION::ANNOTATION(const std::vector<std::string>& comments)
        : comment([&]()
        {
            std::string result = "";
            for (int i = 0; i < comments.size(); i++)
            {
                if (comments[i].empty()) continue;
                result += comments[i];
                if (i != comments.size() - 1)
                    result += "\n";
                if (i + 1 < comments.size() && !comments[i + 1].empty())
                {
                    result += "~ ";
                }
            }
            return result;
        }()) {}

    std::string ANNOTATION::toRACode() const
    {
        return "; " + comment + "\n";
    }

    BREAKPOINT::BREAKPOINT() {}

    std::string BREAKPOINT::toRACode() const
    {
        return "BREAKPOINT: ;\n";
    }


    FLAG::FLAG(const std::string& opRI, const std::string &comment)
        : opRI(opRI), comment(comment) {}

    std::string FLAG::toRACode() const
    {
        return opRI + ": ;" + (comment.empty() ? "" : " " + comment) + "\n";
    }

    PASS::PASS(const std::string& comment)
        : FLAG("PASS", comment) {}

    PARALLEL::PARALLEL(const std::string &opRI, const std::vector<std::string> &idents)
        : opRI(opRI), idents(idents) {}

    std::string PARALLEL::toRACode() const {
        std::string raCode = opRI + ": ";
        for (size_t i = 0; i < idents.size(); i++) {
            raCode += idents[i];
            if (i != idents.size() - 1) {
                raCode += ", ";
            }
        }
        raCode += "\n";
        return raCode;
    }

    std::vector<std::string> PARALLEL::getIdents() const
    {
        return idents;
    }

    ALLOT::ALLOT(const std::vector<std::string> &idents)
        : PARALLEL("ALLOT", idents) {}

    DELETE::DELETE(const std::vector<std::string> &idents)
        : PARALLEL("DELETE", idents){}

    SIN::SIN(const std::vector<std::string> &args)
        : PARALLEL("SIN", args) {}

    ITER_APND::ITER_APND(std::vector<std::string> idents, const std::string &target)
        : PARALLEL("ITER_APND", [&]() mutable {
            idents.push_back(target);
            return idents;
        }()) {}

    std::string ITER_APND::toRACode() const
    {
        if (getIdents().size() == 1)
        {
            return ANNOTATION("To generate an empty list does not need to use the [ITER_APND] ri.").toRACode();
        }
        std::string code{"ITER_APND: "};
        const auto &idents = getIdents();
        for (int i = 0; i < idents.size(); i++)
        {
            code += idents[i];
            if (i != idents.size() - 1)
            {
                code += ", ";
            }
        }
        code += "\n";
        return code;
    }

    FUNC::FUNC(const std::string& ident, std::vector<std::string> params)
        : PARALLEL("FUNC", [&]() mutable
        {
            params.insert(params.begin(), ident);
            return params;
        }()) {}

    FUNI::FUNI(const std::string& ident, std::vector<std::string> params)
        : PARALLEL("FUNI", [&]() mutable
        {
            params.insert(params.begin(), ident);
            return params;
        }()) {}

    CALL::CALL(const std::string& ident, std::vector<std::string> params)
        : PARALLEL("CALL", [&]() mutable
        {
            params.insert(params.begin(), ident);
            return params;
        }()) {}

    IVOK::IVOK(const std::string& ident, std::vector<std::string> params, std::string target)
        : PARALLEL("IVOK", [&]() mutable
        {
            params.insert(params.begin(), ident);
            params.push_back(target);
            return params;
        }()) {}

    SOUT::SOUT(const std::string& mode, std::vector<std::string> params)
        : PARALLEL("SOUT", [&]() mutable
        {
            params.insert(params.begin(), mode);
            return params;
        }()){
    }

    ITER_UNPACK::ITER_UNPACK(const std::string& iterItem, std::vector<std::string> args)
        : PARALLEL("ITER_UNPACK", [&]() mutable
        {
            args.insert(args.begin(), iterItem);
            return args;
        }()) {}

    TP_DEF::TP_DEF(const std::string& ident, const std::string& parent)
        : PARALLEL("TP_DEF", parent.empty() ? std::vector{ident} : std::vector{ident, parent}){}

    UNARY::UNARY(const std::string &opRI, const std::string &ident)
        : ident(ident), opRI(opRI) {}

    std::string UNARY::toRACode() const {
        return opRI + ": " + ident + "\n";
    }

    SET::SET(const std::string &ident)
        : UNARY("SET", ident) {}

    JMP::JMP(const std::string &ident)
        : UNARY("JMP", ident) {}

    EXIT::EXIT(const std::string& ident)
        : UNARY("EXIT", ident) {}

    END::END(const std::string& ident)
        : UNARY("END", ident) {}

    RET::RET(const std::string& ident)
        : UNARY("RET", ident) {}

    BINARY::BINARY(const std::string &op, const std::string &rvalue, const std::string &lvalue)
        : lvalue(lvalue), rvalue(rvalue), opRI(op) {}

    std::string BINARY::toRACode() const {
        return opRI + ": " + rvalue + ", " + lvalue + "\n";
    }

    std::string BINARY::getRValue() const
    {
        return rvalue;
    }

    std::string BINARY::getLValue() const
    {
        return lvalue;
    }

    std::string BINARY::getOp() const
    {
        return opRI;
    }

    TP_NEW::TP_NEW(const std::string& ident, const std::string& lvalue)
        : BINARY("TP_NEW", ident, lvalue) {}

    TP_DERIVE::TP_DERIVE(const std::string& inst, const std::string& derivedType)
        : BINARY("TP_DERIVE", inst, derivedType) {}

    ITER_SIZE::ITER_SIZE(const std::string& ident, const std::string& lvalue)
        : BINARY("ITER_SIZE", ident, lvalue) {}

    PUT::PUT(const std::string &rvalue, const std::string &lvalue)
         : BINARY("PUT", rvalue, lvalue) {}

    COPY::COPY(const std::string &rvalue, const std::string &lvalue)
        : BINARY("COPY", rvalue, lvalue) {}

    OPP::OPP(const std::string &rvalue, const std::string &lvalue)
         : BINARY("OPP", rvalue, lvalue) {}

    JT::JT(const std::string &rvalue, const std::string &lvalue)
        : BINARY("JT", rvalue, lvalue) {}

    JF::JF(const std::string &rvalue, const std::string &lvalue)
        : BINARY("JF", rvalue, lvalue) {}

    TP_SET::TP_SET(const std::string &rvalue, const std::string &lvalue)
        : BINARY("TP_SET", rvalue, lvalue) {}

    std::unordered_set<std::string> noNeedSetTypeLabels = {
        "tp-void", "tp-any", "tp-func", "tp-funi", "tp-clas"
    };

    std::string TP_SET::toRACode() const
    {
        if (noNeedSetTypeLabels.contains(getRValue()))
        {
            return "; No need to set '" + getRValue() + "' type.\n";
        }
        return "TP_SET: " + getRValue() + ", " + getLValue() + "\n";
    }

    UNTIL::UNTIL(const std::string& cmpData, const std::string& rel)
        : BINARY("UNTIL", cmpData, rel) {}

    TP_GET::TP_GET(const std::string& rvalue, const std::string& lvalue)
        : BINARY("TP_GET", rvalue, lvalue) {}

    ITER_DEL::ITER_DEL(const std::string& iterData, const std::string& index)
        : BINARY("ITER_DEL", iterData, index) {}

    DICT_KEYS::DICT_KEYS(const std::string& dictData, const std::string& lvalue)
        : BINARY("DICT_KEYS", dictData, lvalue) {}

    DICT_VALUES::DICT_VALUES(const std::string& dictData, const std::string& lvalue)
        :BINARY("DICT_VALUES", dictData, lvalue) {}

    DICT_DEL::DICT_DEL(const std::string& dictData, const std::string& key)
        : BINARY("DICT_DEL", dictData, key) {}

    TERNARY::TERNARY(const std::string &op, const std::string &rvalue1, const std::string &rvalue2,
                     const std::string &lvalue)
            : opRI(op), rvalue1(rvalue1), rvalue2(rvalue2), lvalue(lvalue){}

    std::string TERNARY::toRACode() const {
        return opRI + ": " + rvalue1 + ", " + rvalue2 + ", " + lvalue + "\n";
    }

    ADD::ADD(const std::string &rvalue1, const std::string &rvalue2,
        const std::string &lvalue)
            : TERNARY("ADD", rvalue1, rvalue2, lvalue) {}

    MUL::MUL(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
        : TERNARY("MUL", rvalue1, rvalue2, lvalue) {}

    DIV::DIV(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
        : TERNARY("DIV", rvalue1, rvalue2, lvalue) {}

    MOD::MOD(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
        : TERNARY("MOD", rvalue1, rvalue2, lvalue) {}

    POW::POW(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
        : TERNARY("POW", rvalue1, rvalue2, lvalue) {}

    ROOT::ROOT(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
        : TERNARY("ROOT", rvalue1, rvalue2, lvalue) {}

    CMP::CMP(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
        : TERNARY("CMP", rvalue1, rvalue2, lvalue) {}

    CREL::CREL(const std::string &rvalue1, const std::string &rvalue2, const std::string &lvalue)
            : TERNARY("CREL", rvalue1, rvalue2, lvalue){}

    JR::JR(const std::string &cmpData, const std::string &rel, const std::string &lvalue)
        : TERNARY("JR", cmpData, rel, lvalue) {}

    PAIR_SET::PAIR_SET(const std::string& key, const std::string& value, const std::string& lvalue)
        : TERNARY("PAIR_SET", key, value, lvalue) {}

    ITER_GET::ITER_GET(const std::string& ident, const std::string& index, const std::string& target)
        : TERNARY("ITER_GET", ident, index, target) {}

    TP_ADD_TP_FIELD::TP_ADD_TP_FIELD(const std::string& tp, const std::string& field, const std::string& value)
        : TERNARY("TP_ADD_TP_FIELD", tp, field, value) {}

    TP_ADD_INST_FIELD::TP_ADD_INST_FIELD(const std::string& tp, const std::string& field, const std::string& value)
        : TERNARY("TP_ADD_INST_FIELD", tp, field, value) {}

    TP_GET_FIELD::TP_GET_FIELD(const std::string& tp, const std::string& field, const std::string& lvalue)
        : TERNARY("TP_GET_FIELD", tp, field, lvalue) {}

    TP_SET_FIELD::TP_SET_FIELD(const std::string& tp, const std::string& field, const std::string& value)
        : TERNARY("TP_SET_FIELD", tp, field, value) {}
}
