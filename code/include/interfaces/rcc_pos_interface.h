//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_POS_INTERFACE_H
#define RCC_RCC_POS_INTERFACE_H

#include "rcc_interface_dec.h"

namespace utils
{
    class Pos;

    class IRCCPosInterface:
    virtual public IRCCObjectInterface
    {
    public:
        ~IRCCPosInterface() override SYS_INTERFACE_CALL = 0;
        virtual std::size_t SYS_INTERFACE_CALL GetLine() const = 0;
        virtual std::size_t SYS_INTERFACE_CALL GetColumn() const = 0;
        virtual std::size_t SYS_INTERFACE_CALL GetOffset() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetFilepath() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetFileField() const = 0;
        virtual const char* SYS_INTERFACE_CALL GetFilePosStr() const = 0;
        virtual void SYS_INTERFACE_CALL SetLine(const std::size_t &line_) = 0;
        virtual void SYS_INTERFACE_CALL SetColumn(const std::size_t &column_) = 0;
        virtual void SYS_INTERFACE_CALL SetOffSet(const std::size_t &offset_) = 0;
        virtual void SYS_INTERFACE_CALL SetFilepath(const char* filepath_) = 0;
        virtual bool SYS_INTERFACE_CALL Compare(const IRCCPosInterface *other) = 0;
        virtual const Pos* SYS_INTERFACE_CALL TransformToPI() const = 0;
    };
}

#endif //RCC_RCC_POS_INTERFACE_H