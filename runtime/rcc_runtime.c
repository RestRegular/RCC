/**
 * RCC Runtime Library Implementation
 * ====================================
 * 提供 LLVM IR 生成器所需的运行时函数实现。
 * 所有值使用 void* (ptr) 作为统一类型。
 *
 * 简化实现：使用简单的 C 数据结构作为运行时表示。
 * 未来可以替换为更高效的实现。
 */

#include "rcc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================================
// 内部数据结构定义
// ============================================================================

typedef enum {
    RCC_TYPE_NULL,
    RCC_TYPE_INT,
    RCC_TYPE_FLOAT,
    RCC_TYPE_BOOL,
    RCC_TYPE_STRING,
    RCC_TYPE_LIST,
    RCC_TYPE_DICT,
    RCC_TYPE_PAIR,
    RCC_TYPE_FUNCTION,
    RCC_TYPE_UNKNOWN
} RCCValueType;

typedef struct {
    RCCValueType type;
    union {
        int64_t intVal;
        double floatVal;
        int boolVal;
        char* strVal;
        struct {
            void** data;
            int64_t size;
            int64_t capacity;
        } list;
        struct {
            void** keys;
            void** values;
            int64_t size;
            int64_t capacity;
        } dict;
        struct {
            void* key;
            void* value;
        } pair;
    };
} RCCValue;

// 类型标签字符串
static const char* TYPE_NAMES[] = {
    "null", "int", "float", "bool", "string",
    "list", "dict", "pair", "function", "unknown"
};

// ============================================================================
// 辅助函数
// ============================================================================

static RCCValue* rcc_value_create(RCCValueType type)
{
    RCCValue* val = (RCCValue*)calloc(1, sizeof(RCCValue));
    if (val) val->type = type;
    return val;
}

static void* rcc_int_to_ptr(int64_t i)
{
    return (void*)(intptr_t)i;
}

static int64_t rcc_ptr_to_int(void* p)
{
    return (int64_t)(intptr_t)p;
}

// ============================================================================
// 数据结构运行时
// ============================================================================

void* __rcc_pair_create(void* key, void* value)
{
    RCCValue* pair = rcc_value_create(RCC_TYPE_PAIR);
    if (!pair) return NULL;
    pair->pair.key = key;
    pair->pair.value = value;
    return pair;
}

void* __rcc_dict_create(int64_t count, ...)
{
    RCCValue* dict = rcc_value_create(RCC_TYPE_DICT);
    if (!dict) return NULL;

    if (count > 0)
    {
        dict->dict.capacity = count * 2;
        dict->dict.keys = (void**)calloc(dict->dict.capacity, sizeof(void*));
        dict->dict.values = (void**)calloc(dict->dict.capacity, sizeof(void*));
        if (!dict->dict.keys || !dict->dict.values)
        {
            free(dict->dict.keys);
            free(dict->dict.values);
            free(dict);
            return NULL;
        }

        va_list args;
        va_start(args, count);
        for (int64_t i = 0; i < count; i++)
        {
            void* key = va_arg(args, void*);
            void* val = va_arg(args, void*);
            if (dict->dict.size < dict->dict.capacity)
            {
                dict->dict.keys[dict->dict.size] = key;
                dict->dict.values[dict->dict.size] = val;
                dict->dict.size++;
            }
        }
        va_end(args);
    }

    return dict;
}

void* __rcc_list_create(int64_t count, ...)
{
    RCCValue* list = rcc_value_create(RCC_TYPE_LIST);
    if (!list) return NULL;

    if (count > 0)
    {
        list->list.capacity = count * 2;
        list->list.data = (void**)calloc(list->list.capacity, sizeof(void*));
        if (!list->list.data)
        {
            free(list);
            return NULL;
        }

        va_list args;
        va_start(args, count);
        for (int64_t i = 0; i < count; i++)
        {
            void* elem = va_arg(args, void*);
            if (list->list.size < list->list.capacity)
            {
                list->list.data[list->list.size] = elem;
                list->list.size++;
            }
        }
        va_end(args);
    }

    return list;
}

void* __rcc_index_get(void* target, void* index)
{
    if (!target) return NULL;

    RCCValue* val = (RCCValue*)target;

    switch (val->type)
    {
    case RCC_TYPE_LIST:
    {
        int64_t idx = rcc_ptr_to_int(index);
        if (idx >= 0 && idx < val->list.size)
            return val->list.data[idx];
        // 支持负索引
        if (idx < 0 && -idx <= val->list.size)
            return val->list.data[val->list.size + idx];
        return NULL;
    }
    case RCC_TYPE_DICT:
    {
        // 线性查找 key
        for (int64_t i = 0; i < val->dict.size; i++)
        {
            if (val->dict.keys[i] == index)
                return val->dict.values[i];
        }
        return NULL;
    }
    case RCC_TYPE_STRING:
    {
        // 字符串索引返回单字符字符串
        int64_t idx = rcc_ptr_to_int(index);
        const char* str = val->strVal;
        int64_t len = (int64_t)strlen(str);
        if (idx >= 0 && idx < len)
        {
            char* ch = (char*)calloc(2, sizeof(char));
            ch[0] = str[idx];
            ch[1] = '\0';
            RCCValue* result = rcc_value_create(RCC_TYPE_STRING);
            result->strVal = ch;
            return result;
        }
        return NULL;
    }
    default:
        return NULL;
    }
}

int64_t __rcc_size(void* obj)
{
    if (!obj) return 0;

    RCCValue* val = (RCCValue*)obj;

    switch (val->type)
    {
    case RCC_TYPE_LIST:
        return val->list.size;
    case RCC_TYPE_DICT:
        return val->dict.size;
    case RCC_TYPE_STRING:
        return val->strVal ? (int64_t)strlen(val->strVal) : 0;
    default:
        return 0;
    }
}

void* __rcc_copy(void* obj)
{
    if (!obj) return NULL;

    RCCValue* src = (RCCValue*)obj;
    RCCValue* dst = rcc_value_create(src->type);
    if (!dst) return NULL;

    switch (src->type)
    {
    case RCC_TYPE_LIST:
        dst->list.size = src->list.size;
        dst->list.capacity = src->list.capacity;
        dst->list.data = (void**)calloc(dst->list.capacity, sizeof(void*));
        if (dst->list.data)
            memcpy(dst->list.data, src->list.data, dst->list.size * sizeof(void*));
        break;
    case RCC_TYPE_DICT:
        dst->dict.size = src->dict.size;
        dst->dict.capacity = src->dict.capacity;
        dst->dict.keys = (void**)calloc(dst->dict.capacity, sizeof(void*));
        dst->dict.values = (void**)calloc(dst->dict.capacity, sizeof(void*));
        if (dst->dict.keys && dst->dict.values)
        {
            memcpy(dst->dict.keys, src->dict.keys, dst->dict.size * sizeof(void*));
            memcpy(dst->dict.values, src->dict.values, dst->dict.size * sizeof(void*));
        }
        break;
    case RCC_TYPE_STRING:
        if (src->strVal)
            dst->strVal = strdup(src->strVal);
        break;
    case RCC_TYPE_INT:
        dst->intVal = src->intVal;
        break;
    case RCC_TYPE_FLOAT:
        dst->floatVal = src->floatVal;
        break;
    case RCC_TYPE_BOOL:
        dst->boolVal = src->boolVal;
        break;
    default:
        break;
    }

    return dst;
}

void __rcc_list_append(void* list, void* elem)
{
    if (!list) return;

    RCCValue* val = (RCCValue*)list;
    if (val->type != RCC_TYPE_LIST) return;

    // 扩容
    if (val->list.size >= val->list.capacity)
    {
        int64_t newCap = val->list.capacity == 0 ? 8 : val->list.capacity * 2;
        void** newData = (void**)realloc(val->list.data, newCap * sizeof(void*));
        if (!newData) return;
        val->list.data = newData;
        val->list.capacity = newCap;
    }

    val->list.data[val->list.size] = elem;
    val->list.size++;
}

void __rcc_list_remove(void* list, void* index)
{
    if (!list) return;

    RCCValue* val = (RCCValue*)list;
    if (val->type != RCC_TYPE_LIST) return;

    int64_t idx = rcc_ptr_to_int(index);
    if (idx < 0) idx = val->list.size + idx;
    if (idx < 0 || idx >= val->list.size) return;

    // 移动元素
    for (int64_t i = idx; i < val->list.size - 1; i++)
    {
        val->list.data[i] = val->list.data[i + 1];
    }
    val->list.size--;
}

void __rcc_dict_remove(void* dict, void* key)
{
    if (!dict) return;

    RCCValue* val = (RCCValue*)dict;
    if (val->type != RCC_TYPE_DICT) return;

    for (int64_t i = 0; i < val->dict.size; i++)
    {
        if (val->dict.keys[i] == key)
        {
            // 移动元素
            for (int64_t j = i; j < val->dict.size - 1; j++)
            {
                val->dict.keys[j] = val->dict.keys[j + 1];
                val->dict.values[j] = val->dict.values[j + 1];
            }
            val->dict.size--;
            return;
        }
    }
}

void* __rcc_dict_keys(void* dict)
{
    if (!dict) return __rcc_list_create(0);

    RCCValue* val = (RCCValue*)dict;
    if (val->type != RCC_TYPE_DICT) return __rcc_list_create(0);

    RCCValue* list = rcc_value_create(RCC_TYPE_LIST);
    if (!list) return NULL;

    list->list.size = val->dict.size;
    list->list.capacity = val->dict.size;
    list->list.data = (void**)calloc(list->list.capacity, sizeof(void*));
    if (list->list.data)
    {
        memcpy(list->list.data, val->dict.keys, list->list.size * sizeof(void*));
    }

    return list;
}

void* __rcc_dict_values(void* dict)
{
    if (!dict) return __rcc_list_create(0);

    RCCValue* val = (RCCValue*)dict;
    if (val->type != RCC_TYPE_DICT) return __rcc_list_create(0);

    RCCValue* list = rcc_value_create(RCC_TYPE_LIST);
    if (!list) return NULL;

    list->list.size = val->dict.size;
    list->list.capacity = val->dict.size;
    list->list.data = (void**)calloc(list->list.capacity, sizeof(void*));
    if (list->list.data)
    {
        memcpy(list->list.data, val->dict.values, list->list.size * sizeof(void*));
    }

    return list;
}

// ============================================================================
// 数据类型运行时
// ============================================================================

void* __rcc_type(void* obj)
{
    if (!obj) return (void*)"null";

    RCCValue* val = (RCCValue*)obj;
    const char* typeName = TYPE_NAMES[val->type];
    return (void*)typeName;
}

int __rcc_check_type(void* obj, void* typeName)
{
    if (!obj || !typeName) return 0;

    RCCValue* val = (RCCValue*)obj;
    const char* expected = (const char*)typeName;
    const char* actual = TYPE_NAMES[val->type];

    return strcmp(actual, expected) == 0 ? 1 : 0;
}

// ============================================================================
// 异常处理运行时（简化实现）
// ============================================================================

void* __rcc_try_begin(void)
{
    // 简化实现：返回一个空的 context
    return NULL;
}

int __rcc_try_end(void* context)
{
    // 简化实现：始终返回 0（无异常）
    (void)context;
    return 0;
}

void __rcc_try_catch(void* context, void* exception)
{
    // 简化实现：无操作
    (void)context;
    (void)exception;
}

void __rcc_try_finally(void* context)
{
    // 简化实现：无操作
    (void)context;
}

void __rcc_throw(void* exception)
{
    // 简化实现：打印错误信息并退出
    const char* msg = (const char*)exception;
    if (msg)
        fprintf(stderr, "Runtime error: %s\n", msg);
    else
        fprintf(stderr, "Runtime error: <unknown exception>\n");
    exit(1);
}

// ============================================================================
// 程序控制运行时
// ============================================================================

void* __rcc_repeat(void* count, void* handler)
{
    // 简化实现：count 是 ptr 编码的整数，handler 是函数指针
    // 由于动态类型系统尚未完全实现，此处为占位
    (void)count;
    (void)handler;
    return NULL;
}
