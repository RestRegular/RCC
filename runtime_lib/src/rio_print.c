#include "rio_runtime.h"
#include <stdio.h>

// ==================== 打印整数 ====================
void rio_print_int(int64_t value) {
    printf("%lld", (long long)value);
}

// ==================== 打印浮点数 ====================
// 使用 %g 格式，自动去掉末尾多余的 0
void rio_print_float(double value) {
    printf("%g", value);
}

// ==================== 打印布尔值 ====================
void rio_print_bool(int32_t value) {
    if (value) {
        printf("true");
    } else {
        printf("false");
    }
}

// ==================== 打印字符串 ====================
void rio_print_string(RioString* s) {
    if (s == NULL) {
        printf("null");
        return;
    }
    printf("%s", s->data);
}

// ==================== 打印对象 ====================
// 根据 type_id 分发到对应的打印函数
void rio_print_object(RioObject* obj) {
    if (obj == NULL) {
        printf("null");
        return;
    }

    switch (obj->type_id) {
        case RIO_TYPE_NULL:
            printf("null");
            break;

        case RIO_TYPE_INT:
            // int 值存储在 obj_size 中（简单方案）
            // 实际使用中，int 通常直接作为 i64 传递，不走对象
            printf("<int object>");
            break;

        case RIO_TYPE_FLOAT:
            printf("<float object>");
            break;

        case RIO_TYPE_BOOL:
            printf("<bool object>");
            break;

        case RIO_TYPE_CHAR:
            printf("<char object>");
            break;

        case RIO_TYPE_STRING: {
            RioString* s = (RioString*)obj;
            rio_print_string(s);
            break;
        }

        case RIO_TYPE_LIST: {
            RioList* list = (RioList*)obj;
            printf("[");
            for (int64_t i = 0; i < list->length; i++) {
                if (i > 0) {
                    printf(", ");
                }
                // Elements may be boxed integers (stored as pointer-sized values)
                // or actual RioObject pointers. Check if the pointer looks valid.
                RioObject* elem = list->elements[i];
                if (elem == NULL) {
                    printf("null");
                } else if ((uintptr_t)elem < 0x1000) {
                    // Small pointer values are likely boxed integers
                    printf("%lld", (int64_t)(uintptr_t)elem);
                } else {
                    rio_print_object(elem);
                }
            }
            printf("]");
            break;
        }

        case RIO_TYPE_DICT: {
            RioDict* dict = (RioDict*)obj;
            printf("{");
            bool first = true;
            for (int64_t i = 0; i < dict->bucket_count; i++) {
                RioDictEntry* entry = dict->buckets[i];
                while (entry != NULL) {
                    if (!first) {
                        printf(", ");
                    }
                    first = false;
                    rio_print_object(entry->key);
                    printf(": ");
                    rio_print_object(entry->value);
                    entry = entry->next;
                }
            }
            printf("}");
            break;
        }

        case RIO_TYPE_FUNCTION:
            printf("<function>");
            break;

        case RIO_TYPE_CLOSURE:
            printf("<closure>");
            break;

        case RIO_TYPE_INSTANCE:
            printf("<instance>");
            break;

        default:
            printf("<unknown object>");
            break;
    }
}

// ==================== 打印换行 ====================
void rio_print_newline(void) {
    printf("\n");
}
