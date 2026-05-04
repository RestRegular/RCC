#include "rio_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ==================== 运行时初始化 ====================
void rio_runtime_init(void) {
    rio_gc_init(0); // 使用默认堆大小 16MB
}

// ==================== 运行时关闭 ====================
void rio_runtime_shutdown(void) {
    rio_gc_shutdown();
}

// ==================== 数学函数 ====================

// 幂运算
double rio_math_pow(double base, double exp) {
    return pow(base, exp);
}

// 绝对值
int64_t rio_math_abs(int64_t value) {
    return llabs(value);
}

// 最大值
int64_t rio_math_max(int64_t a, int64_t b) {
    return (a > b) ? a : b;
}

// 最小值
int64_t rio_math_min(int64_t a, int64_t b) {
    return (a < b) ? a : b;
}

// ==================== 列表函数 ====================

// 创建新列表
RioList* rio_list_new(int64_t initial_capacity) {
    if (initial_capacity < 4) initial_capacity = 4;

    // 通过 GC 分配列表对象
    RioList* list = (RioList*)rio_gc_alloc(sizeof(RioList), RIO_TYPE_LIST);
    if (!list) return NULL;

    list->length = 0;
    list->capacity = initial_capacity;
    list->elements = (RioObject**)calloc(initial_capacity, sizeof(RioObject*));
    if (!list->elements) {
        list->length = 0;
        list->capacity = 0;
    }
    return list;
}

// 向列表追加元素
void rio_list_append(RioList* list, RioObject* item) {
    if (!list) return;

    // 扩容
    if (list->length >= list->capacity) {
        int64_t new_capacity = list->capacity * 2;
        RioObject** new_elements = (RioObject**)realloc(list->elements, new_capacity * sizeof(RioObject*));
        if (new_elements) {
            list->elements = new_elements;
            list->capacity = new_capacity;
        } else {
            return; // 分配失败
        }
    }

    list->elements[list->length] = item;
    list->length++;
}

// 获取列表元素
RioObject* rio_list_get(RioList* list, int64_t index) {
    if (!list || index < 0 || index >= list->length) {
        return (RioObject*)NULL;
    }
    return list->elements[index];
}

// 设置列表元素
void rio_list_set(RioList* list, int64_t index, RioObject* value) {
    if (!list || index < 0 || index >= list->length) {
        return;
    }
    list->elements[index] = value;
}

// 获取列表长度
int64_t rio_list_length(RioList* list) {
    if (!list) return 0;
    return list->length;
}
