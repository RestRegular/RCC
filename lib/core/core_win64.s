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
    subq    $40, %rsp            /* 32 shadow + 8 对齐 */
    movq    %rsi, 32(%rsp)       /* P13-1 修正（ABI 合规）：%rsi 是 callee-saved，
                                  * 下面清 ring 的循环拿它当临时寄存器，必须还回去 */
    movq    %rcx, %r8            /* r8 = size（HeapAlloc 第 3 参） */
    movq    %rdx, %r9            /* 保存 align（本实现忽略，堆已 16 字节对齐） */
    call    GetProcessHeap
    testq   %rax, %rax
    je      .Lalloc_fail
    movq    %rax, %rcx           /* rcx = hHeap */
    xorl    %edx, %edx           /* rdx = flags = 0 */
    movq    %r8, %r8             /* r8 = dwBytes */
    call    HeapAlloc
    /* P13-1 调试钩子（D17）：成功分配才计数。发布构建里这个函数永远不被
     * 调用（codegen 只在 -debug-alloc 时引用），计数恒为 0 也无害。 */
    testq   %rax, %rax
    je      .Lalloc_nobump
    addq    $1, core_dbg_alloc_count(%rip)
    /* 这个地址已经重新分配 -> 从「已释放」ring 里清掉，否则下一轮正常
     * 释放会被误判成重复释放（见 core_free 里的说明）。 */
    pushq   %rax
    movq    %rax, %r8
    leaq    core_dbg_recent(%rip), %rdx
    movl    $16, %ecx
.Lalloc_clear_ring:
    movq    (%rdx), %rsi
    cmpq    %r8, %rsi
    jne     .Lalloc_clear_next
    movq    $0, (%rdx)
.Lalloc_clear_next:
    addq    $8, %rdx
    decl    %ecx
    jne     .Lalloc_clear_ring
    popq    %rax
.Lalloc_nobump:
    movq    32(%rsp), %rsi
    addq    $40, %rsp
    ret
.Lalloc_fail:
    xorl    %eax, %eax
    movq    32(%rsp), %rsi
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
    je      .Lfree_ret
    /* cap == 0 -> 静态只读头，不可释放 */
    cmpq    $0, 8(%rcx)
    je      .Lfree_ret
    /* P13-1 修正（ABI 合规）：下面用 %rsi 当扫描临时寄存器，
     * 而 %rsi/%rdi 在 MS x64 上是 callee-saved —— 必须保存恢复，
     * 否则调用者放在里面的数据会被悄悄改掉（core_list_append 就因此
     * 丢过 value）。 */
    pushq   %rsi
    pushq   %rdi
    subq    $56, %rsp
    movq    %rcx, 32(%rsp)       /* 暂存 p */

    /* ---- P13-1 调试钩子（D17）：重复释放检测 ----
     * 在**最近释放过的指针**环形缓冲里找 p。命中说明同一块被释放了两次，
     * 那是必然的堆损坏（HeapFree 对已释放块是未定义行为，实测进程直接
     * 以 0xC0000374 退出、没有任何可读线索）。
     *
     * 命中后**不再真的调用 HeapFree**，只计数然后返回：把「崩溃」变成
     * 「可观测的数字」。这样负向用例能给出确定结论，而不是在堆里撞运气。
     *
     * **关键前提：core_alloc 成功时必须把这个指针从 ring 里清掉。**
     * 否则「一块被释放 -> 地址被下一轮复用 -> 再被正常释放」会被误判成
     * 重复释放（实测 3 轮循环各释放一次，却报出 1 次重复）。清掉之后，
     * ring 里只保留「释放过且尚未被重新分配」的地址，语义才是对的。 */
    movq    32(%rsp), %rax
    leaq    core_dbg_recent(%rip), %rdx
    movl    $0, %ecx
.Lfree_dup_scan:
    cmpl    $16, %ecx
    jae     .Lfree_dup_scan_done
    movq    (%rdx,%rcx,8), %rsi
    cmpq    %rax, %rsi
    je      .Lfree_dup_hit
    incl    %ecx
    jmp     .Lfree_dup_scan
.Lfree_dup_scan_done:
    jmp     .Lfree_do_it
.Lfree_dup_hit:
    /* 命中重复释放：计数，不真的释放，直接结束 */
    addq    $1, core_dbg_double_free_count(%rip)
    addq    $56, %rsp
    popq    %rdi
    popq    %rsi
    ret

.Lfree_do_it:
    call    GetProcessHeap
    movq    %rax, 40(%rsp)       /* hHeap */

    /* ---- D17：释放前把整块填 0xDD ----
     * 需要真实的块大小，而我们的容器头只记 len/cap（那是逻辑长度，不是
     * 堆块大小）。用 HeapSize 问 Windows 自己，避免猜。
     * 填在 HeapFree **之前**：之后那块就不属于我们了。 */
    movq    40(%rsp), %rcx
    xorl    %edx, %edx
    movq    32(%rsp), %r8
    call    HeapSize
    cmpq    $-1, %rax            /* HeapSize 失败返回 (SIZE_T)-1 */
    je      .Lfree_after_poison
    movq    %rax, 48(%rsp)       /* size */
    testq   %rax, %rax
    je      .Lfree_after_poison
    movq    32(%rsp), %rcx       /* dst = p */
    movl    $0xDD, %edx          /* byte = 0xDD */
    movq    48(%rsp), %r8        /* n = size */
    call    core_mem_set
    movq    48(%rsp), %rax
    addq    %rax, core_dbg_poison_bytes_total(%rip)

.Lfree_after_poison:
    movq    40(%rsp), %rcx
    xorl    %edx, %edx
    movq    32(%rsp), %r8
    call    HeapFree
    /* P13-1 调试钩子（D17）：只有真正落到 HeapFree 才计数 ——
     * null / cap==0 的静默返回不算「释放」，否则计数器证明不了任何事。 */
    addq    $1, core_dbg_free_count(%rip)
    /* 记进 recently-freed ring（FIFO，覆盖最旧的一项） */
    leaq    core_dbg_recent(%rip), %rdx
    movq    core_dbg_recent_idx(%rip), %rcx
    movq    32(%rsp), %rax
    movq    %rax, (%rdx,%rcx,8)
    incq    %rcx
    andq    $15, %rcx
    movq    %rcx, core_dbg_recent_idx(%rip)
    addq    $56, %rsp
    popq    %rdi
    popq    %rsi
.Lfree_done:
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
/* 最近释放过的 16 个指针（环形，FIFO 覆盖）。用来抓「同一块释放两次」。 */
core_dbg_recent:
    .zero   128
core_dbg_recent_idx:
    .zero   8

    .text
/* void core_dbg_reset(void) */
    .globl  core_dbg_reset
core_dbg_reset:
    xorq    %rax, %rax
    movq    %rax, core_dbg_alloc_count(%rip)
    movq    %rax, core_dbg_free_count(%rip)
    movq    %rax, core_dbg_live_bytes(%rip)
    movq    %rax, core_dbg_double_free_count(%rip)
    movq    %rax, core_dbg_poison_bytes_total(%rip)
    movq    %rax, core_dbg_recent_idx(%rip)
    leaq    core_dbg_recent(%rip), %rdx
    movl    $16, %ecx
.Ldbg_reset_ring:
    movq    %rax, (%rdx)
    addq    $8, %rdx
    decl    %ecx
    jne     .Ldbg_reset_ring
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
    addq    %rcx, core_dbg_alloc_count(%rip)
    ret

/* void core_dbg_bump_free(int n) —— 由 core_free 调用 */
    .globl  core_dbg_bump_free
core_dbg_bump_free:
    addq    %rcx, core_dbg_free_count(%rip)
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
