//
// Created by RestRegular on 2025/10/22.
//

#ifndef RCC_RJSON_DEC_H
#define RCC_RJSON_DEC_H
#include <string>

namespace rjson
{

    enum class RJType;
    class RJNull;
    class RJKey;
    class RJValue;
    class RJPair;
    class RJList;
    class RJObject;
    class RJsonParser;

    std::string getRJTypeName(RJType type);
}

#endif //RCC_RJSON_DEC_H