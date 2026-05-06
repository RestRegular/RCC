; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }
%RCCDict = type { i64, i64, ptr, ptr }
%RCCList = type { i64, i64, ptr }
%RCCDict.0 = type { i64, i64, ptr, ptr }
%RCCList.1 = type { i64, i64, ptr }
%RCCDict.2 = type { i64, i64, ptr, ptr }
%RCCDict.3 = type { i64, i64, ptr, ptr }
%RCCList.4 = type { i64, i64, ptr }
%RCCDict.5 = type { i64, i64, ptr, ptr }
%RCCList.6 = type { i64, i64, ptr }
%RCCDict.7 = type { i64, i64, ptr, ptr }
%RCCList.8 = type { i64, i64, ptr }
%RCCDict.9 = type { i64, i64, ptr, ptr }

@.str = private constant [5 x i8] c"name\00"
@.str.1 = private constant [4 x i8] c"Rio\00"
@.str.2 = private constant [4 x i8] c"age\00"
@.str.3 = private constant [5 x i8] c"name\00"
@.str.4 = private constant [5 x i8] c"%lld\00"
@.str.5 = private constant [3 x i8] c"%g\00"
@.str.6 = private constant [3 x i8] c"%s\00"
@.str.7 = private constant [5 x i8] c"true\00"
@.str.8 = private constant [3 x i8] c"%s\00"
@.str.9 = private constant [6 x i8] c"false\00"
@.str.10 = private constant [3 x i8] c"%s\00"
@.str.11 = private constant [3 x i8] c"%s\00"
@.str.12 = private constant [5 x i8] c"null\00"
@.str.13 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.14 = private constant [4 x i8] c"age\00"
@.str.15 = private constant [5 x i8] c"%lld\00"
@.str.16 = private constant [3 x i8] c"%g\00"
@.str.17 = private constant [3 x i8] c"%s\00"
@.str.18 = private constant [5 x i8] c"true\00"
@.str.19 = private constant [3 x i8] c"%s\00"
@.str.20 = private constant [6 x i8] c"false\00"
@.str.21 = private constant [3 x i8] c"%s\00"
@.str.22 = private constant [3 x i8] c"%s\00"
@.str.23 = private constant [5 x i8] c"null\00"
@.str.24 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.25 = private constant [2 x i8] c"a\00"
@.str.26 = private constant [2 x i8] c"b\00"
@.str.27 = private constant [2 x i8] c"c\00"
@.str.28 = private constant [2 x i8] c"a\00"
@.str.29 = private constant [5 x i8] c"%lld\00"
@.str.30 = private constant [3 x i8] c"%g\00"
@.str.31 = private constant [3 x i8] c"%s\00"
@.str.32 = private constant [5 x i8] c"true\00"
@.str.33 = private constant [3 x i8] c"%s\00"
@.str.34 = private constant [6 x i8] c"false\00"
@.str.35 = private constant [3 x i8] c"%s\00"
@.str.36 = private constant [3 x i8] c"%s\00"
@.str.37 = private constant [5 x i8] c"null\00"
@.str.38 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.39 = private constant [2 x i8] c"b\00"
@.str.40 = private constant [5 x i8] c"%lld\00"
@.str.41 = private constant [3 x i8] c"%g\00"
@.str.42 = private constant [3 x i8] c"%s\00"
@.str.43 = private constant [5 x i8] c"true\00"
@.str.44 = private constant [3 x i8] c"%s\00"
@.str.45 = private constant [6 x i8] c"false\00"
@.str.46 = private constant [3 x i8] c"%s\00"
@.str.47 = private constant [3 x i8] c"%s\00"
@.str.48 = private constant [5 x i8] c"null\00"
@.str.49 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.50 = private constant [2 x i8] c"c\00"
@.str.51 = private constant [5 x i8] c"%lld\00"
@.str.52 = private constant [3 x i8] c"%g\00"
@.str.53 = private constant [3 x i8] c"%s\00"
@.str.54 = private constant [5 x i8] c"true\00"
@.str.55 = private constant [3 x i8] c"%s\00"
@.str.56 = private constant [6 x i8] c"false\00"
@.str.57 = private constant [3 x i8] c"%s\00"
@.str.58 = private constant [3 x i8] c"%s\00"
@.str.59 = private constant [5 x i8] c"null\00"
@.str.60 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %dic2 = alloca ptr, align 8
  %dic = alloca ptr, align 8
  store ptr null, ptr %dic, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 4, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr, align 8
  %rcc.val.heap1 = call ptr @malloc(i64 16)
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 0
  store i64 4, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 1
  store ptr @.str.1, ptr %payload.ptr3, align 8
  %rcc.val.heap4 = call ptr @malloc(i64 16)
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 0
  store i64 4, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 1
  store ptr @.str.2, ptr %payload.ptr6, align 8
  %rcc.val.heap7 = call ptr @malloc(i64 16)
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap7, i32 0, i32 0
  store i64 1, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap7, i32 0, i32 1
  store ptr inttoptr (i64 18 to ptr), ptr %payload.ptr9, align 8
  %dict.keys = call ptr @malloc(i64 16)
  %dict.values = call ptr @malloc(i64 16)
  %key.gep = getelementptr inbounds i8, ptr %dict.keys, i64 0
  store ptr %rcc.val.heap, ptr %key.gep, align 8
  %key.gep10 = getelementptr inbounds i8, ptr %dict.keys, i64 8
  store ptr %rcc.val.heap4, ptr %key.gep10, align 8
  %val.gep = getelementptr inbounds i8, ptr %dict.values, i64 0
  store ptr %rcc.val.heap1, ptr %val.gep, align 8
  %val.gep11 = getelementptr inbounds i8, ptr %dict.values, i64 8
  store ptr %rcc.val.heap7, ptr %val.gep11, align 8
  %dict.struct = call ptr @malloc(i64 32)
  %2 = getelementptr inbounds nuw %RCCDict, ptr %dict.struct, i32 0, i32 0
  store i64 2, ptr %2, align 4
  %3 = getelementptr inbounds nuw %RCCDict, ptr %dict.struct, i32 0, i32 1
  store i64 2, ptr %3, align 4
  %4 = getelementptr inbounds nuw %RCCDict, ptr %dict.struct, i32 0, i32 2
  store ptr %dict.keys, ptr %4, align 8
  %5 = getelementptr inbounds nuw %RCCDict, ptr %dict.struct, i32 0, i32 3
  store ptr %dict.values, ptr %5, align 8
  %rcc.val.heap12 = call ptr @malloc(i64 16)
  %tag.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap12, i32 0, i32 0
  store i64 6, ptr %tag.ptr13, align 4
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap12, i32 0, i32 1
  store ptr %dict.struct, ptr %payload.ptr14, align 8
  store ptr %rcc.val.heap12, ptr %dic, align 8
  %dic.load = load ptr, ptr %dic, align 8
  %rcc.val.heap15 = call ptr @malloc(i64 16)
  %tag.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 0
  store i64 4, ptr %tag.ptr16, align 4
  %payload.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 1
  store ptr @.str.3, ptr %payload.ptr17, align 8
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr18, align 8
  %index.int = ptrtoint ptr %payload to i64
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %dic.load, i32 0, i32 1
  %payload20 = load ptr, ptr %payload.ptr19, align 8
  %tag.ptr21 = getelementptr inbounds nuw %RCCValue, ptr %dic.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr21, align 4
  %idx.result = alloca ptr, align 8
  %rcc.val.heap22 = call ptr @malloc(i64 16)
  %tag.ptr23 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap22, i32 0, i32 0
  store i64 0, ptr %tag.ptr23, align 4
  %payload.ptr24 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap22, i32 0, i32 1
  store ptr null, ptr %payload.ptr24, align 8
  store ptr %rcc.val.heap22, ptr %idx.result, align 8
  switch i64 %tag, label %idx.default [
    i64 5, label %idx.list
    i64 6, label %idx.dict
  ]

idx.list:                                         ; preds = %entry
  %list.data = getelementptr inbounds nuw %RCCList, ptr %payload20, i32 0, i32 2
  %data = load ptr, ptr %list.data, align 8
  %idx.byte.offset = mul i64 %index.int, 8
  %elem.ptr = getelementptr inbounds i8, ptr %data, i64 %idx.byte.offset
  %elem = load ptr, ptr %elem.ptr, align 8
  store ptr %elem, ptr %idx.result, align 8
  br label %idx.merge

idx.dict:                                         ; preds = %entry
  %dict.size = getelementptr inbounds nuw %RCCDict.0, ptr %payload20, i32 0, i32 1
  %size = load i64, ptr %dict.size, align 4
  %dict.keys25 = getelementptr inbounds nuw %RCCDict.0, ptr %payload20, i32 0, i32 2
  %keys = load ptr, ptr %dict.keys25, align 8
  %dict.values26 = getelementptr inbounds nuw %RCCDict.0, ptr %payload20, i32 0, i32 3
  %vals = load ptr, ptr %dict.values26, align 8
  %dict.i = alloca i64, align 8
  store i64 0, ptr %dict.i, align 4
  br label %dict.search.header

idx.default:                                      ; preds = %entry
  br label %idx.merge

idx.merge:                                        ; preds = %idx.default, %dict.search.merge, %idx.list
  %idx.result35 = load ptr, ptr %idx.result, align 8
  %tag.ptr36 = getelementptr inbounds nuw %RCCValue, ptr %idx.result35, i32 0, i32 0
  %tag37 = load i64, ptr %tag.ptr36, align 4
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %idx.result35, i32 0, i32 1
  %payload39 = load ptr, ptr %payload.ptr38, align 8
  switch i64 %tag37, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

dict.search.header:                               ; preds = %next.iter, %idx.dict
  %i = load i64, ptr %dict.i, align 4
  %in.bounds = icmp slt i64 %i, %size
  br i1 %in.bounds, label %dict.search.body, label %dict.notfound

dict.search.body:                                 ; preds = %dict.search.header
  %i.body = load i64, ptr %dict.i, align 4
  %key.byte.offset = mul i64 %i.body, 8
  %key.gep27 = getelementptr inbounds i8, ptr %keys, i64 %key.byte.offset
  %key.val = load ptr, ptr %key.gep27, align 8
  %tag.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %key.val, i32 0, i32 0
  %tag29 = load i64, ptr %tag.ptr28, align 4
  %tag.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 0
  %tag31 = load i64, ptr %tag.ptr30, align 4
  %tag.eq = icmp eq i64 %tag29, %tag31
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %key.val, i32 0, i32 1
  %payload33 = load ptr, ptr %payload.ptr32, align 8
  %is.string = icmp eq i64 %tag29, 4
  br i1 %is.string, label %cmp.str, label %cmp.ptr

dict.found:                                       ; preds = %cmp.merge
  %i.found = load i64, ptr %dict.i, align 4
  %val.byte.offset = mul i64 %i.found, 8
  %val.gep34 = getelementptr inbounds i8, ptr %vals, i64 %val.byte.offset
  %val = load ptr, ptr %val.gep34, align 8
  store ptr %val, ptr %idx.result, align 8
  br label %dict.search.merge

dict.notfound:                                    ; preds = %dict.search.header
  br label %dict.search.merge

dict.search.merge:                                ; preds = %dict.notfound, %dict.found
  br label %idx.merge

cmp.str:                                          ; preds = %dict.search.body
  %strcmp.result = call i32 @strcmp(ptr %payload33, ptr %payload)
  %strcmp.eq = icmp eq i32 %strcmp.result, 0
  br label %cmp.merge

cmp.ptr:                                          ; preds = %dict.search.body
  %ptr.eq = icmp eq ptr %payload33, %payload
  br label %cmp.merge

cmp.merge:                                        ; preds = %cmp.ptr, %cmp.str
  %payload.eq = phi i1 [ %strcmp.eq, %cmp.str ], [ %ptr.eq, %cmp.ptr ]
  %key.eq = and i1 %tag.eq, %payload.eq
  store i64 %i.body, ptr %dict.i, align 4
  br i1 %key.eq, label %dict.found, label %next.iter

next.iter:                                        ; preds = %cmp.merge
  %next.i = add i64 %i.body, 1
  store i64 %next.i, ptr %dict.i, align 4
  br label %dict.search.header

sout.int:                                         ; preds = %idx.merge
  %int.val = ptrtoint ptr %payload39 to i64
  %6 = call i32 (ptr, ...) @printf(ptr @.str.4, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %idx.merge
  %float.bits = ptrtoint ptr %payload39 to i64
  %float.val = bitcast i64 %float.bits to double
  %7 = call i32 (ptr, ...) @printf(ptr @.str.5, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %idx.merge
  %bool.val = ptrtoint ptr %payload39 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %idx.merge
  %8 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload39)
  br label %sout.merge

sout.null:                                        ; preds = %idx.merge
  %9 = call i32 (ptr, ...) @printf(ptr @.str.11, ptr @.str.12)
  br label %sout.merge

sout.default:                                     ; preds = %idx.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.13, ptr %payload39)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %11 = call i32 @putchar(i32 10)
  %rcc.val.heap40 = call ptr @malloc(i64 16)
  %tag.ptr41 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap40, i32 0, i32 0
  store i64 0, ptr %tag.ptr41, align 4
  %payload.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap40, i32 0, i32 1
  store ptr null, ptr %payload.ptr42, align 8
  %dic.load43 = load ptr, ptr %dic, align 8
  %rcc.val.heap44 = call ptr @malloc(i64 16)
  %tag.ptr45 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap44, i32 0, i32 0
  store i64 4, ptr %tag.ptr45, align 4
  %payload.ptr46 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap44, i32 0, i32 1
  store ptr @.str.14, ptr %payload.ptr46, align 8
  %payload.ptr47 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap44, i32 0, i32 1
  %payload48 = load ptr, ptr %payload.ptr47, align 8
  %index.int49 = ptrtoint ptr %payload48 to i64
  %payload.ptr50 = getelementptr inbounds nuw %RCCValue, ptr %dic.load43, i32 0, i32 1
  %payload51 = load ptr, ptr %payload.ptr50, align 8
  %tag.ptr52 = getelementptr inbounds nuw %RCCValue, ptr %dic.load43, i32 0, i32 0
  %tag53 = load i64, ptr %tag.ptr52, align 4
  %idx.result54 = alloca ptr, align 8
  %rcc.val.heap55 = call ptr @malloc(i64 16)
  %tag.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap55, i32 0, i32 0
  store i64 0, ptr %tag.ptr56, align 4
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap55, i32 0, i32 1
  store ptr null, ptr %payload.ptr57, align 8
  store ptr %rcc.val.heap55, ptr %idx.result54, align 8
  switch i64 %tag53, label %idx.default60 [
    i64 5, label %idx.list58
    i64 6, label %idx.dict59
  ]

sout.true:                                        ; preds = %sout.bool
  %12 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr @.str.7)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %13 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr @.str.9)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

idx.list58:                                       ; preds = %sout.merge
  %list.data62 = getelementptr inbounds nuw %RCCList.1, ptr %payload51, i32 0, i32 2
  %data63 = load ptr, ptr %list.data62, align 8
  %idx.byte.offset64 = mul i64 %index.int49, 8
  %elem.ptr65 = getelementptr inbounds i8, ptr %data63, i64 %idx.byte.offset64
  %elem66 = load ptr, ptr %elem.ptr65, align 8
  store ptr %elem66, ptr %idx.result54, align 8
  br label %idx.merge61

idx.dict59:                                       ; preds = %sout.merge
  %dict.size67 = getelementptr inbounds nuw %RCCDict.2, ptr %payload51, i32 0, i32 1
  %size68 = load i64, ptr %dict.size67, align 4
  %dict.keys69 = getelementptr inbounds nuw %RCCDict.2, ptr %payload51, i32 0, i32 2
  %keys70 = load ptr, ptr %dict.keys69, align 8
  %dict.values71 = getelementptr inbounds nuw %RCCDict.2, ptr %payload51, i32 0, i32 3
  %vals72 = load ptr, ptr %dict.values71, align 8
  %dict.i78 = alloca i64, align 8
  store i64 0, ptr %dict.i78, align 4
  br label %dict.search.header73

idx.default60:                                    ; preds = %sout.merge
  br label %idx.merge61

idx.merge61:                                      ; preds = %idx.default60, %dict.search.merge77, %idx.list58
  %idx.result107 = load ptr, ptr %idx.result54, align 8
  %tag.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %idx.result107, i32 0, i32 0
  %tag109 = load i64, ptr %tag.ptr108, align 4
  %payload.ptr110 = getelementptr inbounds nuw %RCCValue, ptr %idx.result107, i32 0, i32 1
  %payload111 = load ptr, ptr %payload.ptr110, align 8
  switch i64 %tag109, label %sout.default117 [
    i64 1, label %sout.int112
    i64 2, label %sout.float113
    i64 3, label %sout.bool114
    i64 4, label %sout.str115
    i64 0, label %sout.null116
  ]

dict.search.header73:                             ; preds = %next.iter101, %idx.dict59
  %i79 = load i64, ptr %dict.i78, align 4
  %in.bounds80 = icmp slt i64 %i79, %size68
  br i1 %in.bounds80, label %dict.search.body74, label %dict.notfound76

dict.search.body74:                               ; preds = %dict.search.header73
  %i.body81 = load i64, ptr %dict.i78, align 4
  %key.byte.offset82 = mul i64 %i.body81, 8
  %key.gep83 = getelementptr inbounds i8, ptr %keys70, i64 %key.byte.offset82
  %key.val84 = load ptr, ptr %key.gep83, align 8
  %tag.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %key.val84, i32 0, i32 0
  %tag86 = load i64, ptr %tag.ptr85, align 4
  %tag.ptr87 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap44, i32 0, i32 0
  %tag88 = load i64, ptr %tag.ptr87, align 4
  %tag.eq89 = icmp eq i64 %tag86, %tag88
  %payload.ptr90 = getelementptr inbounds nuw %RCCValue, ptr %key.val84, i32 0, i32 1
  %payload91 = load ptr, ptr %payload.ptr90, align 8
  %is.string92 = icmp eq i64 %tag86, 4
  br i1 %is.string92, label %cmp.str93, label %cmp.ptr94

dict.found75:                                     ; preds = %cmp.merge95
  %i.found103 = load i64, ptr %dict.i78, align 4
  %val.byte.offset104 = mul i64 %i.found103, 8
  %val.gep105 = getelementptr inbounds i8, ptr %vals72, i64 %val.byte.offset104
  %val106 = load ptr, ptr %val.gep105, align 8
  store ptr %val106, ptr %idx.result54, align 8
  br label %dict.search.merge77

dict.notfound76:                                  ; preds = %dict.search.header73
  br label %dict.search.merge77

dict.search.merge77:                              ; preds = %dict.notfound76, %dict.found75
  br label %idx.merge61

cmp.str93:                                        ; preds = %dict.search.body74
  %strcmp.result96 = call i32 @strcmp(ptr %payload91, ptr %payload48)
  %strcmp.eq97 = icmp eq i32 %strcmp.result96, 0
  br label %cmp.merge95

cmp.ptr94:                                        ; preds = %dict.search.body74
  %ptr.eq98 = icmp eq ptr %payload91, %payload48
  br label %cmp.merge95

cmp.merge95:                                      ; preds = %cmp.ptr94, %cmp.str93
  %payload.eq99 = phi i1 [ %strcmp.eq97, %cmp.str93 ], [ %ptr.eq98, %cmp.ptr94 ]
  %key.eq100 = and i1 %tag.eq89, %payload.eq99
  store i64 %i.body81, ptr %dict.i78, align 4
  br i1 %key.eq100, label %dict.found75, label %next.iter101

next.iter101:                                     ; preds = %cmp.merge95
  %next.i102 = add i64 %i.body81, 1
  store i64 %next.i102, ptr %dict.i78, align 4
  br label %dict.search.header73

sout.int112:                                      ; preds = %idx.merge61
  %int.val119 = ptrtoint ptr %payload111 to i64
  %14 = call i32 (ptr, ...) @printf(ptr @.str.15, i64 %int.val119)
  br label %sout.merge118

sout.float113:                                    ; preds = %idx.merge61
  %float.bits120 = ptrtoint ptr %payload111 to i64
  %float.val121 = bitcast i64 %float.bits120 to double
  %15 = call i32 (ptr, ...) @printf(ptr @.str.16, double %float.val121)
  br label %sout.merge118

sout.bool114:                                     ; preds = %idx.merge61
  %bool.val122 = ptrtoint ptr %payload111 to i64
  %is.true123 = icmp ne i64 %bool.val122, 0
  br i1 %is.true123, label %sout.true124, label %sout.false125

sout.str115:                                      ; preds = %idx.merge61
  %16 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload111)
  br label %sout.merge118

sout.null116:                                     ; preds = %idx.merge61
  %17 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.merge118

sout.default117:                                  ; preds = %idx.merge61
  %18 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr %payload111)
  br label %sout.merge118

sout.merge118:                                    ; preds = %sout.default117, %sout.null116, %sout.str115, %sout.bool.merge126, %sout.float113, %sout.int112
  %19 = call i32 @putchar(i32 10)
  %rcc.val.heap127 = call ptr @malloc(i64 16)
  %tag.ptr128 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap127, i32 0, i32 0
  store i64 0, ptr %tag.ptr128, align 4
  %payload.ptr129 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap127, i32 0, i32 1
  store ptr null, ptr %payload.ptr129, align 8
  store ptr null, ptr %dic2, align 8
  %rcc.val.heap130 = call ptr @malloc(i64 16)
  %tag.ptr131 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap130, i32 0, i32 0
  store i64 4, ptr %tag.ptr131, align 4
  %payload.ptr132 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap130, i32 0, i32 1
  store ptr @.str.25, ptr %payload.ptr132, align 8
  %rcc.val.heap133 = call ptr @malloc(i64 16)
  %tag.ptr134 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap133, i32 0, i32 0
  store i64 1, ptr %tag.ptr134, align 4
  %payload.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap133, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr135, align 8
  %rcc.val.heap136 = call ptr @malloc(i64 16)
  %tag.ptr137 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap136, i32 0, i32 0
  store i64 4, ptr %tag.ptr137, align 4
  %payload.ptr138 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap136, i32 0, i32 1
  store ptr @.str.26, ptr %payload.ptr138, align 8
  %rcc.val.heap139 = call ptr @malloc(i64 16)
  %tag.ptr140 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap139, i32 0, i32 0
  store i64 1, ptr %tag.ptr140, align 4
  %payload.ptr141 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap139, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr141, align 8
  %rcc.val.heap142 = call ptr @malloc(i64 16)
  %tag.ptr143 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap142, i32 0, i32 0
  store i64 4, ptr %tag.ptr143, align 4
  %payload.ptr144 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap142, i32 0, i32 1
  store ptr @.str.27, ptr %payload.ptr144, align 8
  %rcc.val.heap145 = call ptr @malloc(i64 16)
  %tag.ptr146 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap145, i32 0, i32 0
  store i64 1, ptr %tag.ptr146, align 4
  %payload.ptr147 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap145, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr147, align 8
  %dict.keys148 = call ptr @malloc(i64 24)
  %dict.values149 = call ptr @malloc(i64 24)
  %key.gep150 = getelementptr inbounds i8, ptr %dict.keys148, i64 0
  store ptr %rcc.val.heap130, ptr %key.gep150, align 8
  %key.gep151 = getelementptr inbounds i8, ptr %dict.keys148, i64 8
  store ptr %rcc.val.heap136, ptr %key.gep151, align 8
  %key.gep152 = getelementptr inbounds i8, ptr %dict.keys148, i64 16
  store ptr %rcc.val.heap142, ptr %key.gep152, align 8
  %val.gep153 = getelementptr inbounds i8, ptr %dict.values149, i64 0
  store ptr %rcc.val.heap133, ptr %val.gep153, align 8
  %val.gep154 = getelementptr inbounds i8, ptr %dict.values149, i64 8
  store ptr %rcc.val.heap139, ptr %val.gep154, align 8
  %val.gep155 = getelementptr inbounds i8, ptr %dict.values149, i64 16
  store ptr %rcc.val.heap145, ptr %val.gep155, align 8
  %dict.struct156 = call ptr @malloc(i64 32)
  %20 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct156, i32 0, i32 0
  store i64 3, ptr %20, align 4
  %21 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct156, i32 0, i32 1
  store i64 3, ptr %21, align 4
  %22 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct156, i32 0, i32 2
  store ptr %dict.keys148, ptr %22, align 8
  %23 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct156, i32 0, i32 3
  store ptr %dict.values149, ptr %23, align 8
  %rcc.val.heap157 = call ptr @malloc(i64 16)
  %tag.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap157, i32 0, i32 0
  store i64 6, ptr %tag.ptr158, align 4
  %payload.ptr159 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap157, i32 0, i32 1
  store ptr %dict.struct156, ptr %payload.ptr159, align 8
  store ptr %rcc.val.heap157, ptr %dic2, align 8
  %dic2.load = load ptr, ptr %dic2, align 8
  %rcc.val.heap160 = call ptr @malloc(i64 16)
  %tag.ptr161 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap160, i32 0, i32 0
  store i64 4, ptr %tag.ptr161, align 4
  %payload.ptr162 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap160, i32 0, i32 1
  store ptr @.str.28, ptr %payload.ptr162, align 8
  %payload.ptr163 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap160, i32 0, i32 1
  %payload164 = load ptr, ptr %payload.ptr163, align 8
  %index.int165 = ptrtoint ptr %payload164 to i64
  %payload.ptr166 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load, i32 0, i32 1
  %payload167 = load ptr, ptr %payload.ptr166, align 8
  %tag.ptr168 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load, i32 0, i32 0
  %tag169 = load i64, ptr %tag.ptr168, align 4
  %idx.result170 = alloca ptr, align 8
  %rcc.val.heap171 = call ptr @malloc(i64 16)
  %tag.ptr172 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap171, i32 0, i32 0
  store i64 0, ptr %tag.ptr172, align 4
  %payload.ptr173 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap171, i32 0, i32 1
  store ptr null, ptr %payload.ptr173, align 8
  store ptr %rcc.val.heap171, ptr %idx.result170, align 8
  switch i64 %tag169, label %idx.default176 [
    i64 5, label %idx.list174
    i64 6, label %idx.dict175
  ]

sout.true124:                                     ; preds = %sout.bool114
  %24 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.bool.merge126

sout.false125:                                    ; preds = %sout.bool114
  %25 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.bool.merge126

sout.bool.merge126:                               ; preds = %sout.false125, %sout.true124
  br label %sout.merge118

idx.list174:                                      ; preds = %sout.merge118
  %list.data178 = getelementptr inbounds nuw %RCCList.4, ptr %payload167, i32 0, i32 2
  %data179 = load ptr, ptr %list.data178, align 8
  %idx.byte.offset180 = mul i64 %index.int165, 8
  %elem.ptr181 = getelementptr inbounds i8, ptr %data179, i64 %idx.byte.offset180
  %elem182 = load ptr, ptr %elem.ptr181, align 8
  store ptr %elem182, ptr %idx.result170, align 8
  br label %idx.merge177

idx.dict175:                                      ; preds = %sout.merge118
  %dict.size183 = getelementptr inbounds nuw %RCCDict.5, ptr %payload167, i32 0, i32 1
  %size184 = load i64, ptr %dict.size183, align 4
  %dict.keys185 = getelementptr inbounds nuw %RCCDict.5, ptr %payload167, i32 0, i32 2
  %keys186 = load ptr, ptr %dict.keys185, align 8
  %dict.values187 = getelementptr inbounds nuw %RCCDict.5, ptr %payload167, i32 0, i32 3
  %vals188 = load ptr, ptr %dict.values187, align 8
  %dict.i194 = alloca i64, align 8
  store i64 0, ptr %dict.i194, align 4
  br label %dict.search.header189

idx.default176:                                   ; preds = %sout.merge118
  br label %idx.merge177

idx.merge177:                                     ; preds = %idx.default176, %dict.search.merge193, %idx.list174
  %idx.result223 = load ptr, ptr %idx.result170, align 8
  %tag.ptr224 = getelementptr inbounds nuw %RCCValue, ptr %idx.result223, i32 0, i32 0
  %tag225 = load i64, ptr %tag.ptr224, align 4
  %payload.ptr226 = getelementptr inbounds nuw %RCCValue, ptr %idx.result223, i32 0, i32 1
  %payload227 = load ptr, ptr %payload.ptr226, align 8
  switch i64 %tag225, label %sout.default233 [
    i64 1, label %sout.int228
    i64 2, label %sout.float229
    i64 3, label %sout.bool230
    i64 4, label %sout.str231
    i64 0, label %sout.null232
  ]

dict.search.header189:                            ; preds = %next.iter217, %idx.dict175
  %i195 = load i64, ptr %dict.i194, align 4
  %in.bounds196 = icmp slt i64 %i195, %size184
  br i1 %in.bounds196, label %dict.search.body190, label %dict.notfound192

dict.search.body190:                              ; preds = %dict.search.header189
  %i.body197 = load i64, ptr %dict.i194, align 4
  %key.byte.offset198 = mul i64 %i.body197, 8
  %key.gep199 = getelementptr inbounds i8, ptr %keys186, i64 %key.byte.offset198
  %key.val200 = load ptr, ptr %key.gep199, align 8
  %tag.ptr201 = getelementptr inbounds nuw %RCCValue, ptr %key.val200, i32 0, i32 0
  %tag202 = load i64, ptr %tag.ptr201, align 4
  %tag.ptr203 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap160, i32 0, i32 0
  %tag204 = load i64, ptr %tag.ptr203, align 4
  %tag.eq205 = icmp eq i64 %tag202, %tag204
  %payload.ptr206 = getelementptr inbounds nuw %RCCValue, ptr %key.val200, i32 0, i32 1
  %payload207 = load ptr, ptr %payload.ptr206, align 8
  %is.string208 = icmp eq i64 %tag202, 4
  br i1 %is.string208, label %cmp.str209, label %cmp.ptr210

dict.found191:                                    ; preds = %cmp.merge211
  %i.found219 = load i64, ptr %dict.i194, align 4
  %val.byte.offset220 = mul i64 %i.found219, 8
  %val.gep221 = getelementptr inbounds i8, ptr %vals188, i64 %val.byte.offset220
  %val222 = load ptr, ptr %val.gep221, align 8
  store ptr %val222, ptr %idx.result170, align 8
  br label %dict.search.merge193

dict.notfound192:                                 ; preds = %dict.search.header189
  br label %dict.search.merge193

dict.search.merge193:                             ; preds = %dict.notfound192, %dict.found191
  br label %idx.merge177

cmp.str209:                                       ; preds = %dict.search.body190
  %strcmp.result212 = call i32 @strcmp(ptr %payload207, ptr %payload164)
  %strcmp.eq213 = icmp eq i32 %strcmp.result212, 0
  br label %cmp.merge211

cmp.ptr210:                                       ; preds = %dict.search.body190
  %ptr.eq214 = icmp eq ptr %payload207, %payload164
  br label %cmp.merge211

cmp.merge211:                                     ; preds = %cmp.ptr210, %cmp.str209
  %payload.eq215 = phi i1 [ %strcmp.eq213, %cmp.str209 ], [ %ptr.eq214, %cmp.ptr210 ]
  %key.eq216 = and i1 %tag.eq205, %payload.eq215
  store i64 %i.body197, ptr %dict.i194, align 4
  br i1 %key.eq216, label %dict.found191, label %next.iter217

next.iter217:                                     ; preds = %cmp.merge211
  %next.i218 = add i64 %i.body197, 1
  store i64 %next.i218, ptr %dict.i194, align 4
  br label %dict.search.header189

sout.int228:                                      ; preds = %idx.merge177
  %int.val235 = ptrtoint ptr %payload227 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.29, i64 %int.val235)
  br label %sout.merge234

sout.float229:                                    ; preds = %idx.merge177
  %float.bits236 = ptrtoint ptr %payload227 to i64
  %float.val237 = bitcast i64 %float.bits236 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.30, double %float.val237)
  br label %sout.merge234

sout.bool230:                                     ; preds = %idx.merge177
  %bool.val238 = ptrtoint ptr %payload227 to i64
  %is.true239 = icmp ne i64 %bool.val238, 0
  br i1 %is.true239, label %sout.true240, label %sout.false241

sout.str231:                                      ; preds = %idx.merge177
  %28 = call i32 (ptr, ...) @printf(ptr @.str.35, ptr %payload227)
  br label %sout.merge234

sout.null232:                                     ; preds = %idx.merge177
  %29 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.merge234

sout.default233:                                  ; preds = %idx.merge177
  %30 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr %payload227)
  br label %sout.merge234

sout.merge234:                                    ; preds = %sout.default233, %sout.null232, %sout.str231, %sout.bool.merge242, %sout.float229, %sout.int228
  %31 = call i32 @putchar(i32 10)
  %rcc.val.heap243 = call ptr @malloc(i64 16)
  %tag.ptr244 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap243, i32 0, i32 0
  store i64 0, ptr %tag.ptr244, align 4
  %payload.ptr245 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap243, i32 0, i32 1
  store ptr null, ptr %payload.ptr245, align 8
  %dic2.load246 = load ptr, ptr %dic2, align 8
  %rcc.val.heap247 = call ptr @malloc(i64 16)
  %tag.ptr248 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap247, i32 0, i32 0
  store i64 4, ptr %tag.ptr248, align 4
  %payload.ptr249 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap247, i32 0, i32 1
  store ptr @.str.39, ptr %payload.ptr249, align 8
  %payload.ptr250 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap247, i32 0, i32 1
  %payload251 = load ptr, ptr %payload.ptr250, align 8
  %index.int252 = ptrtoint ptr %payload251 to i64
  %payload.ptr253 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load246, i32 0, i32 1
  %payload254 = load ptr, ptr %payload.ptr253, align 8
  %tag.ptr255 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load246, i32 0, i32 0
  %tag256 = load i64, ptr %tag.ptr255, align 4
  %idx.result257 = alloca ptr, align 8
  %rcc.val.heap258 = call ptr @malloc(i64 16)
  %tag.ptr259 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap258, i32 0, i32 0
  store i64 0, ptr %tag.ptr259, align 4
  %payload.ptr260 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap258, i32 0, i32 1
  store ptr null, ptr %payload.ptr260, align 8
  store ptr %rcc.val.heap258, ptr %idx.result257, align 8
  switch i64 %tag256, label %idx.default263 [
    i64 5, label %idx.list261
    i64 6, label %idx.dict262
  ]

sout.true240:                                     ; preds = %sout.bool230
  %32 = call i32 (ptr, ...) @printf(ptr @.str.31, ptr @.str.32)
  br label %sout.bool.merge242

sout.false241:                                    ; preds = %sout.bool230
  %33 = call i32 (ptr, ...) @printf(ptr @.str.33, ptr @.str.34)
  br label %sout.bool.merge242

sout.bool.merge242:                               ; preds = %sout.false241, %sout.true240
  br label %sout.merge234

idx.list261:                                      ; preds = %sout.merge234
  %list.data265 = getelementptr inbounds nuw %RCCList.6, ptr %payload254, i32 0, i32 2
  %data266 = load ptr, ptr %list.data265, align 8
  %idx.byte.offset267 = mul i64 %index.int252, 8
  %elem.ptr268 = getelementptr inbounds i8, ptr %data266, i64 %idx.byte.offset267
  %elem269 = load ptr, ptr %elem.ptr268, align 8
  store ptr %elem269, ptr %idx.result257, align 8
  br label %idx.merge264

idx.dict262:                                      ; preds = %sout.merge234
  %dict.size270 = getelementptr inbounds nuw %RCCDict.7, ptr %payload254, i32 0, i32 1
  %size271 = load i64, ptr %dict.size270, align 4
  %dict.keys272 = getelementptr inbounds nuw %RCCDict.7, ptr %payload254, i32 0, i32 2
  %keys273 = load ptr, ptr %dict.keys272, align 8
  %dict.values274 = getelementptr inbounds nuw %RCCDict.7, ptr %payload254, i32 0, i32 3
  %vals275 = load ptr, ptr %dict.values274, align 8
  %dict.i281 = alloca i64, align 8
  store i64 0, ptr %dict.i281, align 4
  br label %dict.search.header276

idx.default263:                                   ; preds = %sout.merge234
  br label %idx.merge264

idx.merge264:                                     ; preds = %idx.default263, %dict.search.merge280, %idx.list261
  %idx.result310 = load ptr, ptr %idx.result257, align 8
  %tag.ptr311 = getelementptr inbounds nuw %RCCValue, ptr %idx.result310, i32 0, i32 0
  %tag312 = load i64, ptr %tag.ptr311, align 4
  %payload.ptr313 = getelementptr inbounds nuw %RCCValue, ptr %idx.result310, i32 0, i32 1
  %payload314 = load ptr, ptr %payload.ptr313, align 8
  switch i64 %tag312, label %sout.default320 [
    i64 1, label %sout.int315
    i64 2, label %sout.float316
    i64 3, label %sout.bool317
    i64 4, label %sout.str318
    i64 0, label %sout.null319
  ]

dict.search.header276:                            ; preds = %next.iter304, %idx.dict262
  %i282 = load i64, ptr %dict.i281, align 4
  %in.bounds283 = icmp slt i64 %i282, %size271
  br i1 %in.bounds283, label %dict.search.body277, label %dict.notfound279

dict.search.body277:                              ; preds = %dict.search.header276
  %i.body284 = load i64, ptr %dict.i281, align 4
  %key.byte.offset285 = mul i64 %i.body284, 8
  %key.gep286 = getelementptr inbounds i8, ptr %keys273, i64 %key.byte.offset285
  %key.val287 = load ptr, ptr %key.gep286, align 8
  %tag.ptr288 = getelementptr inbounds nuw %RCCValue, ptr %key.val287, i32 0, i32 0
  %tag289 = load i64, ptr %tag.ptr288, align 4
  %tag.ptr290 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap247, i32 0, i32 0
  %tag291 = load i64, ptr %tag.ptr290, align 4
  %tag.eq292 = icmp eq i64 %tag289, %tag291
  %payload.ptr293 = getelementptr inbounds nuw %RCCValue, ptr %key.val287, i32 0, i32 1
  %payload294 = load ptr, ptr %payload.ptr293, align 8
  %is.string295 = icmp eq i64 %tag289, 4
  br i1 %is.string295, label %cmp.str296, label %cmp.ptr297

dict.found278:                                    ; preds = %cmp.merge298
  %i.found306 = load i64, ptr %dict.i281, align 4
  %val.byte.offset307 = mul i64 %i.found306, 8
  %val.gep308 = getelementptr inbounds i8, ptr %vals275, i64 %val.byte.offset307
  %val309 = load ptr, ptr %val.gep308, align 8
  store ptr %val309, ptr %idx.result257, align 8
  br label %dict.search.merge280

dict.notfound279:                                 ; preds = %dict.search.header276
  br label %dict.search.merge280

dict.search.merge280:                             ; preds = %dict.notfound279, %dict.found278
  br label %idx.merge264

cmp.str296:                                       ; preds = %dict.search.body277
  %strcmp.result299 = call i32 @strcmp(ptr %payload294, ptr %payload251)
  %strcmp.eq300 = icmp eq i32 %strcmp.result299, 0
  br label %cmp.merge298

cmp.ptr297:                                       ; preds = %dict.search.body277
  %ptr.eq301 = icmp eq ptr %payload294, %payload251
  br label %cmp.merge298

cmp.merge298:                                     ; preds = %cmp.ptr297, %cmp.str296
  %payload.eq302 = phi i1 [ %strcmp.eq300, %cmp.str296 ], [ %ptr.eq301, %cmp.ptr297 ]
  %key.eq303 = and i1 %tag.eq292, %payload.eq302
  store i64 %i.body284, ptr %dict.i281, align 4
  br i1 %key.eq303, label %dict.found278, label %next.iter304

next.iter304:                                     ; preds = %cmp.merge298
  %next.i305 = add i64 %i.body284, 1
  store i64 %next.i305, ptr %dict.i281, align 4
  br label %dict.search.header276

sout.int315:                                      ; preds = %idx.merge264
  %int.val322 = ptrtoint ptr %payload314 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, i64 %int.val322)
  br label %sout.merge321

sout.float316:                                    ; preds = %idx.merge264
  %float.bits323 = ptrtoint ptr %payload314 to i64
  %float.val324 = bitcast i64 %float.bits323 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val324)
  br label %sout.merge321

sout.bool317:                                     ; preds = %idx.merge264
  %bool.val325 = ptrtoint ptr %payload314 to i64
  %is.true326 = icmp ne i64 %bool.val325, 0
  br i1 %is.true326, label %sout.true327, label %sout.false328

sout.str318:                                      ; preds = %idx.merge264
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload314)
  br label %sout.merge321

sout.null319:                                     ; preds = %idx.merge264
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge321

sout.default320:                                  ; preds = %idx.merge264
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload314)
  br label %sout.merge321

sout.merge321:                                    ; preds = %sout.default320, %sout.null319, %sout.str318, %sout.bool.merge329, %sout.float316, %sout.int315
  %39 = call i32 @putchar(i32 10)
  %rcc.val.heap330 = call ptr @malloc(i64 16)
  %tag.ptr331 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap330, i32 0, i32 0
  store i64 0, ptr %tag.ptr331, align 4
  %payload.ptr332 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap330, i32 0, i32 1
  store ptr null, ptr %payload.ptr332, align 8
  %dic2.load333 = load ptr, ptr %dic2, align 8
  %rcc.val.heap334 = call ptr @malloc(i64 16)
  %tag.ptr335 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap334, i32 0, i32 0
  store i64 4, ptr %tag.ptr335, align 4
  %payload.ptr336 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap334, i32 0, i32 1
  store ptr @.str.50, ptr %payload.ptr336, align 8
  %payload.ptr337 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap334, i32 0, i32 1
  %payload338 = load ptr, ptr %payload.ptr337, align 8
  %index.int339 = ptrtoint ptr %payload338 to i64
  %payload.ptr340 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load333, i32 0, i32 1
  %payload341 = load ptr, ptr %payload.ptr340, align 8
  %tag.ptr342 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load333, i32 0, i32 0
  %tag343 = load i64, ptr %tag.ptr342, align 4
  %idx.result344 = alloca ptr, align 8
  %rcc.val.heap345 = call ptr @malloc(i64 16)
  %tag.ptr346 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap345, i32 0, i32 0
  store i64 0, ptr %tag.ptr346, align 4
  %payload.ptr347 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap345, i32 0, i32 1
  store ptr null, ptr %payload.ptr347, align 8
  store ptr %rcc.val.heap345, ptr %idx.result344, align 8
  switch i64 %tag343, label %idx.default350 [
    i64 5, label %idx.list348
    i64 6, label %idx.dict349
  ]

sout.true327:                                     ; preds = %sout.bool317
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge329

sout.false328:                                    ; preds = %sout.bool317
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge329

sout.bool.merge329:                               ; preds = %sout.false328, %sout.true327
  br label %sout.merge321

idx.list348:                                      ; preds = %sout.merge321
  %list.data352 = getelementptr inbounds nuw %RCCList.8, ptr %payload341, i32 0, i32 2
  %data353 = load ptr, ptr %list.data352, align 8
  %idx.byte.offset354 = mul i64 %index.int339, 8
  %elem.ptr355 = getelementptr inbounds i8, ptr %data353, i64 %idx.byte.offset354
  %elem356 = load ptr, ptr %elem.ptr355, align 8
  store ptr %elem356, ptr %idx.result344, align 8
  br label %idx.merge351

idx.dict349:                                      ; preds = %sout.merge321
  %dict.size357 = getelementptr inbounds nuw %RCCDict.9, ptr %payload341, i32 0, i32 1
  %size358 = load i64, ptr %dict.size357, align 4
  %dict.keys359 = getelementptr inbounds nuw %RCCDict.9, ptr %payload341, i32 0, i32 2
  %keys360 = load ptr, ptr %dict.keys359, align 8
  %dict.values361 = getelementptr inbounds nuw %RCCDict.9, ptr %payload341, i32 0, i32 3
  %vals362 = load ptr, ptr %dict.values361, align 8
  %dict.i368 = alloca i64, align 8
  store i64 0, ptr %dict.i368, align 4
  br label %dict.search.header363

idx.default350:                                   ; preds = %sout.merge321
  br label %idx.merge351

idx.merge351:                                     ; preds = %idx.default350, %dict.search.merge367, %idx.list348
  %idx.result397 = load ptr, ptr %idx.result344, align 8
  %tag.ptr398 = getelementptr inbounds nuw %RCCValue, ptr %idx.result397, i32 0, i32 0
  %tag399 = load i64, ptr %tag.ptr398, align 4
  %payload.ptr400 = getelementptr inbounds nuw %RCCValue, ptr %idx.result397, i32 0, i32 1
  %payload401 = load ptr, ptr %payload.ptr400, align 8
  switch i64 %tag399, label %sout.default407 [
    i64 1, label %sout.int402
    i64 2, label %sout.float403
    i64 3, label %sout.bool404
    i64 4, label %sout.str405
    i64 0, label %sout.null406
  ]

dict.search.header363:                            ; preds = %next.iter391, %idx.dict349
  %i369 = load i64, ptr %dict.i368, align 4
  %in.bounds370 = icmp slt i64 %i369, %size358
  br i1 %in.bounds370, label %dict.search.body364, label %dict.notfound366

dict.search.body364:                              ; preds = %dict.search.header363
  %i.body371 = load i64, ptr %dict.i368, align 4
  %key.byte.offset372 = mul i64 %i.body371, 8
  %key.gep373 = getelementptr inbounds i8, ptr %keys360, i64 %key.byte.offset372
  %key.val374 = load ptr, ptr %key.gep373, align 8
  %tag.ptr375 = getelementptr inbounds nuw %RCCValue, ptr %key.val374, i32 0, i32 0
  %tag376 = load i64, ptr %tag.ptr375, align 4
  %tag.ptr377 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap334, i32 0, i32 0
  %tag378 = load i64, ptr %tag.ptr377, align 4
  %tag.eq379 = icmp eq i64 %tag376, %tag378
  %payload.ptr380 = getelementptr inbounds nuw %RCCValue, ptr %key.val374, i32 0, i32 1
  %payload381 = load ptr, ptr %payload.ptr380, align 8
  %is.string382 = icmp eq i64 %tag376, 4
  br i1 %is.string382, label %cmp.str383, label %cmp.ptr384

dict.found365:                                    ; preds = %cmp.merge385
  %i.found393 = load i64, ptr %dict.i368, align 4
  %val.byte.offset394 = mul i64 %i.found393, 8
  %val.gep395 = getelementptr inbounds i8, ptr %vals362, i64 %val.byte.offset394
  %val396 = load ptr, ptr %val.gep395, align 8
  store ptr %val396, ptr %idx.result344, align 8
  br label %dict.search.merge367

dict.notfound366:                                 ; preds = %dict.search.header363
  br label %dict.search.merge367

dict.search.merge367:                             ; preds = %dict.notfound366, %dict.found365
  br label %idx.merge351

cmp.str383:                                       ; preds = %dict.search.body364
  %strcmp.result386 = call i32 @strcmp(ptr %payload381, ptr %payload338)
  %strcmp.eq387 = icmp eq i32 %strcmp.result386, 0
  br label %cmp.merge385

cmp.ptr384:                                       ; preds = %dict.search.body364
  %ptr.eq388 = icmp eq ptr %payload381, %payload338
  br label %cmp.merge385

cmp.merge385:                                     ; preds = %cmp.ptr384, %cmp.str383
  %payload.eq389 = phi i1 [ %strcmp.eq387, %cmp.str383 ], [ %ptr.eq388, %cmp.ptr384 ]
  %key.eq390 = and i1 %tag.eq379, %payload.eq389
  store i64 %i.body371, ptr %dict.i368, align 4
  br i1 %key.eq390, label %dict.found365, label %next.iter391

next.iter391:                                     ; preds = %cmp.merge385
  %next.i392 = add i64 %i.body371, 1
  store i64 %next.i392, ptr %dict.i368, align 4
  br label %dict.search.header363

sout.int402:                                      ; preds = %idx.merge351
  %int.val409 = ptrtoint ptr %payload401 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.51, i64 %int.val409)
  br label %sout.merge408

sout.float403:                                    ; preds = %idx.merge351
  %float.bits410 = ptrtoint ptr %payload401 to i64
  %float.val411 = bitcast i64 %float.bits410 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.52, double %float.val411)
  br label %sout.merge408

sout.bool404:                                     ; preds = %idx.merge351
  %bool.val412 = ptrtoint ptr %payload401 to i64
  %is.true413 = icmp ne i64 %bool.val412, 0
  br i1 %is.true413, label %sout.true414, label %sout.false415

sout.str405:                                      ; preds = %idx.merge351
  %44 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr %payload401)
  br label %sout.merge408

sout.null406:                                     ; preds = %idx.merge351
  %45 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.merge408

sout.default407:                                  ; preds = %idx.merge351
  %46 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload401)
  br label %sout.merge408

sout.merge408:                                    ; preds = %sout.default407, %sout.null406, %sout.str405, %sout.bool.merge416, %sout.float403, %sout.int402
  %47 = call i32 @putchar(i32 10)
  %rcc.val.heap417 = call ptr @malloc(i64 16)
  %tag.ptr418 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap417, i32 0, i32 0
  store i64 0, ptr %tag.ptr418, align 4
  %payload.ptr419 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap417, i32 0, i32 1
  store ptr null, ptr %payload.ptr419, align 8
  ret i32 0

sout.true414:                                     ; preds = %sout.bool404
  %48 = call i32 (ptr, ...) @printf(ptr @.str.53, ptr @.str.54)
  br label %sout.bool.merge416

sout.false415:                                    ; preds = %sout.bool404
  %49 = call i32 (ptr, ...) @printf(ptr @.str.55, ptr @.str.56)
  br label %sout.bool.merge416

sout.bool.merge416:                               ; preds = %sout.false415, %sout.true414
  br label %sout.merge408
}

declare ptr @malloc(i64)

declare i32 @strcmp(ptr, ptr)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)
