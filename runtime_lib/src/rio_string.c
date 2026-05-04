#include "rio_runtime.h"
#include <stdlib.h>
#include <string.h>

// ==================== 创建字符串对象 ====================
// 通过 GC 分配创建字符串对象，复制 C 字符串
RioString* rio_string_new(const char* cstr, int64_t len) {
    if (cstr == NULL) {
        len = 0;
    }
    if (len < 0) {
        len = (int64_t)strlen(cstr);
    }

    // 计算总大小：RioString 头部 + 字符串数据 + \0 终止符
    size_t total_size = sizeof(RioString) + (size_t)len + 1;

    // 通过 GC 分配
    RioString* s = (RioString*)rio_gc_alloc(total_size, RIO_TYPE_STRING);
    if (s == NULL) {
        return NULL;
    }

    s->length = len;
    s->hash_code = 0; // 延迟计算哈希

    // 复制字符串数据
    if (cstr != NULL && len > 0) {
        memcpy(s->data, cstr, (size_t)len);
    }
    s->data[len] = '\0'; // 确保以 \0 结尾

    return s;
}

// ==================== 连接两个字符串 ====================
RioString* rio_string_concat(RioString* a, RioString* b) {
    if (a == NULL || b == NULL) {
        return NULL;
    }

    int64_t new_len = a->length + b->length;
    size_t total_size = sizeof(RioString) + (size_t)new_len + 1;

    RioString* result = (RioString*)rio_gc_alloc(total_size, RIO_TYPE_STRING);
    if (result == NULL) {
        return NULL;
    }

    result->length = new_len;
    result->hash_code = 0; // 新字符串需要重新计算哈希

    // 复制两个字符串的数据
    memcpy(result->data, a->data, (size_t)a->length);
    memcpy(result->data + a->length, b->data, (size_t)b->length);
    result->data[new_len] = '\0';

    return result;
}

// ==================== 返回 C 字符串指针 ====================
const char* rio_string_cstr(RioString* s) {
    if (s == NULL) {
        return "";
    }
    return s->data;
}

// ==================== 返回字符串长度 ====================
int64_t rio_string_length(RioString* s) {
    if (s == NULL) {
        return 0;
    }
    return s->length;
}

// ==================== FNV-1a 哈希算法 ====================
int64_t rio_string_hash(RioString* s) {
    if (s == NULL) {
        return 0;
    }

    // 如果已经计算过哈希，直接返回缓存值
    if (s->hash_code != 0) {
        return s->hash_code;
    }

    // FNV-1a 64位哈希
    const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET_BASIS;
    for (int64_t i = 0; i < s->length; i++) {
        hash ^= (unsigned char)s->data[i];
        hash *= FNV_PRIME;
    }

    // 缓存哈希值（0 表示未计算，如果哈希结果恰好为 0 则用 1 代替）
    s->hash_code = (int64_t)(hash == 0 ? 1 : hash);
    return s->hash_code;
}

// ==================== 比较两个字符串是否相等 ====================
bool rio_string_equals(RioString* a, RioString* b) {
    if (a == b) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->data, b->data, (size_t)a->length) == 0;
}
