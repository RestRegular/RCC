/**
 * RCC Runtime Library
 * ====================
 * 提供 LLVM IR 生成器所需的运行时函数实现。
 * 所有值使用 void* (ptr) 作为统一类型。
 *
 * 编译: clang -c rcc_runtime.c -o rcc_runtime.o
 * 使用: lli -extra-archive=rcc_runtime.o test.ll
 */

#ifndef RCC_RUNTIME_H
#define RCC_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

// ==================== 数据结构运行时 ====================

/**
 * 创建键值对
 * @param key   键
 * @param value 值
 * @return 键值对指针
 */
void* __rcc_pair_create(void* key, void* value);

/**
 * 创建字典
 * @param count 键值对数量
 * @param ...   交替的 key, value 参数
 * @return 字典指针
 */
void* __rcc_dict_create(int64_t count, ...);

/**
 * 创建列表
 * @param count 元素数量
 * @param ...   元素值
 * @return 列表指针
 */
void* __rcc_list_create(int64_t count, ...);

/**
 * 索引访问
 * @param target 目标对象（列表/字典/字符串）
 * @param index  索引/键
 * @return 对应的值
 */
void* __rcc_index_get(void* target, void* index);

/**
 * 获取大小
 * @param obj 可迭代对象
 * @return 大小（编码为 ptr）
 */
int64_t __rcc_size(void* obj);

/**
 * 浅拷贝
 * @param obj 源对象
 * @return 拷贝后的对象
 */
void* __rcc_copy(void* obj);

/**
 * 列表追加元素
 * @param list 列表
 * @param elem 元素
 */
void __rcc_list_append(void* list, void* elem);

/**
 * 列表按索引删除元素
 * @param list 列表
 * @param index 索引
 */
void __rcc_list_remove(void* list, void* index);

/**
 * 字典按键删除元素
 * @param dict 字典
 * @param key  键
 */
void __rcc_dict_remove(void* dict, void* key);

/**
 * 获取字典所有键
 * @param dict 字典
 * @return 键列表
 */
void* __rcc_dict_keys(void* dict);

/**
 * 获取字典所有值
 * @param dict 字典
 * @return 值列表
 */
void* __rcc_dict_values(void* dict);

// ==================== 数据类型运行时 ====================

/**
 * 获取运行时类型名
 * @param obj 对象
 * @return 类型名字符串
 */
void* __rcc_type(void* obj);

/**
 * 类型检查
 * @param obj      对象
 * @param typeName 类型名字符串
 * @return 1 表示匹配，0 表示不匹配
 */
int __rcc_check_type(void* obj, void* typeName);

// ==================== 异常处理运行时 ====================

/**
 * 开始 try 块
 * @return try context token
 */
void* __rcc_try_begin(void);

/**
 * 结束 try 块
 * @param context try context
 * @return 0 表示无异常，非 0 表示有异常
 */
int __rcc_try_end(void* context);

/**
 * 注册 catch handler
 * @param context   try context
 * @param exception 异常值
 */
void __rcc_try_catch(void* context, void* exception);

/**
 * 执行 finally 块
 * @param context try context
 */
void __rcc_try_finally(void* context);

/**
 * 抛出异常
 * @param exception 异常值
 */
void __rcc_throw(void* exception);

// ==================== 程序控制运行时 ====================

/**
 * 重复执行
 * @param count   重复次数
 * @param handler 处理器函数
 * @return 结果
 */
void* __rcc_repeat(void* count, void* handler);

#endif // RCC_RUNTIME_H
