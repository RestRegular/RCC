//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RCC_OBJECT_INTERFACE_H
#define RCC_RCC_OBJECT_INTERFACE_H

#include <cstddef>

#include "rcc_interface_dec.h"

namespace utils
{
    class Object;

    class IRCCObjectInterface
    {
    public:
        virtual SYS_INTERFACE_CALL ~IRCCObjectInterface() = 0;

        virtual const char* SYS_INTERFACE_CALL ToString() const = 0;
        virtual const char* SYS_INTERFACE_CALL BriefString() const = 0;
        virtual const char* SYS_INTERFACE_CALL ProfessionalString() const = 0;
        virtual const char* SYS_INTERFACE_CALL FormatString(const std::size_t &indent, const std::size_t &level) const = 0;
        virtual const char* SYS_INTERFACE_CALL ToJsonString() const = 0;
        virtual const char* SYS_INTERFACE_CALL HasCode() = 0;
        virtual const char* SYS_INTERFACE_CALL UniqueId() = 0;
        virtual const IRCCObjectInterface* SYS_INTERFACE_CALL CopySelf() = 0;
        virtual const Object* SYS_INTERFACE_CALL Transform() const = 0;
    };
}

#endif //RCC_RCC_OBJECT_INTERFACE_H