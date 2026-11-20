/* argv_runtime_probe.c —— G1b 运行时层验证探针
 *
 * 链接 core_win64.o + rt.o，直接调用 rt_argv_init(argc, argv)，
 * 按 ABI v2 头布局解出 list[str]，验证：
 *   1) 元素个数 = argc-1（不含程序名 argv[0]）
 *   2) 每个元素的 len/cap/bytes 与传入的 C 字符串一致
 *   3) 无参时返回空 list（len=0）
 *
 * 编译（MinGW x64）：
 *   gcc -O0 -g -o argv_runtime_probe.exe argv_runtime_probe.c \
 *       release/lib/core/core_win64.o release/lib/rt/rt.o
 */
#include <stdio.h>
#include <string.h>

/* rt.rio: fun rt_argv_init(argc: int, argv: ptr): ptr */
extern void *rt_argv_init(long long argc, char **argv);

/* ABI v2 头：len@0, cap@8；str 数据从 +16 起；list 数据从 +24 起 */
typedef struct { long long len, cap; } Header;
typedef struct { long long len, cap, elem_size; void **data; } ListHeader;

int main(int argc, char **argv) {
    void *list = rt_argv_init((long long)argc, argv);
    if (!list) { printf("FAIL: rt_argv_init returned null\n"); return 1; }
    ListHeader *lh = (ListHeader *)list;
    long long want = argc - 1;
    printf("argc=%d list.len=%lld (want %lld) cap=%lld elem_size=%lld\n",
           argc, lh->len, want, lh->cap, lh->elem_size);
    int ok = (lh->len == want && lh->cap == want && lh->elem_size == 8);
    for (long long i = 0; i < lh->len; ++i) {
        Header *h = (Header *)lh->data[i];
        const char *expected = argv[i + 1];   /* +1 跳过程序名 */
        size_t n = strlen(expected);
        int good = h && h->len == (long long)n && h->cap == (long long)n &&
                   memcmp((char *)h + 16, expected, n) == 0 &&
                   *((char *)h + 16 + n) == '\0';
        printf("  [%lld] len=%lld cap=%lld bytes=\"%s\" %s\n",
               i, h ? h->len : -1, h ? h->cap : -1,
               h ? (char *)h + 16 : "<null>", good ? "OK" : "FAIL");
        if (!good) ok = 0;
    }
    printf("RESULT: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
