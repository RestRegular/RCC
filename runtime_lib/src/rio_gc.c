#include "rio_runtime.h"
#include <stdlib.h>
#include <string.h>

// ==================== GC 全局状态 ====================
static struct {
    RioObject* head;           // 对象链表头
    size_t total_allocated;    // 总分配大小
    size_t heap_threshold;     // 堆大小阈值
    bool collecting;           // 是否正在回收
    size_t collection_count;   // 回收次数
} gc_state = { NULL, 0, 0, false, 0 };

// ==================== GC 初始化 ====================
void rio_gc_init(size_t heap_size) {
    gc_state.head = NULL;
    gc_state.total_allocated = 0;
    gc_state.heap_threshold = (heap_size > 0) ? heap_size : (16 * 1024 * 1024); // 默认16MB
    gc_state.collecting = false;
    gc_state.collection_count = 0;
}

// ==================== GC 分配 ====================
// 分配对象内存，注册到 GC 链表，当已分配大小超过阈值时触发回收
RioObject* rio_gc_alloc(size_t size, uint32_t type_id) {
    // 如果不在回收中且超过阈值，先触发一次GC
    if (!gc_state.collecting && gc_state.total_allocated + size > gc_state.heap_threshold) {
        rio_gc_collect();
    }

    // 使用 malloc 分配内存
    RioObject* obj = (RioObject*)malloc(size);
    if (obj == NULL) {
        return NULL;
    }

    // 初始化对象头
    obj->type_id = type_id;
    obj->gc_mark = 0;
    obj->obj_size = (uint32_t)size;
    obj->gc_next = gc_state.head;

    // 注册到 GC 链表头部
    gc_state.head = obj;
    gc_state.total_allocated += size;

    return obj;
}

// ==================== GC 标记阶段 ====================
// 简单遍历：当前实现标记所有可达对象
// 注意：完整的GC需要从根集合开始遍历，这里提供基础框架
static void rio_gc_mark(RioObject* obj) {
    if (obj == NULL || obj->gc_mark != 0) {
        return;
    }

    obj->gc_mark = 1;

    // 根据对象类型递归标记子对象
    switch (obj->type_id) {
        case RIO_TYPE_LIST: {
            RioList* list = (RioList*)obj;
            for (int64_t i = 0; i < list->length; i++) {
                rio_gc_mark(list->elements[i]);
            }
            break;
        }
        case RIO_TYPE_DICT: {
            RioDict* dict = (RioDict*)obj;
            for (int64_t i = 0; i < dict->bucket_count; i++) {
                RioDictEntry* entry = dict->buckets[i];
                while (entry != NULL) {
                    rio_gc_mark(entry->key);
                    rio_gc_mark(entry->value);
                    entry = entry->next;
                }
            }
            break;
        }
        // 其他类型没有子对象引用，无需递归
        default:
            break;
    }
}

// ==================== GC 清除阶段 ====================
// 释放所有未标记的对象，重置已标记对象的标记位
static void rio_gc_sweep(void) {
    RioObject** current = &gc_state.head;
    while (*current != NULL) {
        RioObject* obj = *current;
        if (obj->gc_mark == 0) {
            // 未标记：从链表中移除并释放
            *current = obj->gc_next;
            gc_state.total_allocated -= obj->obj_size;

            // 释放列表的元素数组
            if (obj->type_id == RIO_TYPE_LIST) {
                RioList* list = (RioList*)obj;
                free(list->elements);
            }

            // 释放字典的桶数组和条目
            if (obj->type_id == RIO_TYPE_DICT) {
                RioDict* dict = (RioDict*)obj;
                for (int64_t i = 0; i < dict->bucket_count; i++) {
                    RioDictEntry* entry = dict->buckets[i];
                    while (entry != NULL) {
                        RioDictEntry* next_entry = entry->next;
                        free(entry);
                        entry = next_entry;
                    }
                }
                free(dict->buckets);
            }

            free(obj);
        } else {
            // 已标记：重置标记位，继续遍历
            obj->gc_mark = 0;
            current = &obj->gc_next;
        }
    }
}

// ==================== GC 回收 ====================
void rio_gc_collect(void) {
    if (gc_state.collecting) {
        return; // 防止递归回收
    }

    gc_state.collecting = true;
    gc_state.collection_count++;

    // 标记阶段：从根集合标记可达对象
    // 当前实现遍历整个链表（保守式标记）
    // 完整实现应从栈、寄存器等根集合开始
    RioObject* obj = gc_state.head;
    while (obj != NULL) {
        rio_gc_mark(obj);
        obj = obj->gc_next;
    }

    // 清除阶段
    rio_gc_sweep();

    // 回收后动态调整阈值：如果释放后仍超过阈值的75%，扩大阈值
    if (gc_state.total_allocated > gc_state.heap_threshold * 3 / 4) {
        gc_state.heap_threshold *= 2;
    }

    gc_state.collecting = false;
}

// ==================== GC 关闭 ====================
void rio_gc_shutdown(void) {
    // 释放所有剩余对象
    RioObject* current = gc_state.head;
    while (current != NULL) {
        RioObject* next = current->gc_next;

        // 释放列表的元素数组
        if (current->type_id == RIO_TYPE_LIST) {
            RioList* list = (RioList*)current;
            free(list->elements);
        }

        // 释放字典的桶数组和条目
        if (current->type_id == RIO_TYPE_DICT) {
            RioDict* dict = (RioDict*)current;
            for (int64_t i = 0; i < dict->bucket_count; i++) {
                RioDictEntry* entry = dict->buckets[i];
                while (entry != NULL) {
                    RioDictEntry* next_entry = entry->next;
                    free(entry);
                    entry = next_entry;
                }
            }
            free(dict->buckets);
        }

        free(current);
        current = next;
    }

    // 重置 GC 状态
    gc_state.head = NULL;
    gc_state.total_allocated = 0;
    gc_state.collecting = false;
    gc_state.collection_count = 0;
}

// ==================== 写屏障（预留接口） ====================
void rio_gc_write_barrier(RioObject* container, RioObject* value) {
    // 当前为空实现，预留用于未来增量/分代 GC
    (void)container;
    (void)value;
}
