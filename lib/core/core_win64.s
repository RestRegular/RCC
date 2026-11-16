/*
 * core_win64.s —— core.rio 的 Windows x64 汇编实现（ABI v1）
 *
 * 调用约定：MS x64
 *   整型/指针参数：RCX, RDX, R8, R9（按参数位置，第 5 个起压栈）
 *   浮点参数：    XMM0..XMM3（同样按参数位置）
 *   返回值：      RAX（整数/指针）、XMM0（浮点）
 *   调用者必须预留 32 字节 shadow space，并保证 16 字节栈对齐
 *
 * 语法：GNU as（AT&T 后缀语法），由 as.exe 汇编为 COFF 目标文件
 *
 * 注意：所有导出符号使用 core_ 前缀，避免与用户代码和 CRT 冲突。
 */

    .text

/* ============================================================
 * void core_mem_copy(void* dst, const void* src, size_t n)
 *   rcx = dst, rdx = src, r8 = n
 * ============================================================ */
    .globl  core_mem_copy
core_mem_copy:
    movq    %rcx, %rax          /* 返回 dst */
    testq   %r8, %r8
    je      .Lmem_copy_done
    movq    %r8, %r9
    cld                          /* 明确方向：递增 */
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r9, %rcx
    rep movsb                    /* 逐字节复制 */
.Lmem_copy_done:
    ret

/* ============================================================
 * void core_mem_move(void* dst, const void* src, size_t n)
 * 处理重叠区间。先判断方向，再决定正序或逆序复制。
 *   rcx = dst, rdx = src, r8 = n
 * ============================================================ */
    .globl  core_mem_move
core_mem_move:
    movq    %rcx, %rax
    testq   %r8, %r8
    je      .Lmem_move_done
    cmpq    %rdx, %rcx
    jbe     .Lmem_move_forward   /* dst <= src：可以正序 */
    movq    %rcx, %r9
    subq    %rdx, %r9
    cmpq    %r8, %r9
    jae     .Lmem_move_forward   /* 不重叠：正序即可 */

    /* 重叠且 dst > src：必须从尾部倒序复制 */
    leaq    -1(%rcx,%r8), %rdi
    leaq    -1(%rdx,%r8), %rsi
    movq    %r8, %rcx
    std
    rep movsb
    cld
    jmp     .Lmem_move_done

.Lmem_move_forward:
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r8, %rcx
    cld
    rep movsb
.Lmem_move_done:
    ret

/* ============================================================
 * void core_mem_set(void* dst, int byte, size_t n)
 *   rcx = dst, rdx = byte, r8 = n
 * ============================================================ */
    .globl  core_mem_set
core_mem_set:
    movq    %rcx, %rax
    testq   %r8, %r8
    je      .Lmem_set_done
    movq    %rcx, %rdi
    movl    %edx, %eax
    movq    %r8, %rcx
    cld
    rep stosb
.Lmem_set_done:
    ret

/* ============================================================
 * bool core_mem_eq(const void* a, const void* b, size_t n)
 *   rcx = a, rdx = b, r8 = n
 * 返回 1 表示相等，0 表示不等
 * ============================================================ */
    .globl  core_mem_eq
core_mem_eq:
    testq   %r8, %r8
    je      .Lmem_eq_true
    movq    %rcx, %rsi
    movq    %rdx, %rdi
    movq    %r8, %rcx
    cld
    repe cmpsb
    jne     .Lmem_eq_false
.Lmem_eq_true:
    movl    $1, %eax
    ret
.Lmem_eq_false:
    xorl    %eax, %eax
    ret

/* ============================================================
 * void* core_alloc(size_t size, size_t align)
 *   rcx = size, rdx = align
 * 直接转发到 HeapAlloc（GetProcessHeap）。
 * 返回 null 表示失败。
 * ============================================================ */
    .globl  core_alloc
core_alloc:
    subq    $40, %rsp            /* 32 shadow + 8 对齐 */
    movq    %rcx, %r8            /* r8 = size（HeapAlloc 第 3 参） */
    movq    %rdx, %r9            /* 保存 align（本实现忽略，堆已 16 字节对齐） */
    call    GetProcessHeap
    testq   %rax, %rax
    je      .Lalloc_fail
    movq    %rax, %rcx           /* rcx = hHeap */
    xorl    %edx, %edx           /* rdx = flags = 0 */
    movq    %r8, %r8             /* r8 = dwBytes */
    call    HeapAlloc
    addq    $40, %rsp
    ret
.Lalloc_fail:
    xorl    %eax, %eax
    addq    $40, %rsp
    ret

/* ============================================================
 * void* core_realloc(void* p, size_t old_size, size_t size, size_t align)
 *   rcx = p, rdx = old_size, r8 = size, r9 = align
 * 简化实现：分配新块 -> 复制 min(old,size) -> 释放旧块
 * ============================================================ */
    .globl  core_realloc
core_realloc:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $48, %rsp            /* 32 shadow + 16 对齐（3 push + 48 = 72，16 对齐） */

    movq    %rcx, %rbx           /* rbx = 旧指针 */
    movq    %rdx, %rsi           /* rsi = old_size */
    movq    %r8,  %rdi           /* rdi = size */
    movq    %r9,  32(%rsp)       /* 暂存 align */

    /* 新块 = core_alloc(size, align) */
    movq    %rdi, %rcx
    movq    32(%rsp), %rdx
    call    core_alloc
    movq    %rax, 40(%rsp)       /* 暂存新指针 */
    testq   %rax, %rax
    je      .Lrealloc_out        /* 分配失败：返回 null，旧块保留 */

    /* 复制 min(old_size, size) 字节 */
    testq   %rbx, %rbx
    je      .Lrealloc_free_old
    movq    %rsi, %rdx
    cmpq    %rdi, %rdx
    jbe     .Lrealloc_copy
    movq    %rdi, %rdx           /* old_size > size：只复制 size */
.Lrealloc_copy:
    testq   %rdx, %rdx
    je      .Lrealloc_free_old
    movq    40(%rsp), %rcx       /* dst = 新块 */
    movq    %rbx, %rsi           /* src = 旧块（rsi 已用完） */
    movq    %rdx, %r8
    call    core_mem_copy

.Lrealloc_free_old:
    testq   %rbx, %rbx
    je      .Lrealloc_ret_new
    movq    %rbx, %rcx
    call    core_free
.Lrealloc_ret_new:
    movq    40(%rsp), %rax
.Lrealloc_out:
    addq    $48, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* ============================================================
 * void core_free(void* p)
 *   rcx = p
 *
 * P13-1 / D11 裁决 A：两条**静默返回**（不是隐式运行时检查，
 * 是值表示的约定 —— 见 docs/p13_plan.md D5/D11）：
 *   1. p == nullptr      -> 直接返回
 *   2. p->cap == 0       -> 直接返回
 *      「cap == 0」表示这是**静态只读头**（字符串字面量指向 .rdata，
 *      由 codegen 的 internStrValue 发出，cap 字段恒为 0）。
 *      对这种头调 HeapFree 会直接崩。
 *
 * 约定 cap 在 header 偏移 +8：str / list / dict 三个容器共用
 * 「len@0, cap@8」（见 rcc_codegen.h 的 kHeaderLen/kHeaderCap）。
 *
 * 重复释放仍是**未定义行为**（不加运行期检查，符合「无隐式运行时检查」
 * 原则）。调试构建的重复释放检测由 codegen 侧钩子负责（D17）。
 * ============================================================ */
    .globl  core_free
core_free:
    testq   %rcx, %rcx
    je      .Lfree_done
    /* cap == 0 -> 静态只读头，不可释放 */
    cmpq    $0, 8(%rcx)
    je      .Lfree_done
    subq    $40, %rsp
    movq    %rcx, 32(%rsp)       /* 暂存 p */
    call    GetProcessHeap
    movq    %rax, %rcx
    xorl    %edx, %edx
    movq    32(%rsp), %r8
    call    HeapFree
    addq    $40, %rsp
.Lfree_done:
    ret

/* ============================================================
 * int core_io_write(int fd, const void* buf, size_t n)
 *   rcx = fd, rdx = buf, r8 = n
 * 用 WriteFile 写到对应句柄。fd 0/1/2 映射到标准句柄。
 * 返回写入字节数，失败返回 -1。
 * ============================================================ */
    .globl  core_io_write
core_io_write:
    /* 帧布局（rsp 相对）：
     *   0..31   shadow space（WriteFile 用）
     *   32      第 5 个参数 lpOverlapped
     *   40      buf
     *   48      n
     *   56      hFile
     *   64      bytesWritten
     * subq $88 后，88+8(返回地址)=96，满足 16 字节对齐
     */
    subq    $88, %rsp

    movq    %rdx, 40(%rsp)       /* buf */
    movq    %r8,  48(%rsp)       /* n */
    movl    %ecx, %eax

    /* 选句柄：0->stdin, 1->stdout, 2->stderr */
    cmpl    $1, %eax
    je      .Lio_w_stdout
    cmpl    $2, %eax
    je      .Lio_w_stderr
    movl    $-10, %ecx           /* STD_INPUT_HANDLE */
    call    GetStdHandle
    movq    %rax, 56(%rsp)
    jmp     .Lio_w_do

.Lio_w_stdout:
    movl    $-11, %ecx           /* STD_OUTPUT_HANDLE */
    call    GetStdHandle
    movq    %rax, 56(%rsp)
    jmp     .Lio_w_do

.Lio_w_stderr:
    movl    $-12, %ecx           /* STD_ERROR_HANDLE */
    call    GetStdHandle
    movq    %rax, 56(%rsp)

.Lio_w_do:
    movq    56(%rsp), %rcx       /* hFile */
    movq    40(%rsp), %rdx       /* lpBuffer */
    movq    48(%rsp), %r8        /* nNumberOfBytesToWrite */
    leaq    64(%rsp), %r9        /* lpNumberOfBytesWritten */
    movq    $0, 32(%rsp)         /* 第 5 参：lpOverlapped = NULL */
    call    WriteFile
    testl   %eax, %eax
    je      .Lio_w_fail
    movq    64(%rsp), %rax       /* 返回实际写入字节数 */
    addq    $88, %rsp
    ret
.Lio_w_fail:
    movq    $-1, %rax
    addq    $88, %rsp
    ret

/* ============================================================
 * int core_io_write_byte(int fd, int ch)
 *   rcx = fd, rdx = ch（只用低 8 位）
 *
 * 单字节写。存在的理由：rt 层要用 Rio 写整数/浮点的十进制转换，
 * 而每输出一个数字字符都需要「写一个字节」。Rio 在 P1 还没有
 * 取地址运算符，无法把一个局部变量当缓冲区传给 core_io_write，
 * 所以在这里提供一个直接收字节的入口。
 *
 * 这符合 core 的定位：写一个字节给 OS 是 Rio 语言本身表达不了的。
 *
 * 帧布局（rsp 相对）：
 *   0..31   shadow space（WriteFile 用）
 *   32      第 5 个参数 lpOverlapped
 *   40      hFile
 *   48      bytesWritten
 *   56      单字节缓冲
 *   subq $88 后 88+8(返回地址)=96，满足 16 字节对齐
 * ============================================================ */
    .globl  core_io_write_byte
core_io_write_byte:
    subq    $88, %rsp

    movb    %dl, 56(%rsp)        /* 把字符存到栈缓冲 */

    movl    %ecx, %eax
    cmpl    $2, %eax
    je      .Lio_wb_stderr
    movl    $-11, %ecx           /* STD_OUTPUT_HANDLE（fd 0/1 都当 stdout） */
    call    GetStdHandle
    movq    %rax, 40(%rsp)
    jmp     .Lio_wb_do

.Lio_wb_stderr:
    movl    $-12, %ecx           /* STD_ERROR_HANDLE */
    call    GetStdHandle
    movq    %rax, 40(%rsp)

.Lio_wb_do:
    movq    40(%rsp), %rcx       /* hFile */
    leaq    56(%rsp), %rdx       /* lpBuffer（指向单字节缓冲） */
    movq    $1, %r8              /* 写 1 字节 */
    leaq    48(%rsp), %r9        /* lpNumberOfBytesWritten */
    movq    $0, 32(%rsp)         /* 第 5 参：lpOverlapped = NULL */
    call    WriteFile
    testl   %eax, %eax
    je      .Lio_wb_fail
    movq    $1, %rax
    addq    $88, %rsp
    ret
.Lio_wb_fail:
    movq    $-1, %rax
    addq    $88, %rsp
    ret

/* ============================================================
 * int core_str_len(void* str)
 *   rcx = str（指向 StrHeader）
 * 返回 str 的字节长度（StrHeader 的第一个 8 字节）。
 *
 * 存在的理由：P1 的 Rio 还没有指针解引用运算符（`*p`），
 * 无法在 Rio 里读取「指针指向的那个 8 字节」。等 P2 支持解引用后，
 * rt_print_str 可以改成纯 Rio 实现，这个函数就可以删掉。
 *
 * str 为 null 时返回 0。
 * ============================================================ */
    .globl  core_str_len
core_str_len:
    xorl    %eax, %eax
    testq   %rcx, %rcx
    je      .Lstr_len_done
    movq    (%rcx), %rax
.Lstr_len_done:
    ret

/* ============================================================
 * int core_io_read(int fd, void* buf, size_t n)
 *   rcx = fd, rdx = buf, r8 = n
 * ============================================================ */
    .globl  core_io_read
core_io_read:
    /* 帧布局同 core_io_write：0..31 shadow，32 第5参，40 buf，48 n，56 hFile，64 bytesRead */
    subq    $88, %rsp

    movq    %rdx, 40(%rsp)       /* buf */
    movq    %r8,  48(%rsp)       /* n */

    movl    $-10, %ecx           /* STD_INPUT_HANDLE */
    call    GetStdHandle
    movq    %rax, 56(%rsp)

    movq    56(%rsp), %rcx
    movq    40(%rsp), %rdx
    movq    48(%rsp), %r8
    leaq    64(%rsp), %r9
    movq    $0, 32(%rsp)         /* 第 5 参：lpOverlapped = NULL */
    call    ReadFile
    testl   %eax, %eax
    je      .Lio_r_fail
    movq    64(%rsp), %rax
    addq    $88, %rsp
    ret
.Lio_r_fail:
    movq    $-1, %rax
    addq    $88, %rsp
    ret

/* ============================================================
 * void core_io_flush(int fd)
 * 简化为无操作（stdout 为行缓冲/无缓冲时无需显式刷新）
 * ============================================================ */
    .globl  core_io_flush
core_io_flush:
    ret

/* ============================================================
 * void core_proc_exit(int code)
 *   rcx = code
 * ============================================================ */
    .globl  core_proc_exit
core_proc_exit:
    subq    $40, %rsp
    call    ExitProcess
    hlt                          /* 不会到达 */
    addq    $40, %rsp
    ret

/* ============================================================
 * void core_proc_abort(void)
 * ============================================================ */
    .globl  core_proc_abort
core_proc_abort:
    subq    $40, %rsp
    movl    $3, %ecx             /* 非零退出码 */
    call    ExitProcess
    hlt
    addq    $40, %rsp
    ret

/* ============================================================
 * void core_panic_raise(const char* msg, size_t len)
 *   rcx = msg, rdx = len
 * 写 stderr 后 abort
 * ============================================================ */
    .globl  core_panic_raise
core_panic_raise:
    subq    $40, %rsp
    movl    $2, %r8d             /* fd = stderr，但 core_io_write 签名是 (fd,buf,n) */
    movq    %rcx, %r9            /* 暂存 msg */
    movq    %rdx, %r10           /* 暂存 len */
    /* core_io_write(2, msg, len) */
    movl    $2, %ecx
    movq    %r9, %rdx
    movq    %r10, %r8
    call    core_io_write
    call    core_proc_abort
    addq    $40, %rsp
    ret

/* ============================================================
 * void core_stack_probe(size_t size)
 *   rcx = size
 * 逐页触碰栈，触发 guard page 增长。
 * 由编译器在 >4KB 栈帧的函数序言里插入。
 * ============================================================ */
    .globl  core_stack_probe
core_stack_probe:
    movq    %rcx, %rax
    testq   %rax, %rax
    je      .Lprobe_done
.Lprobe_loop:
    subq    $4096, %rax
    testq   %rax, %rax
    jle     .Lprobe_done
    movq    (%rsp), %rcx         /* 触碰页面 */
    jmp     .Lprobe_loop
.Lprobe_done:
    ret

/* ============================================================
 * void* core_eh_push(void* frame)
 * void  core_eh_pop(void* frame)
 * void  core_eh_throw(void* exc)
 *
 * 显式帧链（方案 B）：core 维护一个 handler 栈顶指针。
 * 帧布局由 codegen 决定，core 只知道前 8 字节是 prev 指针，
 * 以及 +8/+16/+24/+32 分别是 saved rbp / saved rsp / resume / exc。
 *
 * 帧布局（48 字节）：
 *   +0   prev        上一帧（core_eh_push 写）
 *   +8   saved rbp   进入 try 时的 %rbp
 *   +16  saved rsp   进入 try 时的 %rsp
 *   +24  resume      异常落地后跳转的地址
 *   +32  exc         异常值（core_eh_throw 写，catch 读）
 *   +40  pad
 *
 * 这些偏移是 codegen 与 core 之间的**约定**，改动必须同步
 * code/src/backend/rcc_codegen.cpp 里的 emitTry。
 * ============================================================ */
    .data
    .align 8
core_eh_top:
    .quad   0                    /* 当前栈顶帧 */

    .text
    .globl  core_eh_push
core_eh_push:
    movq    core_eh_top(%rip), %rax
    movq    %rax, (%rcx)         /* frame->prev = 旧栈顶 */
    movq    %rcx, core_eh_top(%rip)
    movq    %rcx, %rax           /* 返回新栈顶 */
    ret

    .globl  core_eh_pop
core_eh_pop:
    movq    (%rcx), %rax         /* frame->prev */
    movq    %rax, core_eh_top(%rip)
    ret

/* ============================================================
 * void core_eh_throw(void* exc)
 *   rcx = 异常值
 *
 * 沿帧链向上找 handler：把 exc 写进目标帧的 +32，
 * 恢复它保存的 %rbp / %rsp，然后跳到 +24 的 resume 地址。
 *
 * 关键点：
 *   * 链顶在跳跃前**停在目标帧**上（不弹出它）—— 目标帧的 handler
 *     落地后会自己 core_eh_pop，这样 catch 体里再 throw 不会看到
 *     自己那一帧（否则会自己接自己，死循环）。
 *   * 恢复 %rsp 必须在读取帧字段**之后**：目标帧可能就在当前 %rsp
 *     之上，先改 %rsp 会踩掉还没读的字段。
 *   * 没有任何 handler 时退化为 abort（退出码 3），与 P0 骨架一致。
 * ============================================================ */
    .globl  core_eh_throw
core_eh_throw:
    movq    core_eh_top(%rip), %rax
    testq   %rax, %rax
    je      .Leh_uncaught        /* 链空 -> 未捕获 */
    movq    %rcx, 32(%rax)       /* frame->exc = exc（先存，后面要毁掉 rcx） */
    movq    8(%rax), %rbp        /* 恢复 %rbp */
    movq    16(%rax), %rcx       /* 取 saved rsp */
    movq    24(%rax), %rdx       /* 取 resume 地址 */
    movq    %rcx, %rsp           /* 恢复 %rsp（必须在读完帧字段之后） */
    jmp     *%rdx

.Leh_uncaught:
    /* 未捕获：退化为 abort（退出码 3），与 P0 骨架的既有约定一致。
       用 jmp 而不是 call，省掉对齐顾虑，反正不会返回。 */
    jmp     core_proc_abort

/* ============================================================
 * long long core_time_ns(void)
 * 用 QueryPerformanceCounter 近似（P0 骨架）
 * ============================================================ */
    .globl  core_time_ns
core_time_ns:
    subq    $40, %rsp
    leaq    32(%rsp), %rcx
    call    QueryPerformanceCounter
    movq    32(%rsp), %rax
    addq    $40, %rsp
    ret

/* ============================================================
 * void core_sys_breakpoint(void)
 * ============================================================ */
    .globl  core_sys_breakpoint
core_sys_breakpoint:
    int3
    ret
