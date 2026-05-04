#include "rio_runtime.h"
#include <string.h>

// ==================== 对象类型名称 ====================
const char* rio_object_type_name(RioObject* obj) {
    if (obj == NULL) {
        return "null";
    }
    switch (obj->type_id) {
        case RIO_TYPE_NULL:     return "null";
        case RIO_TYPE_INT:      return "int";
        case RIO_TYPE_FLOAT:    return "float";
        case RIO_TYPE_BOOL:     return "bool";
        case RIO_TYPE_CHAR:     return "char";
        case RIO_TYPE_STRING:   return "string";
        case RIO_TYPE_LIST:     return "list";
        case RIO_TYPE_DICT:     return "dict";
        case RIO_TYPE_FUNCTION: return "function";
        case RIO_TYPE_CLOSURE:  return "closure";
        case RIO_TYPE_INSTANCE: return "instance";
        case RIO_TYPE_ANY:      return "any";
        default:                return "unknown";
    }
}

// ==================== 对象真值判断 ====================
// null 和 false 为假值，其余均为真值
bool rio_object_is_truthy(RioObject* obj) {
    if (obj == NULL) {
        return false;
    }
    switch (obj->type_id) {
        case RIO_TYPE_NULL:
            return false;
        case RIO_TYPE_BOOL:
            // bool 对象的 gc_mark 字段暂用来存储布尔值（0=false, 1=true）
            // 这里通过检查 obj_size 来判断：false 的 obj_size 设为 0，true 设为 1
            return (obj->obj_size == 1);
        default:
            return true;
    }
}
