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
 *
 * P13-1 修正（ABI 合规）：%rdi / %rsi 在 MS x64 上是 **callee-saved**，
 * 这里必须保存并恢复。此前直接借来当 rep movsb 的源/目的寄存器、返回前
 * 不恢复，导致任何「把值放在 rsi/rdi 里跨 core_* 调用」的调用者被悄悄
 * 改掉数据 —— core_list_append 的 value（rsi）与 newcap（rdi）就是这么
 * 被冲掉的，实测追加后元素变成 0。
 * ============================================================ */
    .globl  core_mem_copy
core_mem_copy:
    pushq   %rdi
    pushq   %rsi
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
    popq    %rsi
    popq    %rdi
    ret

/* ============================================================
 * void core_mem_move(void* dst, const void* src, size_t n)
 * 处理重叠区间。先判断方向，再决定正序或逆序复制。
 *   rcx = dst, rdx = src, r8 = n
 * ============================================================ */
    .globl  core_mem_move
core_mem_move:
    pushq   %rdi
    pushq   %rsi
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
    popq    %rsi
    popq    %rdi
    ret

/* ============================================================
 * void core_mem_set(void* dst, int byte, size_t n)
 *   rcx = dst, rdx = byte, r8 = n
 * ============================================================ */
    .globl  core_mem_set
core_mem_set:
    pushq   %rdi
    pushq   %rsi
    movq    %rcx, %rax
    testq   %r8, %r8
    je      .Lmem_set_done
    movq    %rcx, %rdi
    movl    %edx, %eax
    movq    %r8, %rcx
    cld
    rep stosb
.Lmem_set_done:
    popq    %rsi
    popq    %rdi
    ret

/* ============================================================
 * bool core_mem_eq(const void* a, const void* b, size_t n)
 *   rcx = a, rdx = b, r8 = n
 * 返回 1 表示相等，0 表示不等
 * ============================================================ */
    .globl  core_mem_eq
core_mem_eq:
    pushq   %rdi
    pushq   %rsi
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
    popq    %rsi
    popq    %rdi
    ret
.Lmem_eq_false:
    xorl    %eax, %eax
    popq    %rsi
    popq    %rdi
    ret

/* ============================================================
 * void* core_alloc(size_t size, size_t align)
 *   rcx = size, rdx = align
 * 直接转发到 HeapAlloc（GetProcessHeap）。
 * 返回 null 表示失败。
 * ============================================================ */
    .globl  core_alloc
core_alloc:
    pushq   %rbx                 /* callee-saved；进入时 rsp≡8，push 后 ≡0 */
    subq    $48, %rsp            /* 32 shadow + 16，保持 rsp≡0 */
    movq    %rcx, %rbx           /* rbx = size */
    movq    %rdx, 32(%rsp)       /* 保存 align */
    call    GetProcessHeap
    testq   %rax, %rax
    je      .Lalloc_fail
    movq    %rax, %rcx           /* rcx = hHeap */
    xorl    %edx, %edx           /* rdx = flags = 0 */
    movq    %rbx, %r8            /* r8 = dwBytes = size */
    call    HeapAlloc
    movq    %rax, 32(%rsp)       /* 保存分配指针（helper 会破坏 rax） */
    testq   %rax, %rax
    je      .Lalloc_nobump
    lock addq $1, core_dbg_alloc_count(%rip)
    /* 这个地址已重新分配 -> 从「已释放」ring 清掉（P14-1a：加锁 helper）。 */
    movq    %rax, %rcx
    call    core_dbg_ring_clear
.Lalloc_nobump:
    movq    32(%rsp), %rax       /* 返回值 = 分配指针（null 时也是它） */
    addq    $48, %rsp
    popq    %rbx
    ret
.Lalloc_fail:
    xorl    %eax, %eax
    addq    $48, %rsp
    popq    %rbx
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
    je      .Lfree_ret
    cmpq    $0, 8(%rcx)              /* cap==0 -> 静态只读头，不可释放 */
    je      .Lfree_ret
    pushq   %rsi                     /* callee-saved（下方 core_mem_set 等用） */
    pushq   %rdi
    subq    $56, %rsp                /* 32 shadow + 24；rsp≡0 */
    movq    %rcx, 32(%rsp)           /* 暂存 p */

    /* ---- P14-1a：重复释放检测（加锁 helper 保护 ring 复合操作） ---- */
    movq    32(%rsp), %rcx
    call    core_dbg_dup_check
    testq   %rax, %rax
    jne     .Lfree_dup_hit           /* 命中 -> 只计数，不真的释放 */

.Lfree_do_it:
    call    GetProcessHeap
    movq    %rax, 40(%rsp)           /* hHeap */
    /* 释放前整块填 0xDD（块大小用 HeapSize 问系统） */
    movq    40(%rsp), %rcx
    xorl    %edx, %edx
    movq    32(%rsp), %r8
    call    HeapSize
    cmpq    $-1, %rax
    je      .Lfree_after_poison
    movq    %rax, 48(%rsp)           /* size */
    testq   %rax, %rax
    je      .Lfree_after_poison
    movq    32(%rsp), %rcx
    movl    $0xDD, %edx
    movq    48(%rsp), %r8
    call    core_mem_set
    movq    48(%rsp), %rax
    lock addq %rax, core_dbg_poison_bytes_total(%rip)

.Lfree_after_poison:
    movq    40(%rsp), %rcx
    xorl    %edx, %edx
    movq    32(%rsp), %r8
    call    HeapFree
    lock addq $1, core_dbg_free_count(%rip)
    /* 记进 recently-freed ring（加锁 helper） */
    movq    32(%rsp), %rcx
    call    core_dbg_ring_add
    addq    $56, %rsp
    popq    %rdi
    popq    %rsi
    ret

.Lfree_dup_hit:
    lock addq $1, core_dbg_double_free_count(%rip)
    addq    $56, %rsp
    popq    %rdi
    popq    %rsi
    ret

.Lfree_ret:
    ret
/* ============================================================
 * ptr core_list_append(ptr list, ptr value)
 *   rcx = list header, rdx = value（8 字节元素，直接按位复制）
 *
 * P13-1 / D6 + D12：受控的**原地追加**。
 *
 * 为什么必须在 core 层做：列表长度头在容器内部，而 codegen 的索引写
 * （`xs[n] = v`）有边界检查 —— 先比长度头，越界直接跳过。所以 std 层
 * 「先改长度头再写」这条走不通（实测元素根本没进去）。能同时改长度头
 * 和 data 指针的代码必须在 core 层。
 *
 * ABI v2 布局（rcc_codegen.h 的 kHeaderLen/kHeaderCap/kListData）：
 *   len@0  cap@8  elem_size@16  data@24
 *
 * 扩容策略：cap 不够时按 2 倍长（至少 4）重新分配 data，**header 地址不变**
 * —— 这正是 D12 选「header 与 data 分离」的理由：单指针紧凑布局下 realloc
 * 会让调用者的变量、其他拷贝、形参里的指针全部悬空。
 *
 * 返回 header 本身（原地）；扩容失败返回 null，此时 header 与旧 data 都
 * 保持原样，调用方可安全放弃。
 * ============================================================ */
    .globl  core_list_append
core_list_append:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $64, %rsp            /* 32 shadow + 32 局部（3 push + 64 = 88，16 对齐） */

    movq    %rcx, %rbx           /* rbx = header */
    movq    %rdx, 48(%rsp)       /* value 放栈槽：跨 core_* 调用不会被冲掉 */
    testq   %rbx, %rbx
    je      .Lla_fail            /* null header：无可追加 */

    movq    (%rbx), %rax         /* len */
    movq    8(%rbx), %rdx        /* cap */
    cmpq    %rdx, %rax
    jb      .Lla_have_space      /* len < cap：原地写 */

    /* ---- 需要扩容 ---- */
    /* newcap = max(4, cap*2) */
    movq    %rdx, %rdi
    testq   %rdi, %rdi
    jne     .Lla_double
    movl    $4, %edi
    jmp     .Lla_newcap_ready
.Lla_double:
    addq    %rdi, %rdi
    cmpq    $4, %rdi
    jae     .Lla_newcap_ready
    movl    $4, %edi
.Lla_newcap_ready:
    movq    %rdi, 40(%rsp)       /* newcap 放栈槽 */
    /* 新 data 块 = newcap * 8 字节 */
    movq    %rdi, %rcx
    shlq    $3, %rcx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lla_fail
    movq    %rax, 32(%rsp)       /* newdata */

    /* 复制旧元素：n = len * 8 */
    movq    (%rbx), %r8
    shlq    $3, %r8
    testq   %r8, %r8
    je      .Lla_copy_done
    movq    24(%rbx), %rdx       /* 旧 data */
    testq   %rdx, %rdx
    je      .Lla_copy_done
    movq    32(%rsp), %rcx       /* dst */
    call    core_mem_copy
.Lla_copy_done:
    /* 释放旧 data（cap>0 时它一定是我们分配的） */
    movq    24(%rbx), %rcx
    testq   %rcx, %rcx
    je      .Lla_install
    call    core_free
.Lla_install:
    /* 改写 header：data 指向新块、cap = newcap。len 不变（下面统一 +1） */
    movq    32(%rsp), %rax
    movq    %rax, 24(%rbx)
    movq    40(%rsp), %rax
    movq    %rax, 8(%rbx)

.Lla_have_space:
    /* 写元素：data[len] = value */
    movq    (%rbx), %rax         /* len */
    movq    24(%rbx), %rcx       /* data */
    testq   %rcx, %rcx
    je      .Lla_fail            /* 有空间却没有 data：状态不一致 */
    movq    48(%rsp), %rdx       /* value */
    movq    %rdx, (%rcx,%rax,8)
    /* len += 1 */
    incq    %rax
    movq    %rax, (%rbx)

    movq    %rbx, %rax           /* 返回 header（原地） */
    jmp     .Lla_out
.Lla_fail:
    xorl    %eax, %eax
.Lla_out:
    addq    $64, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* ============================================================
 * ptr core_clone_str(ptr src)
 *   rcx = src str header（len@0, cap@8, bytes@16）
 * 返回一个**新分配**的 str 头（cap 等于真实容量，因而可释放）；
 * src 为 null 时返回 null。
 *
 * 为什么需要：严格借用规则拦下 `ret 形参` 这类恒等函数之后，std 需要一个
 * 合法手段把「借来的值」变成「自己拥有的新值」—— clone 就是那个手段，
 * 不需要放宽任何规则，也不制造近似。
 * ============================================================ */
    .globl  core_clone_str
core_clone_str:
    pushq   %rbx
    pushq   %rsi
    subq    $40, %rsp            /* 32 shadow + 8 对齐（2 push + 40 = 56） */

    movq    %rcx, %rbx           /* rbx = src */
    testq   %rbx, %rbx
    je      .Lcs_null
    movq    (%rbx), %rsi         /* len */
    /* 新块 = 16 + len（头 16 字节 + 内容） */
    leaq    16(%rsi), %rcx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lcs_null
    /* len = len, cap = len（真实容量，可释放） */
    movq    %rsi, (%rax)
    movq    %rsi, 8(%rax)
    /* 复制内容：dst=rax+16, src=rbx+16, n=len */
    movq    %rax, 32(%rsp)       /* 保命：core_mem_copy 会冲掉 rax */
    testq   %rsi, %rsi
    je      .Lcs_done
    leaq    16(%rax), %rcx
    leaq    16(%rbx), %rdx
    movq    %rsi, %r8
    call    core_mem_copy
.Lcs_done:
    movq    32(%rsp), %rax
    jmp     .Lcs_out
.Lcs_null:
    xorl    %eax, %eax
.Lcs_out:
    addq    $40, %rsp
    popq    %rsi
    popq    %rbx
    ret

/* ============================================================
 * ptr core_clone_list(ptr src)
 *   rcx = src list header（len@0, cap@8, elem_size@16, data@24）
 *
 * 语义上是**浅拷贝元素槽**：src 里那些 8 字节值原样复制到新 data 块。
 * 这与 D14 的口径一致 —— P13-1 不递归释放元素，所以也不递归克隆元素
 * （逐元素判断「这个槽里是 int 还是 str 指针」需要运行期类型标签，而本
 * 架构是静态的、无标签）。真正的按元素类型深拷贝要等 P13-4 的子引用
 * 路径，那时元素所有权才有静态依据。
 *
 * 所以这里的 clone 保证的是：**容器自身**与源完全独立（改一方的
 * len/data/元素槽不影响另一方），元素指向的堆对象仍共享。
 * ============================================================ */
    .globl  core_clone_list
core_clone_list:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $48, %rsp

    movq    %rcx, %rbx           /* rbx = src */
    testq   %rbx, %rbx
    je      .Lcl_null
    movq    (%rbx), %rsi         /* len */
    movq    16(%rbx), %rdi       /* elem_size */

    /* 新 header（32 字节） */
    movl    $32, %ecx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lcl_null
    movq    %rax, 32(%rsp)       /* newheader */

    /* 新 header 的 len/cap/elem_size */
    movq    %rsi, (%rax)
    movq    %rsi, 8(%rax)
    movq    %rdi, 16(%rax)

    /* 新 data 块：len*8（len==0 时不分配，留 null） */
    testq   %rsi, %rsi
    je      .Lcl_no_data
    movq    %rsi, %rcx
    shlq    $3, %rcx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lcl_fail
    movq    32(%rsp), %rcx
    movq    %rax, 24(%rcx)       /* newheader->data */
    /* 复制元素槽：n = len*8 */
    movq    %rax, %rcx           /* dst */
    movq    24(%rbx), %rdx       /* src data */
    movq    %rsi, %r8
    shlq    $3, %r8
    call    core_mem_copy
    jmp     .Lcl_done
.Lcl_no_data:
    movq    32(%rsp), %rcx
    movq    $0, 24(%rcx)
.Lcl_done:
    movq    32(%rsp), %rax
    jmp     .Lcl_out
.Lcl_fail:
    movq    32(%rsp), %rcx
    call    core_free
    xorl    %eax, %eax
    jmp     .Lcl_out
.Lcl_null:
    xorl    %eax, %eax
.Lcl_out:
    addq    $48, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* ============================================================
 * ptr core_clone_dict(ptr src)
 *   rcx = src dict header（len@0, cap@8, keys@16, vals@24）
 *
 * 与 core_clone_list 同口径（D14）：浅拷贝 keys/vals 两个 8 字节槽数组，
 * 元素指向的堆对象仍共享；新 header 与源完全独立。静态无类型标签，
 * 不做逐元素深拷贝。
 * ============================================================ */
    .globl  core_clone_dict
core_clone_dict:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    pushq   %r12
    subq    $48, %rsp

    movq    %rcx, %rbx           /* rbx = src */
    testq   %rbx, %rbx
    je      .Lcd_null
    movq    (%rbx), %rsi         /* len */
    movq    16(%rbx), %rdi       /* keys */
    movq    24(%rbx), %r12       /* vals */

    /* 新 header（32 字节） */
    movl    $32, %ecx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lcd_null
    movq    %rax, 32(%rsp)       /* newheader */
    movq    %rsi, (%rax)         /* len */
    movq    %rsi, 8(%rax)        /* cap */

    testq   %rsi, %rsi
    je      .Lcd_no_data
    /* 新 keys 块 */
    movq    %rsi, %rcx
    shlq    $3, %rcx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lcd_fail
    movq    %rax, 16(%rsp)       /* newkeys */
    movq    32(%rsp), %rcx
    movq    %rax, 16(%rcx)       /* header->keys */
    /* 复制 keys */
    movq    %rax, %rcx
    movq    %rdi, %rdx
    movq    %rsi, %r8
    shlq    $3, %r8
    call    core_mem_copy
    /* 新 vals 块 */
    movq    %rsi, %rcx
    shlq    $3, %rcx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lcd_fail
    movq    %rax, 8(%rsp)        /* newvals */
    movq    32(%rsp), %rcx
    movq    %rax, 24(%rcx)       /* header->vals */
    /* 复制 vals */
    movq    %rax, %rcx
    movq    %r12, %rdx
    movq    %rsi, %r8
    shlq    $3, %r8
    call    core_mem_copy
    jmp     .Lcd_done
.Lcd_no_data:
    movq    32(%rsp), %rcx
    movq    $0, 16(%rcx)
    movq    $0, 24(%rcx)
.Lcd_done:
    movq    32(%rsp), %rax
    jmp     .Lcd_out
.Lcd_fail:
    /* 新 keys 可能已分配，释放之；再释放 header */
    movq    16(%rsp), %rcx
    testq   %rcx, %rcx
    je      .Lcd_fail2
    call    core_free
.Lcd_fail2:
    movq    32(%rsp), %rcx
    call    core_free
    xorl    %eax, %eax
    jmp     .Lcd_out
.Lcd_null:
    xorl    %eax, %eax
.Lcd_out:
    addq    $48, %rsp
    popq    %r12
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* ============================================================
 * ============ P13-1 调试钩子（D17）============
 *
 * 只在**调试构建**里被 codegen 引用（`-debug-alloc` 开关），发布构建
 * 完全不调用它们 —— D5 说的是「产物无运行期检查」，调试钩子不违反。
 *
 * 没有它们，「作用域末确实释放了」这件事无法自动化验证：程序跑完
 * 就退出，泄漏与正确释放从外部看不出来。
 *
 * 计数器是进程内的静态 8 字节变量，不参与任何 ABI 参数传递：
 *   core_dbg_alloc_count        alloc 成功次数
 *   core_dbg_free_count         free 真正落到 HeapFree 的次数（cap==0/ null 不算）
 *   core_dbg_poison_bytes       free 前填 0xDD 的总字节数
 *   core_dbg_double_free_count  被检测到的重复释放次数
 * ============================================================ */

    .bss
    .align  8
core_dbg_alloc_count:
    .zero   8
core_dbg_free_count:
    .zero   8
core_dbg_live_bytes:
    .zero   8
/* D17：重复释放检测与填埋统计 */
core_dbg_double_free_count:
    .zero   8
core_dbg_poison_bytes_total:
    .zero   8
/* P14-1a：recent ring 改为 TLS（每线程 136 字节：16 指针 + idx）。
 * 原全局地址 ring 在多线程地址复用下会误判重复释放（实测 11/402000），
 * 加锁无法解决该 ABA 时序问题；按 Q14=C「线程状态归线程」改为 TLS 后，
 * 每个线程只看自己释放过的地址，跨线程地址复用不再互相污染。 */
    .align  8
core_dbg_tls_index:      /* DWORD，0=未初始化；TlsAlloc 惰性分配 */
    .zero   4
core_dbg_tls_inited:     /* DWORD，1=索引已分配 */
    .zero   4
core_dbg_spin:           /* 保护 TLS 索引惰性初始化的一次性自旋锁 */
    .zero   4
    .zero   4

    .text
/* ============================================================
 * P14-1a：调试状态自旋锁的内部 helper（不导出，仅 core 内部用）
 *   core_dbg_lock_local / core_dbg_unlock_local：xchg 隐含 lock 语义
 *   core_dbg_dup_check / ring_add / ring_clear：把 ring 的复合操作包进临界区
 * 这些 helper 只破坏 caller-saved 寄存器（rax/rcx/rdx/r8），不碰
 * rbx/rbp/rdi/rsi/r12-r15，符合 MS x64 ABI。
 * ============================================================ */
/* ------------------------------------------------------------
 * P14-1a helper：recent ring 改 TLS（每线程独立），消除并发地址复用误判。
 * core_dbg_get_ring() -> rax = 本线程 ring 地址（136 字节）。
 * 惰性初始化 TlsAlloc + HeapAlloc；破坏 rax/rcx/rdx/r8/r9；不碰 callee-saved。
 * 栈：进入 rsp≡8；push rbx -> 0；sub $40 -> 0；内部 call 时 rsp≡0 ✓。
 * ------------------------------------------------------------ */
core_dbg_get_ring:
    pushq   %rbx
    subq    $40, %rsp
    /* TLS 索引一次性初始化（自旋锁 + 双重检查） */
    cmpl    $0, core_dbg_tls_inited(%rip)
    jne     .Lgr_have_index
    movl    $1, %eax
.Lgr_spin:
    xchgl   %eax, core_dbg_spin(%rip)
    testl   %eax, %eax
    jz      .Lgr_spin_got
    pause
    jmp     .Lgr_spin
.Lgr_spin_got:
    cmpl    $0, core_dbg_tls_inited(%rip)
    jne     .Lgr_init_done
    call    TlsAlloc
    movl    %eax, core_dbg_tls_index(%rip)
    movl    $1, core_dbg_tls_inited(%rip)
.Lgr_init_done:
    xorl    %eax, %eax
    xchgl   %eax, core_dbg_spin(%rip)
.Lgr_have_index:
    movl    core_dbg_tls_index(%rip), %ecx
    call    TlsGetValue
    testq   %rax, %rax
    jnz     .Lgr_ret
    /* 本线程 ring 未分配：GetProcessHeap -> HeapAlloc(136) -> TlsSetValue */
    call    GetProcessHeap
    movq    %rax, %rbx                   /* rbx = hHeap */
    xorl    %edx, %edx                  /* flags = 0 */
    movq    $136, %r8                   /* size */
    movq    %rbx, %rcx                  /* hHeap */
    call    HeapAlloc
    movq    %rax, %rbx                  /* rbx = ring */
    testq   %rbx, %rbx
    je      .Lgr_ret                    /* 分配失败：rax=null，dup 检测退化 */
    /* 清零 136 字节（idx@128 必须是 0，否则 ring_add 越界） */
    movq    %rbx, %rcx
    xorl    %edx, %edx
    movq    $136, %r8
    call    core_mem_set
    movl    core_dbg_tls_index(%rip), %ecx
    movq    %rbx, %rdx
    call    TlsSetValue
    movq    %rbx, %rax
.Lgr_ret:
    addq    $40, %rsp
    popq    %rbx
    ret

/* int core_dbg_dup_check(void* p) — rcx=p；rax=1 命中/0 未命中
 * 栈：进入 rsp≡8；push rbx/rdi -> rsp≡0；sub $40 -> 0。 */
core_dbg_dup_check:
    pushq   %rbx
    pushq   %rdi
    subq    $40, %rsp
    movq    %rcx, %rbx                  /* rbx = p */
    call    core_dbg_get_ring
    testq   %rax, %rax
    je      .Ldc_miss
    movq    %rax, %rdi                  /* rdi = ring */
    xorl    %ecx, %ecx
.Ldc_loop:
    cmpl    $16, %ecx
    jae     .Ldc_miss
    movq    (%rdi,%rcx,8), %rax
    cmpq    %rbx, %rax
    je      .Ldc_hit
    incl    %ecx
    jmp     .Ldc_loop
.Ldc_hit:
    movl    $1, %eax
    jmp     .Ldc_done
.Ldc_miss:
    xorl    %eax, %eax
.Ldc_done:
    addq    $40, %rsp
    popq    %rdi
    popq    %rbx
    ret

/* void core_dbg_ring_add(void* p) — rcx=p；FIFO 覆盖 */
core_dbg_ring_add:
    pushq   %rbx
    pushq   %rdi
    subq    $40, %rsp
    movq    %rcx, %rbx
    call    core_dbg_get_ring
    testq   %rax, %rax
    je      .Lra_done
    movq    %rax, %rdi
    movq    128(%rdi), %rax             /* idx */
    movq    %rbx, (%rdi,%rax,8)
    incq    %rax
    andq    $15, %rax
    movq    %rax, 128(%rdi)
.Lra_done:
    addq    $40, %rsp
    popq    %rdi
    popq    %rbx
    ret

/* void core_dbg_ring_clear(void* p) — rcx=p */
core_dbg_ring_clear:
    pushq   %rbx
    pushq   %rdi
    subq    $40, %rsp
    movq    %rcx, %rbx
    call    core_dbg_get_ring
    testq   %rax, %rax
    je      .Lrc_done
    movq    %rax, %rdi
    xorl    %ecx, %ecx
.Lrc_loop:
    movq    (%rdi,%rcx,8), %rax
    cmpq    %rbx, %rax
    jne     .Lrc_next
    movq    $0, (%rdi,%rcx,8)
.Lrc_next:
    incl    %ecx
    cmpl    $16, %ecx
    jne     .Lrc_loop
.Lrc_done:
    addq    $40, %rsp
    popq    %rdi
    popq    %rbx
    ret

/* void core_dbg_reset(void) —— 只清全局计数；TLS ring 随线程退出自然回收 */
    .globl  core_dbg_reset
core_dbg_reset:
    xorq    %rax, %rax
    movq    %rax, core_dbg_alloc_count(%rip)
    movq    %rax, core_dbg_free_count(%rip)
    movq    %rax, core_dbg_live_bytes(%rip)
    movq    %rax, core_dbg_double_free_count(%rip)
    movq    %rax, core_dbg_poison_bytes_total(%rip)
    ret
/* int core_dbg_allocations(void) */
    .globl  core_dbg_allocations
core_dbg_allocations:
    movq    core_dbg_alloc_count(%rip), %rax
    ret

/* int core_dbg_frees(void) —— 真正释放的次数（不含 null / cap==0 的静默返回） */
    .globl  core_dbg_frees
core_dbg_frees:
    movq    core_dbg_free_count(%rip), %rax
    ret

/* int core_dbg_double_frees(void) —— 被检测到的重复释放次数（D17） */
    .globl  core_dbg_double_frees
core_dbg_double_frees:
    movq    core_dbg_double_free_count(%rip), %rax
    ret

/* int core_dbg_poison_bytes(void) —— 释放前填 0xDD 的总字节数（D17） */
    .globl  core_dbg_poison_bytes
core_dbg_poison_bytes:
    movq    core_dbg_poison_bytes_total(%rip), %rax
    ret

/* void core_dbg_bump_alloc(int n) —— 由 core_alloc 调用 */
    .globl  core_dbg_bump_alloc
core_dbg_bump_alloc:
    lock addq %rcx, core_dbg_alloc_count(%rip)
    ret

/* void core_dbg_bump_free(int n) —— 由 core_free 调用 */
    .globl  core_dbg_bump_free
core_dbg_bump_free:
    lock addq %rcx, core_dbg_free_count(%rip)
    ret

/* ============================================================
 * int core_io_write(int fd, const void* buf, size_t n)
 *   rcx = fd, rdx = buf, r8 = n
 * 用 WriteFile 写到对应句柄。fd 0/1/2 映射到标准句柄。
 * 返回写入字节数，失败返回 -1。
 * ============================================================ */
/* P14-1a：stdout/stderr 全局输出锁（自旋锁，写一次持锁一次，Q14=C）。 */
    .data
    .align  4
core_io_spin:
    .zero   4
    .text
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
    /* P14-1a：加全局输出锁（xchg 只改 eax，ecx=fd 保持） */
    movl    $1, %eax
.Lio_w_spin:
    xchgl   %eax, core_io_spin(%rip)
    testl   %eax, %eax
    jz      .Lio_w_locked
    pause
    movl    $1, %eax
    jmp     .Lio_w_spin
.Lio_w_locked:

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
    xorl    %edx, %edx
    xchgl   %edx, core_io_spin(%rip)   /* 解锁（edx 已不用） */
    addq    $88, %rsp
    ret
.Lio_w_fail:
    movq    $-1, %rax
    xorl    %edx, %edx
    xchgl   %edx, core_io_spin(%rip)
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
    /* P14-1a：加全局输出锁（xchg 只改 eax；dl=ch、ecx=fd 保持） */
    movl    $1, %eax
.Lio_wb_spin:
    xchgl   %eax, core_io_spin(%rip)
    testl   %eax, %eax
    jz      .Lio_wb_locked
    pause
    movl    $1, %eax
    jmp     .Lio_wb_spin
.Lio_wb_locked:

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
    xorl    %edx, %edx
    xchgl   %edx, core_io_spin(%rip)
    addq    $88, %rsp
    ret
.Lio_wb_fail:
    movq    $-1, %rax
    xorl    %edx, %edx
    xchgl   %edx, core_io_spin(%rip)
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
    .align  8
core_eh_tls_index:               /* DWORD：TLS 槽索引（存每线程 eh 链头指针） */
    .zero   4
core_eh_tls_inited:
    .zero   4
core_eh_tls_spin:                /* 一次性初始化自旋锁 */
    .zero   4
    .zero   4

    .text
/* ------------------------------------------------------------
 * P14-1a：core_eh_get_slot() -> rax = 本线程 eh 链头槽地址（8 字节）。
 * 惰性 TlsAlloc + HeapAlloc(8) + TlsSetValue，分配后清零。
 * 破坏 rax/rcx/rdx/r8；不碰 callee-saved。进入 rsp≡8，
 * push rbx -> 0；sub $40 -> 0；内部 call 时 rsp≡0 ✓。
 * ------------------------------------------------------------ */
core_eh_get_slot:
    pushq   %rbx
    subq    $40, %rsp
    cmpl    $0, core_eh_tls_inited(%rip)
    jne     .Lesh_have
    movl    $1, %eax
.Lesh_spin:
    xchgl   %eax, core_eh_tls_spin(%rip)
    testl   %eax, %eax
    jz      .Lesh_spin_got
    pause
    jmp     .Lesh_spin
.Lesh_spin_got:
    cmpl    $0, core_eh_tls_inited(%rip)
    jne     .Lesh_init_done
    call    TlsAlloc
    movl    %eax, core_eh_tls_index(%rip)
    movl    $1, core_eh_tls_inited(%rip)
.Lesh_init_done:
    xorl    %eax, %eax
    xchgl   %eax, core_eh_tls_spin(%rip)
.Lesh_have:
    movl    core_eh_tls_index(%rip), %ecx
    call    TlsGetValue
    testq   %rax, %rax
    jnz     .Lesh_ret
    call    GetProcessHeap
    movq    %rax, %rbx
    xorl    %edx, %edx
    movq    $8, %r8
    movq    %rbx, %rcx
    call    HeapAlloc
    movq    %rax, %rbx
    testq   %rbx, %rbx
    je      .Lesh_ret
    movq    $0, (%rbx)                   /* slot = null（无 handler） */
    movl    core_eh_tls_index(%rip), %ecx
    movq    %rbx, %rdx
    call    TlsSetValue
    movq    %rbx, %rax
.Lesh_ret:
    addq    $40, %rsp
    popq    %rbx
    ret

/* void* core_eh_push(void* frame) — rcx=frame；rax=frame */
    .globl  core_eh_push
core_eh_push:
    pushq   %rbx
    pushq   %rdi
    subq    $40, %rsp
    movq    %rcx, %rbx                  /* rbx = frame */
    call    core_eh_get_slot
    movq    %rax, %rdi                  /* rdi = slot */
    movq    (%rdi), %rax                /* rax = 旧栈顶 */
    movq    %rax, (%rbx)                /* frame->prev = 旧栈顶 */
    movq    %rbx, (%rdi)                /* slot = frame */
    movq    %rbx, %rax                  /* 返回 frame */
    addq    $40, %rsp
    popq    %rdi
    popq    %rbx
    ret

/* void core_eh_pop(void* frame) — rcx=frame */
    .globl  core_eh_pop
core_eh_pop:
    pushq   %rbx
    pushq   %rdi
    subq    $40, %rsp
    movq    %rcx, %rbx
    call    core_eh_get_slot
    movq    %rax, %rdi
    movq    (%rbx), %rax                /* frame->prev */
    movq    %rax, (%rdi)                /* slot = prev */
    addq    $40, %rsp
    popq    %rdi
    popq    %rbx
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
    /* 保存 exc（rcx）到 rbx（get_slot 会破坏 rcx） */
    pushq   %rbx
    subq    $40, %rsp
    movq    %rcx, %rbx                  /* rbx = exc */
    call    core_eh_get_slot
    movq    (%rax), %rax                /* rax = 本线程链头 */
    testq   %rax, %rax
    je      .Lthrow_uncaught            /* 链空 -> 未捕获 */
    /* 先在栈未恢复时把 exc 写进帧（此时 rbx 仍是 exc） */
    movq    %rbx, 32(%rax)              /* frame->exc = exc */
    movq    8(%rax), %rbp               /* 恢复 %rbp */
    movq    16(%rax), %rcx              /* 取 saved rsp */
    movq    24(%rax), %rdx              /* 取 resume 地址 */
    addq    $40, %rsp
    popq    %rbx
    movq    %rcx, %rsp                  /* 恢复 %rsp（帧字段已读完） */
    jmp     *%rdx
.Lthrow_uncaught:
    addq    $40, %rsp
    popq    %rbx
    jmp     .Leh_uncaught

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
 * P13-6：Pool 三档真实分配器（Q1=A，默认 chunked）
 *
 * 池头 48 字节（8 字节对齐，地址稳定，永不搬动）：
 *   0  mode      1=fixed 2=boxed 3=chunked
 *   8  cap       fixed=容量 / chunked=每块槽数 / boxed=忽略
 *   16 elem_size 首个 new[T] 时写入（0=未定）
 *   24 used_in_chunk 当前块已用槽数
 *   32 current_chunk 当前块指针（0=未分配）；块布局 [prev_ptr][slots...]
 *   40 total     已分配槽总数
 *
 * 槽地址 = current_chunk + 8 + used_in_chunk*elem_size，一旦分配永不搬家。
 * 所有档位共用一套块链代码，fixed 与 boxed 只在「何时开新块」上分化。
 * ============================================================ */

/* void* core_pool_new(mode, cap) — rcx=mode, rdx=cap -> rax=池头 */
    .globl  core_pool_new
core_pool_new:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $64, %rsp
    movq    %rcx, %rbx          /* rbx = mode */
    movq    %rdx, %rsi          /* rsi = cap / block */
    movq    $48, %rcx
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lpn_fail
    movq    %rax, %rdi          /* rdi = pool */
    movq    %rax, %rcx
    xorl    %edx, %edx
    movq    $48, %r8
    call    core_mem_set        /* 池头清零 */
    movq    %rbx, 0(%rdi)
    movq    %rsi, 8(%rdi)
    movq    %rdi, %rax
    jmp     .Lpn_out
.Lpn_fail:
    xorl    %eax, %eax
.Lpn_out:
    addq    $64, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* void* core_pool_alloc(pool, elem_size) — rcx=pool, rdx=elem_size -> rax=槽地址
 * 满（fixed 达容量）分配失败返回 0，由 codegen 报诊断。 */
    .globl  core_pool_alloc
core_pool_alloc:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $64, %rsp
    movq    %rcx, %rbx          /* rbx = pool */
    movq    %rdx, %rsi          /* rsi = elem_size */
    testq   %rbx, %rbx
    je      .Lpa_fail
    testq   %rsi, %rsi
    je      .Lpa_fail
    movq    16(%rbx), %rax      /* 首个 new[T] 时定下 elem_size */
    testq   %rax, %rax
    jne     .Lpa_have_size
    movq    %rsi, 16(%rbx)
.Lpa_have_size:
    movq    16(%rbx), %rsi      /* rsi = 已定 elem_size */
    movq    0(%rbx), %rax       /* mode */
    cmpq    $2, %rax
    je      .Lpa_boxed
    movq    8(%rbx), %rdi       /* fixed/chunked: 每块槽数 = cap */
    jmp     .Lpa_slots_ready
.Lpa_boxed:
    movq    $1, %rdi
.Lpa_slots_ready:
    movq    %rdi, 56(%rsp)      /* slots_per_chunk 栈槽 */
    movq    32(%rbx), %rax      /* current_chunk */
    testq   %rax, %rax
    je      .Lpa_new_chunk
    movq    24(%rbx), %rax      /* used_in_chunk */
    cmpq    56(%rsp), %rax
    jb      .Lpa_have_slot      /* used < slots -> 复用当前块 */
    movq    0(%rbx), %rax
    cmpq    $1, %rax            /* fixed 满了 -> 失败 */
    je      .Lpa_fail
.Lpa_new_chunk:
    movq    56(%rsp), %rcx
    imulq   %rsi, %rcx          /* slots * elem_size */
    addq    $8, %rcx            /* + prev 指针 */
    movq    $8, %rdx
    call    core_alloc
    testq   %rax, %rax
    je      .Lpa_fail
    movq    %rax, %rdi          /* rdi = 新块 */
    movq    32(%rbx), %rax
    movq    %rax, 0(%rdi)       /* 新块->prev = 旧块 */
    movq    %rdi, 32(%rbx)      /* current = 新块 */
    movq    $0, 24(%rbx)        /* used_in_chunk = 0 */
.Lpa_have_slot:
    movq    32(%rbx), %rdi
    addq    $8, %rdi            /* 跳过 prev 指针 */
    movq    24(%rbx), %rax
    imulq   %rsi, %rax
    addq    %rax, %rdi          /* slot = chunk + 8 + used*elem_size */
    incq    24(%rbx)
    incq    40(%rbx)
    movq    %rdi, %rax
    jmp     .Lpa_out
.Lpa_fail:
    xorl    %eax, %eax
.Lpa_out:
    addq    $64, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* void core_pool_release(pool, slot) — rcx=pool, rdx=slot
 * 运行时只做记账；§6.4 的成员集合判据在编译期。boxed 档顺带释放单槽块。 */
    .globl  core_pool_release
core_pool_release:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $64, %rsp
    movq    %rcx, %rbx
    movq    %rdx, %rsi
    testq   %rbx, %rbx
    je      .Lpr_out
    testq   %rsi, %rsi
    je      .Lpr_out
    decq    40(%rbx)
    movq    0(%rbx), %rax
    cmpq    $2, %rax
    jne     .Lpr_out
    movq    %rsi, %rcx
    subq    $8, %rcx
    call    core_free
.Lpr_out:
    addq    $64, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* ============================================================
 * P14-3：Pool 枚举原语（par_each / pool.at）
 *
 * 池头沿用 P13-6 布局：+16 elem_size、+24 used_in_chunk、
 * +32 current_chunk、+40 total。块布局 [prev_ptr][slots...]，
 * 每块 slots_per_chunk 个槽（fixed/chunked=cap，boxed=1）；
 * 除当前块外所有块都是满的。槽地址 = chunk + 8 + local*elem_size。
 * 注意：这些原语按「分配序号 0..total-1」枚举，若中间 release 过槽，
 * total 会减小但数组仍可能有空洞 —— par_each 首版假定迭代期间不改结构、
 * 且池未发生 release（成员集合快照语义，见 p14_3 方案）。
 * ============================================================ */

/* int core_pool_count(pool) — rcx=pool -> rax=total（已分配槽数） */
    .globl  core_pool_count
core_pool_count:
    testq   %rcx, %rcx
    je      .Lpc_null
    movq    40(%rcx), %rax
    ret
.Lpc_null:
    xorl    %eax, %eax
    ret

/* void* core_pool_slot_at(pool, index) — rcx=pool, rdx=index -> rax=槽地址
 * index 越界（>= total）返回 0。沿 current_chunk->prev 链定位所在块。 */
    .globl  core_pool_slot_at
core_pool_slot_at:
    movq    %rcx, %r8          /* r8 = pool */
    testq   %r8, %r8
    je      .Lpsa_fail
    movq    40(%r8), %rax      /* rax = total */
    cmpq    %rax, %rdx
    jae     .Lpsa_fail         /* index >= total -> 0 */
    movq    24(%r8), %r9       /* 当前块已用槽数 u */
    movq    32(%r8), %r10      /* r10 = current_chunk */
    movq    %rax, %r11
    subq    %r9, %r11          /* r11 = 当前块首槽的全局序号 = total - u */
    /* slots_per_chunk：boxed=1，其余=cap */
    movq    0(%r8), %rax       /* mode */
    cmpq    $2, %rax
    je      .Lpsa_boxed
    movq    8(%r8), %rax       /* cap */
    jmp     .Lpsa_cap_ready
.Lpsa_boxed:
    movq    $1, %rax
.Lpsa_cap_ready:
    movq    %rax, %r9          /* r9 = slots_per_chunk */
.Lpsa_loop:
    cmpq    %r11, %rdx
    jae     .Lpsa_found        /* index >= 块首序号 -> 落在本块 */
    movq    (%r10), %r10       /* 沿 prev 往前一块 */
    testq   %r10, %r10
    je      .Lpsa_fail         /* 链意外断裂 */
    subq    %r9, %r11          /* 块首序号 -= 每块槽数 */
    jmp     .Lpsa_loop
.Lpsa_found:
    movq    %rdx, %rax
    subq    %r11, %rax         /* rax = 块内局部序号 */
    movq    16(%r8), %rcx      /* rcx = elem_size */
    imulq   %rcx, %rax
    addq    %r10, %rax
    addq    $8, %rax           /* 跳过 prev 指针 */
    ret
.Lpsa_fail:
    xorl    %eax, %eax
    ret

/* ============================================================
 * P13-7：Link 成对引用运行时（规范 §7）
 *
 * 用一张全局边表存所有 Link 边。每条边 32 字节：
 *   +0  aSlot      +8  aField
 *   +16 bSlot      +24 bField
 * 其中 aSlot/bSlot 是池槽地址，aField/bField 是 Link 字段索引（编译期分配）。
 * 边守恒：a.L 含 b ⟺ b.L' 含 a —— add/remove/clear 都同时维护两端。
 * 容量固定 4096 条（P13-7 不做动态扩容；够覆盖回归用例与典型图）。
 * ============================================================ */
    .data
    .align 8
    .globl  core_link_edges
core_link_edges:
    .space  131072                  /* 4096 * 32 */
    .globl  core_link_edge_count
core_link_edge_count:
    .quad   0

    .section .rdata,"dr"
core_link_msg_crosspool:
    .ascii  "cross-pool link"
core_link_msg_full:
    .ascii  "link endpoint not Full"
core_link_msg_dup:
    .ascii  "link already connected"
core_link_msg_overflow:
    .ascii  "link table overflow"

    .text
/* 内部：panic 辅助。rcx = msg, rdx = len。 */
core_link_panic:
    subq    $40, %rsp
    movq    %rcx, %r9
    movq    %rdx, %r10
    call    core_panic_raise
    addq    $40, %rsp
    ret

/* 内部：int core_link_slot_in_pool(pool, slot)
 *   rcx=pool, rdx=slot -> rax=1 属于该池 / 0 不属于。
 * 沿池头的块链（+32 current，块 +0 prev，槽从 +8 起，每块 cap=+8 个 elem_size=+16 字节槽）。 */
core_link_slot_in_pool:
    pushq   %rbx
    pushq   %rsi
    movq    %rcx, %rbx            /* rbx = pool */
    movq    %rdx, %rsi            /* rsi = slot */
    movq    16(%rbx), %r8         /* r8 = elem_size */
    movq    8(%rbx), %r9          /* r9 = cap (slots per chunk) */
    imulq   %r9, %r8              /* r8 = chunk 数据区字节数 */
    movq    32(%rbx), %rdx        /* rdx = current chunk */
.Llsip_loop:
    testq   %rdx, %rdx
    je      .Llsip_no
    leaq    8(%rdx), %rax         /* 数据区起点 */
    cmpq    %rax, %rsi
    jb      .Llsip_next           /* slot < start */
    leaq    8(%rdx), %rax
    addq    %r8, %rax             /* 数据区末尾 */
    cmpq    %rax, %rsi
    jae     .Llsip_next           /* slot >= end */
    movq    $1, %rax
    jmp     .Llsip_out
.Llsip_next:
    movq    0(%rdx), %rdx         /* prev chunk */
    jmp     .Llsip_loop
.Llsip_no:
    xorl    %eax, %eax
.Llsip_out:
    popq    %rsi
    popq    %rbx
    ret

/* void core_link_add(pool, aSlot, aField, bSlot, bField)
 *   rcx=pool, rdx=aSlot, r8=aField, r9=bSlot, 5th=bField (stack) */
    .globl  core_link_add
core_link_add:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    pushq   %r12
    pushq   %r13
    subq    $64, %rsp
    movq    %rcx, %rbx             /* rbx = pool */
    movq    %rdx, %rsi             /* rsi = aSlot */
    movq    %r8, %rdi              /* rdi = aField */
    movq    %r9, %r12              /* r12 = bSlot */
    /* 5th 栈参 bField：入口 rsp+40，现 rsp 已下移 5*8+64=104 */
    movq    144(%rsp), %rax
    movq    %rax, %r10             /* r10 = bField */
    /* Q6=A：同池校验 —— 两端槽都必须落在本 pool 的块链里，否则 cross-pool panic。 */
    movq    %rbx, %rcx
    movq    %rsi, %rdx
    call    core_link_slot_in_pool
    testq   %rax, %rax
    je      .Llkadd_crosspanic
    movq    %rbx, %rcx
    movq    %r12, %rdx
    call    core_link_slot_in_pool
    testq   %rax, %rax
    je      .Llkadd_crosspanic
    /* 去重：扫描边表，若 (a,aField,b,bField) 已存在则 panic */
    movq    core_link_edge_count(%rip), %r13  /* 循环计数 */
    testq   %r13, %r13
    je      .Llkadd_append
    leaq    core_link_edges(%rip), %r11
.Llkadd_dup_loop:
    /* 正向 */
    movq    0(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llkadd_dup_rev
    movq    8(%r11), %rax
    cmpq    %rdi, %rax
    jne     .Llkadd_dup_rev
    movq    16(%r11), %rax
    cmpq    %r12, %rax
    jne     .Llkadd_dup_rev
    movq    24(%r11), %rax
    cmpq    %r10, %rax
    je      .Llkadd_duppanic
.Llkadd_dup_rev:
    /* 反向（对称边只存一条，b→a 也算已连） */
    movq    0(%r11), %rax
    cmpq    %r12, %rax
    jne     .Llkadd_dup_next
    movq    8(%r11), %rax
    cmpq    %r10, %rax
    jne     .Llkadd_dup_next
    movq    16(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llkadd_dup_next
    movq    24(%r11), %rax
    cmpq    %rdi, %rax
    je      .Llkadd_duppanic
.Llkadd_dup_next:
    addq    $32, %r11
    decq    %r13
    testq   %r13, %r13
    jne     .Llkadd_dup_loop
.Llkadd_append:
    movq    core_link_edge_count(%rip), %rax
    cmpq    $4096, %rax
    jae     .Llkadd_overflow
    leaq    core_link_edges(%rip), %r11
    imulq   $32, %rax
    addq    %rax, %r11
    movq    %rsi, 0(%r11)
    movq    %rdi, 8(%r11)
    movq    %r12, 16(%r11)
    movq    %r10, 24(%r11)
    incq    core_link_edge_count(%rip)
    jmp     .Llkadd_out
.Llkadd_crosspanic:
    leaq    core_link_msg_crosspool(%rip), %rcx
    movq    $15, %rdx
    call    core_link_panic
.Llkadd_duppanic:
    leaq    core_link_msg_dup(%rip), %rcx
    movq    $22, %rdx
    call    core_link_panic
.Llkadd_overflow:
    leaq    core_link_msg_overflow(%rip), %rcx
    movq    $19, %rdx
    call    core_link_panic
.Llkadd_out:
    addq    $64, %rsp
    popq    %r13
    popq    %r12
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* 内部：删除第 i 条边（把最后一条搬过来），rcx=i */
core_link_remove_at:
    movq    core_link_edge_count(%rip), %rax
    decq    %rax
    cmpq    %rax, %rcx
    je      .Llkra_done
    leaq    core_link_edges(%rip), %rdx
    imulq   $32, %rcx
    addq    %rcx, %rdx            /* 目标槽 */
    imulq   $32, %rax
    leaq    core_link_edges(%rip), %r8
    addq    %r8, %rax            /* 最后一条槽 */
    movq    0(%rax), %r8
    movq    %r8, 0(%rdx)
    movq    8(%rax), %r8
    movq    %r8, 8(%rdx)
    movq    16(%rax), %r8
    movq    %r8, 16(%rdx)
    movq    24(%rax), %r8
    movq    %r8, 24(%rdx)
.Llkra_done:
    decq    core_link_edge_count(%rip)
    ret

/* void core_link_set(pool, aSlot, aField, bSlot, bField)
 * P13-7 完整化（§7.4/§7.7）：set 到新对端前，先删除所有以 (aSlot,aField)
 * 为**任一端**的旧边（对称边只存一条，正向/反向都要匹配），再建立新边。
 * 这样旧对端的对偶字段同步清空，不会残留「a 同时连两人」。 */
    .globl  core_link_set
core_link_set:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    pushq   %r12
    pushq   %r13
    subq    $64, %rsp
    movq    %rdx, %rsi             /* rsi = aSlot */
    movq    %r8, %rdi              /* rdi = aField */
    movq    %r9, %r12              /* r12 = bSlot */
    movq    144(%rsp), %r10        /* 5th 栈参 bField（5 push + 64 = 104） */
    movq    %rcx, %r13             /* r13 = pool */
.Llks_restart:
    movq    core_link_edge_count(%rip), %rax
    testq   %rax, %rax
    je      .Llks_addnew
    leaq    core_link_edges(%rip), %r11
    xorl    %ebx, %ebx             /* i */
.Llks_loop:
    /* 正向：edge.a==aSlot && edge.aField==aField */
    movq    0(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llks_reverse
    movq    8(%r11), %rax
    cmpq    %rdi, %rax
    je      .Llks_found
.Llks_reverse:
    /* 反向：edge.b==aSlot && edge.bField==aField（a 曾作对端） */
    movq    16(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llks_next
    movq    24(%r11), %rax
    cmpq    %rdi, %rax
    je      .Llks_found
.Llks_next:
    addq    $32, %r11
    incq    %rbx
    movq    core_link_edge_count(%rip), %rax
    cmpq    %rbx, %rax
    ja      .Llks_loop
    jmp     .Llks_addnew
.Llks_found:
    movq    %rbx, %rcx
    call    core_link_remove_at
    jmp     .Llks_restart           /* swap-remove 后重扫 */
.Llks_addnew:
    /* 恢复寄存器，尾调用 core_link_add(pool,a,aF,b,bF) */
    movq    %r13, %rcx
    movq    %rsi, %rdx
    movq    %rdi, %r8
    movq    %r12, %r9
    /* bField 仍在 %r10，写到新调用的 5th 栈参位置（rsp+32） */
    movq    %r10, 32(%rsp)
    call    core_link_add
    addq    $64, %rsp
    popq    %r13
    popq    %r12
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* void core_link_remove(pool, aSlot, aField, bSlot, bField)
 * 删除两端对应的那一条边（成对，单边不可构造）。 */
    .globl  core_link_remove
core_link_remove:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    pushq   %r12
    subq    $64, %rsp
    movq    %rdx, %rsi
    movq    %r8, %rdi
    movq    %r9, %r12
    movq    136(%rsp), %r10        /* 5th 栈参 bField（4 push + 64 sub = 96） */
    movq    core_link_edge_count(%rip), %rax
    testq   %rax, %rax
    je      .Llkrm_out
    leaq    core_link_edges(%rip), %r11
    xorl    %ebx, %ebx            /* i */
.Llkrm_loop:
    /* 正向匹配 (a,aF)-(b,bF) */
    movq    0(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llkrm_reverse
    movq    8(%r11), %rax
    cmpq    %rdi, %rax
    jne     .Llkrm_reverse
    movq    16(%r11), %rax
    cmpq    %r12, %rax
    jne     .Llkrm_reverse
    movq    24(%r11), %rax
    cmpq    %r10, %rax
    je      .Llkrm_found
.Llkrm_reverse:
    /* 反向匹配 (b,bF)-(a,aF) */
    movq    0(%r11), %rax
    cmpq    %r12, %rax
    jne     .Llkrm_next
    movq    8(%r11), %rax
    cmpq    %r10, %rax
    jne     .Llkrm_next
    movq    16(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llkrm_next
    movq    24(%r11), %rax
    cmpq    %rdi, %rax
    jne     .Llkrm_next
.Llkrm_found:
    movq    %rbx, %rcx
    call    core_link_remove_at
    jmp     .Llkrm_out              /* swap-remove 后索引变化，直接结束 */
.Llkrm_next:
    addq    $32, %r11
    incq    %rbx
    movq    core_link_edge_count(%rip), %rax
    cmpq    %rbx, %rax
    ja      .Llkrm_loop
.Llkrm_out:
    addq    $64, %rsp
    popq    %r12
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* void core_link_clear(pool, aSlot, aField, bField)
 * 删除所有以 (aSlot, aField) 为一端的边（每条边同时代表对端，删一次即两端同步）。
 * bField 参数保留在签名里以保持接口统一，实现里不需要。 */
    .globl  core_link_clear
core_link_clear:
    pushq   %rbx
    pushq   %rsi
    pushq   %rdi
    subq    $40, %rsp
    movq    %rdx, %rsi            /* aSlot */
    movq    %r8, %rdi             /* aField */
.Llkclr_restart:
    movq    core_link_edge_count(%rip), %rax
    testq   %rax, %rax
    je      .Llkclr_out
    leaq    core_link_edges(%rip), %r11
    xorl    %ebx, %ebx
.Llkclr_loop:
    movq    0(%r11), %rax
    cmpq    %rsi, %rax
    jne     .Llkclr_next
    movq    8(%r11), %rax
    cmpq    %rdi, %rax
    je      .Llkclr_found
.Llkclr_next:
    addq    $32, %r11
    incq    %rbx
    movq    core_link_edge_count(%rip), %rax
    cmpq    %rbx, %rax
    ja      .Llkclr_loop
    jmp     .Llkclr_out
.Llkclr_found:
    movq    %rbx, %rcx
    call    core_link_remove_at
    jmp     .Llkclr_restart          /* swap-remove 后重扫，直到无匹配 */
.Llkclr_out:
    addq    $40, %rsp
    popq    %rdi
    popq    %rsi
    popq    %rbx
    ret

/* 直接删除所有 (aSlot==rcx) 或 (bSlot==rcx) 的边 —— core_link_unlink_all。
 * 规范 §7.2：清空 a 的所有 Link 字段，两端同步。 */
    .globl  core_link_unlink_all
core_link_unlink_all:
    pushq   %rbx
    pushq   %rsi
    subq    $40, %rsp
    /* 签名 (pool, a)：a 是第 2 参 %rdx（%rcx 是 pool，这里不用） */
    movq    %rdx, %rsi            /* target slot */
.Llkua_restart:
    movq    core_link_edge_count(%rip), %rax
    testq   %rax, %rax
    je      .Llkua_out
    leaq    core_link_edges(%rip), %r11
    xorl    %ebx, %ebx
.Llkua_loop:
    movq    0(%r11), %rax
    cmpq    %rsi, %rax
    je      .Llkua_found
    movq    16(%r11), %rax
    cmpq    %rsi, %rax
    je      .Llkua_found
.Llkua_next:
    addq    $32, %r11
    incq    %rbx
    movq    core_link_edge_count(%rip), %rax
    cmpq    %rbx, %rax
    ja      .Llkua_loop
    jmp     .Llkua_out
.Llkua_found:
    /* 内联 swap-remove，避免嵌套 call 破坏 rcx/栈约定 */
    movq    core_link_edge_count(%rip), %rax
    decq    %rax
    cmpq    %rax, %rbx
    je      .Llkua_pop
    leaq    core_link_edges(%rip), %rdx
    imulq   $32, %rbx
    addq    %rbx, %rdx
    imulq   $32, %rax
    leaq    core_link_edges(%rip), %r8
    addq    %r8, %rax
    movq    0(%rax), %r8
    movq    %r8, 0(%rdx)
    movq    8(%rax), %r8
    movq    %r8, 8(%rdx)
    movq    16(%rax), %r8
    movq    %r8, 16(%rdx)
    movq    24(%rax), %r8
    movq    %r8, 24(%rdx)
.Llkua_pop:
    decq    core_link_edge_count(%rip)
    jmp     .Llkua_restart
.Llkua_out:
    addq    $40, %rsp
    popq    %rsi
    popq    %rbx
    ret

/* int core_link_count(slot, field) — rcx=slot, rdx=field -> rax=该字段的链接数 */
    .globl  core_link_count
core_link_count:
    pushq   %rbx
    movq    %rcx, %rbx            /* slot */
    xorl    %eax, %eax            /* count */
    movq    core_link_edge_count(%rip), %r8
    testq   %r8, %r8
    je      .Llkcnt_out
    leaq    core_link_edges(%rip), %r9
.Llkcnt_loop:
    /* 匹配 (slot,field) 在边的任一端（对称边只存一条） */
    movq    0(%r9), %rcx
    cmpq    %rbx, %rcx
    jne     .Llkcnt_as_b
    movq    8(%r9), %rcx
    cmpq    %rdx, %rcx
    je      .Llkcnt_match
.Llkcnt_as_b:
    movq    16(%r9), %rcx
    cmpq    %rbx, %rcx
    jne     .Llkcnt_next
    movq    24(%r9), %rcx
    cmpq    %rdx, %rcx
    jne     .Llkcnt_next
.Llkcnt_match:
    incq    %rax
.Llkcnt_next:
    addq    $32, %r9
    decq    %r8
    testq   %r8, %r8
    jne     .Llkcnt_loop
.Llkcnt_out:
    popq    %rbx
    ret

/* int core_link_has(slot, field, peerSlot) — rcx=slot, rdx=field, r8=peerSlot -> rax=1/0 */
    .globl  core_link_has
core_link_has:
    movq    core_link_edge_count(%rip), %r9
    testq   %r9, %r9
    je      .Llkhas_no
    leaq    core_link_edges(%rip), %r10
.Llkhas_loop:
    /* slot/field 在任一端，peer 在对端 */
    movq    0(%r10), %rax
    cmpq    %rcx, %rax
    jne     .Llkhas_as_b
    movq    8(%r10), %rax
    cmpq    %rdx, %rax
    jne     .Llkhas_as_b
    movq    16(%r10), %rax
    cmpq    %r8, %rax
    je      .Llkhas_yes
    jmp     .Llkhas_next
.Llkhas_as_b:
    movq    16(%r10), %rax
    cmpq    %rcx, %rax
    jne     .Llkhas_next
    movq    24(%r10), %rax
    cmpq    %rdx, %rax
    jne     .Llkhas_next
    movq    0(%r10), %rax
    cmpq    %r8, %rax
    je      .Llkhas_yes
.Llkhas_next:
    addq    $32, %r10
    decq    %r9
    testq   %r9, %r9
    jne     .Llkhas_loop
.Llkhas_no:
    xorl    %eax, %eax
    ret
.Llkhas_yes:
    movq    $1, %rax
    ret

/* int core_link_is_empty(slot) — rcx=slot -> rax: 1 空 / 0 非空 */
    .globl  core_link_is_empty
core_link_is_empty:
    movq    core_link_edge_count(%rip), %rdx
    testq   %rdx, %rdx
    je      .Llkie_empty
    leaq    core_link_edges(%rip), %r11
.Llkie_loop:
    movq    0(%r11), %rax
    cmpq    %rcx, %rax
    je      .Llkie_nonempty
    movq    16(%r11), %rax
    cmpq    %rcx, %rax
    je      .Llkie_nonempty
    addq    $32, %r11
    decq    %rdx
    testq   %rdx, %rdx
    jne     .Llkie_loop
.Llkie_empty:
    movq    $1, %rax
    ret
.Llkie_nonempty:
    xorl    %eax, %eax
    ret

/* ============================================================
 * P14-1a：结构化并发运行时地基（线程原语，1:1 OS 线程，Q1=A）
 *
 * void* core_thread_create(void* fn, void* arg)
 *   rcx = fn（线程入口，MS x64 ABI：arg 在 %rcx 传入）
 *   rdx = arg
 *   返回 HANDLE（ptr），失败返回 null。
 *
 * void core_thread_join(void* handle)
 *   rcx = HANDLE；等待线程结束并 CloseHandle；null 直接返回。
 *
 * Win32 x64 栈对齐约定：调用者进函数时 %rsp≡8 (mod 16)；进入本函数后
 * 第一个 push 使 %rsp≡0。CreateThread 有 6 个参数，需 32 字节 shadow +
 * 2 个栈参数（lpThreadId、预留），共 48 字节，必须保持 %rsp≡0 (mod 16)
 * 再 call（call 压返回地址后被调方看到 ≡8，符合 ABI）。
 * ============================================================ */
    .globl  core_thread_create
core_thread_create:
    pushq   %rbx                 /* %rsp: 8 -> 0 (mod 16)，rbx callee-saved */
    pushq   %r12                 /* %rsp: 0 -> 8；再 push 到 0 需偶数个 push */
    pushq   %r13                 /* %rsp: 8 -> 0 */
    subq    $48, %rsp            /* 32 shadow + 2 栈参(16) = 48，保持 ≡0 */
    movq    %rcx, %r12           /* r12 = fn（Rio 函数值 = 闭包对象） */
    /* P14-1a：Rio 的具名函数值 emit 成闭包对象 .Lfnclosure_f: .quad f，
     * 首字是真实入口地址。解包一次；首版只支持无捕获函数（Q3 裁决：
     * 只 spawn 具名顶层函数）。带捕获闭包需不同约定，不在本阶段范围。 */
    movq    (%r12), %r12         /* r12 = 真实函数入口 */
    movq    %rdx, %r13           /* r13 = arg */
    xorl    %ecx, %ecx           /* lpThreadAttributes = NULL */
    xorl    %edx, %edx           /* dwStackSize = 0（默认） */
    movq    %r12, %r8            /* lpStartAddress = fn */
    movq    %r13, %r9            /* lpParameter = arg */
    xorl    %eax, %eax           /* rax = 0 */
    movq    %rax, 32(%rsp)       /* 参数5 dwCreationFlags = 0，立即运行 */
    movq    %rax, 40(%rsp)       /* 参数6 lpThreadId = NULL（不需要 id） */
    call    CreateThread
    addq    $48, %rsp
    popq    %r13
    popq    %r12
    popq    %rbx
    ret                          /* rax = HANDLE 或 null */

    .globl  core_thread_join
core_thread_join:
    testq   %rcx, %rcx
    je      .Lthread_join_ret    /* null -> 直接返回 */
    pushq   %rbx                 /* %rsp: 8 -> 0 (mod 16) */
    subq    $32, %rsp            /* 32 shadow，保持 ≡0 */
    movq    %rcx, %rbx           /* rbx = handle */
    movq    %rbx, %rcx           /* hHandle */
    movl    $-1, %edx            /* INFINITE */
    call    WaitForSingleObject
    movq    %rbx, %rcx           /* hObject = handle */
    call    CloseHandle
    addq    $32, %rsp
    popq    %rbx
.Lthread_join_ret:
    ret

/* ============================================================
 * P14-4：显式同步运行时原语（Q6 / Q7）
 *   - 自旋锁：core_lock_acquire / core_lock_release
 *     锁字 4 字节：0=未锁，1=已锁。xchg 隐含 lock 语义（全屏障）。
 *   - 原子：i64 走 q 后缀，i32 走 l 后缀，全部顺序一致。
 *     load 用对齐 mov（x86-64 上 load 天然顺序一致）；store 用 xchg；
 *     fetch_add 用 lock xadd；swap 用 xchg；cas 用 lock cmpxchg；
 *     fetch_and/or/xor 用 lock cmpxchg 循环。
 *     所有原语只改 caller-saved 寄存器（rax/rcx/rdx/r8/r9/r10/r11）。
 * 参数与返回值遵循 Win64 ABI：rcx/rdx/r8/r9 传参，rax 返回；
 * 调用方已预留 32 字节 shadow space，本文件例程不自行调整 rsp。
 * ============================================================ */

/* void core_lock_acquire(ptr lockword) — rcx = 锁字地址 */
    .globl  core_lock_acquire
core_lock_acquire:
    movl    $1, %eax
.Lcla_spin:
    xchgl   %eax, (%rcx)        /* 交换：old -> eax，写入 1 */
    testl   %eax, %eax
    jz      .Lcla_got
    pause
    movl    $1, %eax            /* xchg 把 eax 改成 old，重入前重置为 1 */
    jmp     .Lcla_spin
.Lcla_got:
    ret

/* void core_lock_release(ptr lockword) — rcx = 锁字地址 */
    .globl  core_lock_release
core_lock_release:
    xorl    %eax, %eax
    xchgl   %eax, (%rcx)        /* 写 0，old 进 eax（忽略） */
    ret

/* ---- i64 原子（返回值走 rax，调用方按 int 使用） ---- */
/* long core_atomic_load_i64(ptr) */
    .globl  core_atomic_load_i64
core_atomic_load_i64:
    movq    (%rcx), %rax
    ret

/* void core_atomic_store_i64(ptr, long) */
    .globl  core_atomic_store_i64
core_atomic_store_i64:
    xchgq   %rdx, (%rcx)
    ret

/* long core_atomic_fetch_add_i64(ptr, long) */
    .globl  core_atomic_fetch_add_i64
core_atomic_fetch_add_i64:
    movq    %rdx, %rax
    lock xaddq %rax, (%rcx)
    ret

/* long core_atomic_swap_i64(ptr, long) */
    .globl  core_atomic_swap_i64
core_atomic_swap_i64:
    movq    %rdx, %rax
    xchgq   %rax, (%rcx)
    ret

/* long core_atomic_cas_i64(ptr, long expected, long new) — 返回读到的旧值 */
    .globl  core_atomic_cas_i64
core_atomic_cas_i64:
    movq    %rdx, %rax          /* expected -> rax，cmpxchg 的比较基准 */
    lock cmpxchgq %r8, (%rcx)
    ret                         /* rax = 实际旧值（成功时 == expected） */

/* long core_atomic_fetch_op_i64(ptr, long val, int op) — op: 0=and,1=or,2=xor */
    .globl  core_atomic_fetch_op_i64
core_atomic_fetch_op_i64:
    movq    %rdx, %r10          /* r10 = val */
    movl    %r8d, %r11d         /* r11 = op */
.Lcfo64_retry:
    movq    (%rcx), %rax        /* rax = old */
    movq    %rax, %rdx          /* rdx = new 计算区 */
    cmpl    $0, %r11d
    je      .Lcfo64_and
    cmpl    $1, %r11d
    je      .Lcfo64_or
    xorq    %r10, %rdx
    jmp     .Lcfo64_cmp
.Lcfo64_and:
    andq    %r10, %rdx
    jmp     .Lcfo64_cmp
.Lcfo64_or:
    orq     %r10, %rdx
.Lcfo64_cmp:
    lock cmpxchgq %rdx, (%rcx)
    jne     .Lcfo64_retry
    ret                         /* rax = 旧值 */

/* ---- i32 原子（返回值低 32 位在 eax，调用方 movslq 符号扩展） ---- */
/* int core_atomic_load_i32(ptr) */
    .globl  core_atomic_load_i32
core_atomic_load_i32:
    movl    (%rcx), %eax
    ret

/* void core_atomic_store_i32(ptr, int) */
    .globl  core_atomic_store_i32
core_atomic_store_i32:
    xchgl   %edx, (%rcx)
    ret

/* int core_atomic_fetch_add_i32(ptr, int) */
    .globl  core_atomic_fetch_add_i32
core_atomic_fetch_add_i32:
    movl    %edx, %eax
    lock xaddl %eax, (%rcx)
    ret

/* int core_atomic_swap_i32(ptr, int) */
    .globl  core_atomic_swap_i32
core_atomic_swap_i32:
    movl    %edx, %eax
    xchgl   %eax, (%rcx)
    ret

/* int core_atomic_cas_i32(ptr, int expected, int new) — 返回读到的旧值 */
    .globl  core_atomic_cas_i32
core_atomic_cas_i32:
    movl    %edx, %eax
    lock cmpxchgl %r8d, (%rcx)
    ret

/* int core_atomic_fetch_op_i32(ptr, int val, int op) — op: 0=and,1=or,2=xor */
    .globl  core_atomic_fetch_op_i32
core_atomic_fetch_op_i32:
    movl    %edx, %r10d
    movl    %r8d, %r11d
.Lcfo32_retry:
    movl    (%rcx), %eax
    movl    %eax, %edx
    cmpl    $0, %r11d
    je      .Lcfo32_and
    cmpl    $1, %r11d
    je      .Lcfo32_or
    xorl    %r10d, %edx
    jmp     .Lcfo32_cmp
.Lcfo32_and:
    andl    %r10d, %edx
    jmp     .Lcfo32_cmp
.Lcfo32_or:
    orl     %r10d, %edx
.Lcfo32_cmp:
    lock cmpxchgl %edx, (%rcx)
    jne     .Lcfo32_retry
    ret

/* ============================================================
 * P14-5：Chan 有界通道运行时原语（Q8 / 裁决 4-A~4-E）
 *
 *   对象布局（core_alloc 一块，8 字节对齐）：
 *     +0  lockword（4B，0=未锁/1=已锁）
 *     +8  capacity（非 0：core_free 用 cap==0 判静态头，这里必须非 0）
 *     +16 head（读游标）
 *     +24 tail（写游标）
 *     +32 count（当前元素数）
 *     +40 closed（0=开 / 1=关）
 *     +48 slots[capacity]（每槽 8 字节，载荷是不透明 8B 值）
 *
 *   并发：一把自旋锁保护所有字段；满/空时用 SwitchToThread() 让出时间片
 *   后重试（解锁后让出，绝不持锁等待）。close 幂等，唤醒阻塞的 recv。
 *   所有原语只改 caller-saved 寄存器（callee-saved 用 rbx/r12 均保存恢复）。
 * ============================================================ */

/* void* core_chan_new(int capacity) — rcx=capacity，返回对象指针 */
    .globl  core_chan_new
core_chan_new:
    pushq   %rbx                    /* rsp: 8 -> 0 */
    pushq   %r12                   /* rsp: 0 -> 8 */
    subq    $40, %rsp              /* 40 ≡ 8 (mod16)：rsp -> ≡0，全程保持该帧 */
    movq    %rcx, %rbx             /* rbx = capacity */
    testq   %rbx, %rbx
    jg      .Lchan5_new_cap_ok
    movq    $16, %rbx              /* capacity<=0 -> 默认 16（防御） */
.Lchan5_new_cap_ok:
    leaq    (,%rbx,8), %rax        /* rax = cap*8 */
    addq    $48, %rax              /* 对象大小 = 48 + cap*8 */
    movq    %rax, %rcx             /* size */
    movq    $8, %rdx               /* align */
    call    core_alloc             /* rsp≡0，32B shadow 在帧内 */
    testq   %rax, %rax
    je      .Lchan5_new_oom
    movq    %rax, %r12             /* r12 = obj（callee-saved，不覆盖 rbx=cap） */
    movq    %r12, %rcx             /* dst = obj */
    xorl    %edx, %edx             /* byte = 0 */
    movq    $48, %r8               /* n = 48 */
    call    core_mem_set           /* 清零头部 48B；帧仍在，rsp≡0 */
    movq    %r12, %rax
    movq    %rbx, 8(%rax)          /* cap（非 0 保证 core_free 真释放；rbx 仍是 capacity） */
    addq    $40, %rsp
    popq    %r12
    popq    %rbx
    ret
.Lchan5_new_oom:
    call    core_proc_abort        /* 帧仍在，rsp≡0 */
    hlt

/* void core_chan_send(ptr ch, ptr value) — rcx=ch, rdx=value
 * 满则解锁让出重试；已关闭则 abort（Q8=裁决 4-D）。
 * %rbx=ch, %r12=value, %r13=槽内偏移(callee-saved，跨 core_* 调用不丢)。 */
    .globl  core_chan_send
core_chan_send:
    pushq   %rbx
    pushq   %r12
    pushq   %r13                   /* rsp: 8 -> 0 (mod 16) */
    subq    $32, %rsp
    movq    %rcx, %rbx             /* rbx = ch */
    movq    %rdx, %r12             /* r12 = value */
.Lchan5_send_retry:
    movq    %rbx, %rcx
    call    core_lock_acquire
    cmpq    $0, 40(%rbx)           /* closed? */
    je      .Lchan5_send_open
    movq    %rbx, %rcx             /* closed -> 解锁后 abort */
    call    core_lock_release
    call    core_proc_abort        /* rsp≡0，shadow 已在栈上 */
    hlt
.Lchan5_send_open:
    movq    32(%rbx), %rax         /* count */
    cmpq    8(%rbx), %rax          /* count vs cap */
    jl      .Lchan5_send_room
    movq    %rbx, %rcx             /* 满：解锁 -> 让出 -> 重试 */
    call    core_lock_release
    call    SwitchToThread
    jmp     .Lchan5_send_retry
.Lchan5_send_room:
    movq    24(%rbx), %r13         /* r13 = tail */
    shlq    $3, %r13
    addq    $48, %r13
    addq    %rbx, %r13             /* &slots[tail]（callee-saved，不会跨调用被改） */
    movq    %r12, (%r13)           /* slots[tail] = value */
    movq    24(%rbx), %rax
    addq    $1, %rax
    cmpq    8(%rbx), %rax          /* tail+1 >= cap ? */
    jl      .Lchan5_send_tail_ok
    xorl    %eax, %eax             /* 回绕到 0 */
.Lchan5_send_tail_ok:
    movq    %rax, 24(%rbx)         /* tail 更新 */
    addq    $1, 32(%rbx)           /* count++ */
    movq    %rbx, %rcx
    call    core_lock_release
    addq    $32, %rsp
    popq    %r13
    popq    %r12
    popq    %rbx
    ret

/* ptr core_chan_recv(ptr ch) — rcx=ch，返回值或 null(0)
 * 空且开：解锁让出重试；空且关：返回 null。
 * %rbx=ch, %r12=取出的 value（callee-saved，跨 core_* 调用不丢）。 */
    .globl  core_chan_recv
core_chan_recv:
    pushq   %rbx
    pushq   %r12                   /* rsp: 8 -> 0 (mod 16) */
    subq    $40, %rsp              /* 40 ≡ 8 (mod 16)，保持调用前 rsp≡0 */
    movq    %rcx, %rbx
.Lchan5_recv_retry:
    movq    %rbx, %rcx
    call    core_lock_acquire
    cmpq    $0, 32(%rbx)           /* count > 0 ? */
    jg      .Lchan5_recv_item
    cmpq    $0, 40(%rbx)           /* 空：closed ? */
    je      .Lchan5_recv_wait
    movq    %rbx, %rcx             /* closed+空 -> 解锁返回 null */
    call    core_lock_release
    xorl    %eax, %eax
    jmp     .Lchan5_recv_done
.Lchan5_recv_wait:
    movq    %rbx, %rcx             /* 空：解锁 -> 让出 -> 重试 */
    call    core_lock_release
    call    SwitchToThread
    jmp     .Lchan5_recv_retry
.Lchan5_recv_item:
    movq    16(%rbx), %rax         /* head */
    shlq    $3, %rax
    addq    $48, %rax
    addq    %rbx, %rax             /* &slots[head] */
    movq    (%rax), %r12           /* value（callee-saved，不被 release 调用冲掉） */
    movq    16(%rbx), %rax
    addq    $1, %rax
    cmpq    8(%rbx), %rax          /* head+1 >= cap ? */
    jl      .Lchan5_recv_head_ok
    xorl    %eax, %eax
.Lchan5_recv_head_ok:
    movq    %rax, 16(%rbx)         /* head 更新 */
    subq    $1, 32(%rbx)           /* count-- */
    movq    %rbx, %rcx
    call    core_lock_release
    movq    %r12, %rax             /* 返回值 = value */
.Lchan5_recv_done:
    addq    $40, %rsp
    popq    %r12
    popq    %rbx
    ret

/* void core_chan_close(ptr ch) — rcx=ch，幂等 */
    .globl  core_chan_close
core_chan_close:
    pushq   %rbx                   /* rsp: 8 -> 0 */
    movq    %rcx, %rbx
    subq    $32, %rsp              /* rsp≡0 */
    movq    %rbx, %rcx
    call    core_lock_acquire
    movq    $1, 40(%rbx)           /* closed = 1 */
    movq    %rbx, %rcx
    call    core_lock_release
    addq    $32, %rsp
    popq    %rbx
    ret

/* void core_chan_drop(ptr ch) — rcx=ch，释放通道对象（不递归释放载荷） */
    .globl  core_chan_drop
core_chan_drop:
    subq    $40, %rsp              /* rsp: 8 -> 0，shadow for core_free */
    call    core_free
    addq    $40, %rsp
    ret

/* ============================================================
 * void core_sys_breakpoint(void)
 * ============================================================ */
    .globl  core_sys_breakpoint
core_sys_breakpoint:
    int3
    ret


