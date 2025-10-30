//
// Created by RestRegular on 2025/10/24.
//

#ifndef RCC_RCC_BUILTIN_INTERFACE_H
#define RCC_RCC_BUILTIN_INTERFACE_H

#include <cstddef>

#include "rcc_interface_dec.h"

namespace builtin
{
    struct IRCCStringArray
    {
        char** stringArray;
        std::size_t size;
    };

    struct IRCCStringPair
    {
        char* first;
        char* second;
    };

    struct IRCCOpItemArray
    {
        ast::IRCCOpItemInterface** opItemIArray;
        size_t size;
    };

    struct IRCCStringOpItemPair
    {
        char* name;
        ast::IRCCOpItemInterface* opItemI;
    };

    struct IRCCStringOpItemMap
    {
        IRCCStringOpItemPair* nameOpItemPair;
        size_t size;
    };

    struct IRCCCallInfos
    {
        IRCCStringArray processedArgs;
        struct
        {
            IRCCStringPair* pairs;
            size_t size;
        } originalArgs;
        IRCCOpItemArray posArgOpItems;
        IRCCStringOpItemMap namedArgOpItems;
        utils::IRCCPosInterface* callPosI;
        IRCCStringOpItemMap orderedArgOpItems;
    };
}

#endif //RCC_RCC_BUILTIN_INTERFACE_H