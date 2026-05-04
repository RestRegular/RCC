#ifndef RIO_RUNTIME_H
#define RIO_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 类型ID定义 ====================
#define RIO_TYPE_NULL     0
#define RIO_TYPE_INT      1
#define RIO_TYPE_FLOAT    2
#define RIO_TYPE_BOOL     3
#define RIO_TYPE_CHAR     4
#define RIO_TYPE_STRING   5
#define RIO_TYPE_LIST     6
#define RIO_TYPE_DICT     7
#define RIO_TYPE_FUNCTION 8
#define RIO_TYPE_CLOSURE  9
#define RIO_TYPE_INSTANCE 10
#define RIO_TYPE_ANY      11

// ==================== 对象头 ====================
typedef struct RioObject {
    uint32_t type_id;       // 类型ID
    uint32_t gc_mark;       // GC标记位
    uint32_t obj_size;      // 对象大小（含头部）
    struct RioObject* gc_next; // GC链表指针
} RioObject;

// ==================== 字符串对象 ====================
typedef struct RioString {
    RioObject header;
    int64_t length;
    int64_t hash_code;
    char data[];  // 柔性数组，UTF-8编码，以\0结尾
} RioString;

// ==================== 列表对象 ====================
typedef struct RioList {
    RioObject header;
    int64_t length;
    int64_t capacity;
    RioObject** elements;  // 元素指针数组
} RioList;

// ==================== 字典对象 ====================
typedef struct RioDictEntry {
    RioObject* key;
    RioObject* value;
    struct RioDictEntry* next; // 哈希冲突链表
} RioDictEntry;

typedef struct RioDict {
    RioObject header;
    int64_t bucket_count;
    int64_t entry_count;
    RioDictEntry** buckets;  // 哈希桶数组
} RioDict;

// ==================== GC 函数 ====================
void rio_gc_init(size_t heap_size);
void rio_gc_shutdown(void);
RioObject* rio_gc_alloc(size_t size, uint32_t type_id);
void rio_gc_collect(void);
void rio_gc_write_barrier(RioObject* container, RioObject* value);

// ==================== 字符串函数 ====================
RioString* rio_string_new(const char* cstr, int64_t len);
RioString* rio_string_concat(RioString* a, RioString* b);
const char* rio_string_cstr(RioString* s);
int64_t rio_string_length(RioString* s);
int64_t rio_string_hash(RioString* s);
bool rio_string_equals(RioString* a, RioString* b);

// ==================== 列表函数 ====================
RioList* rio_list_new(int64_t initial_capacity);
void rio_list_append(RioList* list, RioObject* item);
RioObject* rio_list_get(RioList* list, int64_t index);
void rio_list_set(RioList* list, int64_t index, RioObject* value);
int64_t rio_list_length(RioList* list);

// ==================== 打印函数 ====================
void rio_print_int(int64_t value);
void rio_print_float(double value);
void rio_print_bool(int32_t value);
void rio_print_string(RioString* s);
void rio_print_object(RioObject* obj);
void rio_print_newline(void);

// ==================== 数学函数 ====================
double rio_math_pow(double base, double exp);
int64_t rio_math_abs(int64_t value);
int64_t rio_math_max(int64_t a, int64_t b);
int64_t rio_math_min(int64_t a, int64_t b);

// ==================== 运行时初始化 ====================
void rio_runtime_init(void);
void rio_runtime_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // RIO_RUNTIME_H
