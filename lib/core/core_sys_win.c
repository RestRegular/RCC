/*
 * core_sys_win.c —— 系统能力原语的 Win32 C 实现（2026-11-20）
 *
 * 背景：core_win64.s 里的手写汇编调用带 5+ 参数的 Win32 API
 * （CreateFileA 等）时，栈参数布局极易出错；改由 C 编译器生成调用
 * 序列最稳妥。本文件编译成 .o 后由 CMake 用 `ld -r` 合并进
 * core_win64.o，对 RCC 链接器依旧是单个 core 目标文件（ABI 不变）。
 *
 * 约定：
 *   - path 是字节串指针 + 长度（Rio str 的 data 区），不依赖 NUL。
 *   - 句柄作为不透明 ptr 返回；失败返回 0，调用方必须显式 close。
 *   - 全部使用 Win32 ANSI API，与 core_cstr_len 的字节口径一致。
 */
#include <windows.h>
#include <stdint.h>
#include <string.h>

/* 文件句柄：用 MAX_PATH 的局部缓冲把非 NUL 结尾的字节串补 0。
 * 长度截断到 259，避免溢出。 */
static void copy_path(char* dst, size_t dstCap, const char* src, int64_t len)
{
    size_t n = (len < 0) ? 0 : (size_t)len;
    if (n >= dstCap) n = dstCap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* mode: 0=read, 1=write(截断), 2=append；失败返回 0 */
void* core_file_open(const char* path, int64_t len, int64_t mode)
{
    char buf[MAX_PATH];
    copy_path(buf, sizeof(buf), path, (int)len);

    DWORD access = GENERIC_READ;
    DWORD share = FILE_SHARE_READ;
    DWORD disp = OPEN_EXISTING;
    if (mode == 1) {
        access = GENERIC_WRITE;
        share = 0;
        disp = CREATE_ALWAYS;
    } else if (mode == 2) {
        access = GENERIC_WRITE | FILE_APPEND_DATA;
        share = FILE_SHARE_READ;
        disp = OPEN_ALWAYS;
    }
    HANDLE h = CreateFileA(buf, access, share, NULL, disp,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    if (mode == 2) {
        /* OPEN_ALWAYS 打开后文件指针停在 0；显式移到末尾才是真正的追加。 */
        LARGE_INTEGER end;
        end.QuadPart = 0;
        SetFilePointerEx(h, end, NULL, FILE_END);
    }
    return (void*)h;
}

void core_file_close(void* h)
{
    if (h) CloseHandle((HANDLE)h);
}

/* 返回字节数，失败 -1 */
int64_t core_file_size(void* h)
{
    LARGE_INTEGER size;
    if (!h || !GetFileSizeEx((HANDLE)h, &size)) return -1;
    return (int64_t)size.QuadPart;
}

/* 从文件头定位；返回是否成功（0/1） */
int64_t core_file_seek(void* h, int64_t offset)
{
    LARGE_INTEGER li;
    li.QuadPart = offset;
    if (!h) return 0;
    return SetFilePointerEx((HANDLE)h, li, NULL, FILE_BEGIN) ? 1 : 0;
}

/* 读/写，返回实际字节数，失败 -1 */
int64_t core_file_read(void* h, void* buf, int64_t n)
{
    DWORD got = 0;
    if (!h || n < 0) return -1;
    if (!ReadFile((HANDLE)h, buf, (DWORD)n, &got, NULL)) return -1;
    return (int64_t)got;
}

int64_t core_file_write(void* h, const void* buf, int64_t n)
{
    DWORD put = 0;
    if (!h || n < 0) return -1;
    if (!WriteFile((HANDLE)h, buf, (DWORD)n, &put, NULL)) return -1;
    return (int64_t)put;
}

/* Unix 毫秒时间戳（UTC）。
 * FILETIME 是 1601-01-01 起 100ns；减去到 1970 的偏移再 /10000。 */
int64_t core_time_unix_ms(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    const uint64_t EPOCH = 116444736000000000ULL;
    uint64_t v = u.QuadPart;
    if (v <= EPOCH) return 0;
    return (int64_t)((v - EPOCH) / 10000ULL);
}
