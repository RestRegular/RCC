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

dict.search.header:                               ; preds = %dict.search.body, %idx.dict
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
  %payload.eq = icmp eq ptr %payload33, %payload
  %key.eq = and i1 %tag.eq, %payload.eq
  %next.i = add i64 %i.body, 1
  store i64 %next.i, ptr %dict.i, align 4
  br i1 %key.eq, label %dict.found, label %dict.search.header

dict.found:                                       ; preds = %dict.search.body
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
  %idx.result99 = load ptr, ptr %idx.result54, align 8
  %tag.ptr100 = getelementptr inbounds nuw %RCCValue, ptr %idx.result99, i32 0, i32 0
  %tag101 = load i64, ptr %tag.ptr100, align 4
  %payload.ptr102 = getelementptr inbounds nuw %RCCValue, ptr %idx.result99, i32 0, i32 1
  %payload103 = load ptr, ptr %payload.ptr102, align 8
  switch i64 %tag101, label %sout.default109 [
    i64 1, label %sout.int104
    i64 2, label %sout.float105
    i64 3, label %sout.bool106
    i64 4, label %sout.str107
    i64 0, label %sout.null108
  ]

dict.search.header73:                             ; preds = %dict.search.body74, %idx.dict59
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
  %payload.eq92 = icmp eq ptr %payload91, %payload48
  %key.eq93 = and i1 %tag.eq89, %payload.eq92
  %next.i94 = add i64 %i.body81, 1
  store i64 %next.i94, ptr %dict.i78, align 4
  br i1 %key.eq93, label %dict.found75, label %dict.search.header73

dict.found75:                                     ; preds = %dict.search.body74
  %i.found95 = load i64, ptr %dict.i78, align 4
  %val.byte.offset96 = mul i64 %i.found95, 8
  %val.gep97 = getelementptr inbounds i8, ptr %vals72, i64 %val.byte.offset96
  %val98 = load ptr, ptr %val.gep97, align 8
  store ptr %val98, ptr %idx.result54, align 8
  br label %dict.search.merge77

dict.notfound76:                                  ; preds = %dict.search.header73
  br label %dict.search.merge77

dict.search.merge77:                              ; preds = %dict.notfound76, %dict.found75
  br label %idx.merge61

sout.int104:                                      ; preds = %idx.merge61
  %int.val111 = ptrtoint ptr %payload103 to i64
  %14 = call i32 (ptr, ...) @printf(ptr @.str.15, i64 %int.val111)
  br label %sout.merge110

sout.float105:                                    ; preds = %idx.merge61
  %float.bits112 = ptrtoint ptr %payload103 to i64
  %float.val113 = bitcast i64 %float.bits112 to double
  %15 = call i32 (ptr, ...) @printf(ptr @.str.16, double %float.val113)
  br label %sout.merge110

sout.bool106:                                     ; preds = %idx.merge61
  %bool.val114 = ptrtoint ptr %payload103 to i64
  %is.true115 = icmp ne i64 %bool.val114, 0
  br i1 %is.true115, label %sout.true116, label %sout.false117

sout.str107:                                      ; preds = %idx.merge61
  %16 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload103)
  br label %sout.merge110

sout.null108:                                     ; preds = %idx.merge61
  %17 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.merge110

sout.default109:                                  ; preds = %idx.merge61
  %18 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr %payload103)
  br label %sout.merge110

sout.merge110:                                    ; preds = %sout.default109, %sout.null108, %sout.str107, %sout.bool.merge118, %sout.float105, %sout.int104
  %19 = call i32 @putchar(i32 10)
  %rcc.val.heap119 = call ptr @malloc(i64 16)
  %tag.ptr120 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap119, i32 0, i32 0
  store i64 0, ptr %tag.ptr120, align 4
  %payload.ptr121 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap119, i32 0, i32 1
  store ptr null, ptr %payload.ptr121, align 8
  store ptr null, ptr %dic2, align 8
  %rcc.val.heap122 = call ptr @malloc(i64 16)
  %tag.ptr123 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap122, i32 0, i32 0
  store i64 4, ptr %tag.ptr123, align 4
  %payload.ptr124 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap122, i32 0, i32 1
  store ptr @.str.25, ptr %payload.ptr124, align 8
  %rcc.val.heap125 = call ptr @malloc(i64 16)
  %tag.ptr126 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap125, i32 0, i32 0
  store i64 1, ptr %tag.ptr126, align 4
  %payload.ptr127 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap125, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr127, align 8
  %rcc.val.heap128 = call ptr @malloc(i64 16)
  %tag.ptr129 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap128, i32 0, i32 0
  store i64 4, ptr %tag.ptr129, align 4
  %payload.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap128, i32 0, i32 1
  store ptr @.str.26, ptr %payload.ptr130, align 8
  %rcc.val.heap131 = call ptr @malloc(i64 16)
  %tag.ptr132 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap131, i32 0, i32 0
  store i64 1, ptr %tag.ptr132, align 4
  %payload.ptr133 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap131, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr133, align 8
  %rcc.val.heap134 = call ptr @malloc(i64 16)
  %tag.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap134, i32 0, i32 0
  store i64 4, ptr %tag.ptr135, align 4
  %payload.ptr136 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap134, i32 0, i32 1
  store ptr @.str.27, ptr %payload.ptr136, align 8
  %rcc.val.heap137 = call ptr @malloc(i64 16)
  %tag.ptr138 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap137, i32 0, i32 0
  store i64 1, ptr %tag.ptr138, align 4
  %payload.ptr139 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap137, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr139, align 8
  %dict.keys140 = call ptr @malloc(i64 24)
  %dict.values141 = call ptr @malloc(i64 24)
  %key.gep142 = getelementptr inbounds i8, ptr %dict.keys140, i64 0
  store ptr %rcc.val.heap122, ptr %key.gep142, align 8
  %key.gep143 = getelementptr inbounds i8, ptr %dict.keys140, i64 8
  store ptr %rcc.val.heap128, ptr %key.gep143, align 8
  %key.gep144 = getelementptr inbounds i8, ptr %dict.keys140, i64 16
  store ptr %rcc.val.heap134, ptr %key.gep144, align 8
  %val.gep145 = getelementptr inbounds i8, ptr %dict.values141, i64 0
  store ptr %rcc.val.heap125, ptr %val.gep145, align 8
  %val.gep146 = getelementptr inbounds i8, ptr %dict.values141, i64 8
  store ptr %rcc.val.heap131, ptr %val.gep146, align 8
  %val.gep147 = getelementptr inbounds i8, ptr %dict.values141, i64 16
  store ptr %rcc.val.heap137, ptr %val.gep147, align 8
  %dict.struct148 = call ptr @malloc(i64 32)
  %20 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct148, i32 0, i32 0
  store i64 3, ptr %20, align 4
  %21 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct148, i32 0, i32 1
  store i64 3, ptr %21, align 4
  %22 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct148, i32 0, i32 2
  store ptr %dict.keys140, ptr %22, align 8
  %23 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct148, i32 0, i32 3
  store ptr %dict.values141, ptr %23, align 8
  %rcc.val.heap149 = call ptr @malloc(i64 16)
  %tag.ptr150 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap149, i32 0, i32 0
  store i64 6, ptr %tag.ptr150, align 4
  %payload.ptr151 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap149, i32 0, i32 1
  store ptr %dict.struct148, ptr %payload.ptr151, align 8
  store ptr %rcc.val.heap149, ptr %dic2, align 8
  %dic2.load = load ptr, ptr %dic2, align 8
  %rcc.val.heap152 = call ptr @malloc(i64 16)
  %tag.ptr153 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap152, i32 0, i32 0
  store i64 4, ptr %tag.ptr153, align 4
  %payload.ptr154 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap152, i32 0, i32 1
  store ptr @.str.28, ptr %payload.ptr154, align 8
  %payload.ptr155 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap152, i32 0, i32 1
  %payload156 = load ptr, ptr %payload.ptr155, align 8
  %index.int157 = ptrtoint ptr %payload156 to i64
  %payload.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load, i32 0, i32 1
  %payload159 = load ptr, ptr %payload.ptr158, align 8
  %tag.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load, i32 0, i32 0
  %tag161 = load i64, ptr %tag.ptr160, align 4
  %idx.result162 = alloca ptr, align 8
  %rcc.val.heap163 = call ptr @malloc(i64 16)
  %tag.ptr164 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap163, i32 0, i32 0
  store i64 0, ptr %tag.ptr164, align 4
  %payload.ptr165 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap163, i32 0, i32 1
  store ptr null, ptr %payload.ptr165, align 8
  store ptr %rcc.val.heap163, ptr %idx.result162, align 8
  switch i64 %tag161, label %idx.default168 [
    i64 5, label %idx.list166
    i64 6, label %idx.dict167
  ]

sout.true116:                                     ; preds = %sout.bool106
  %24 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.bool.merge118

sout.false117:                                    ; preds = %sout.bool106
  %25 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.bool.merge118

sout.bool.merge118:                               ; preds = %sout.false117, %sout.true116
  br label %sout.merge110

idx.list166:                                      ; preds = %sout.merge110
  %list.data170 = getelementptr inbounds nuw %RCCList.4, ptr %payload159, i32 0, i32 2
  %data171 = load ptr, ptr %list.data170, align 8
  %idx.byte.offset172 = mul i64 %index.int157, 8
  %elem.ptr173 = getelementptr inbounds i8, ptr %data171, i64 %idx.byte.offset172
  %elem174 = load ptr, ptr %elem.ptr173, align 8
  store ptr %elem174, ptr %idx.result162, align 8
  br label %idx.merge169

idx.dict167:                                      ; preds = %sout.merge110
  %dict.size175 = getelementptr inbounds nuw %RCCDict.5, ptr %payload159, i32 0, i32 1
  %size176 = load i64, ptr %dict.size175, align 4
  %dict.keys177 = getelementptr inbounds nuw %RCCDict.5, ptr %payload159, i32 0, i32 2
  %keys178 = load ptr, ptr %dict.keys177, align 8
  %dict.values179 = getelementptr inbounds nuw %RCCDict.5, ptr %payload159, i32 0, i32 3
  %vals180 = load ptr, ptr %dict.values179, align 8
  %dict.i186 = alloca i64, align 8
  store i64 0, ptr %dict.i186, align 4
  br label %dict.search.header181

idx.default168:                                   ; preds = %sout.merge110
  br label %idx.merge169

idx.merge169:                                     ; preds = %idx.default168, %dict.search.merge185, %idx.list166
  %idx.result207 = load ptr, ptr %idx.result162, align 8
  %tag.ptr208 = getelementptr inbounds nuw %RCCValue, ptr %idx.result207, i32 0, i32 0
  %tag209 = load i64, ptr %tag.ptr208, align 4
  %payload.ptr210 = getelementptr inbounds nuw %RCCValue, ptr %idx.result207, i32 0, i32 1
  %payload211 = load ptr, ptr %payload.ptr210, align 8
  switch i64 %tag209, label %sout.default217 [
    i64 1, label %sout.int212
    i64 2, label %sout.float213
    i64 3, label %sout.bool214
    i64 4, label %sout.str215
    i64 0, label %sout.null216
  ]

dict.search.header181:                            ; preds = %dict.search.body182, %idx.dict167
  %i187 = load i64, ptr %dict.i186, align 4
  %in.bounds188 = icmp slt i64 %i187, %size176
  br i1 %in.bounds188, label %dict.search.body182, label %dict.notfound184

dict.search.body182:                              ; preds = %dict.search.header181
  %i.body189 = load i64, ptr %dict.i186, align 4
  %key.byte.offset190 = mul i64 %i.body189, 8
  %key.gep191 = getelementptr inbounds i8, ptr %keys178, i64 %key.byte.offset190
  %key.val192 = load ptr, ptr %key.gep191, align 8
  %tag.ptr193 = getelementptr inbounds nuw %RCCValue, ptr %key.val192, i32 0, i32 0
  %tag194 = load i64, ptr %tag.ptr193, align 4
  %tag.ptr195 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap152, i32 0, i32 0
  %tag196 = load i64, ptr %tag.ptr195, align 4
  %tag.eq197 = icmp eq i64 %tag194, %tag196
  %payload.ptr198 = getelementptr inbounds nuw %RCCValue, ptr %key.val192, i32 0, i32 1
  %payload199 = load ptr, ptr %payload.ptr198, align 8
  %payload.eq200 = icmp eq ptr %payload199, %payload156
  %key.eq201 = and i1 %tag.eq197, %payload.eq200
  %next.i202 = add i64 %i.body189, 1
  store i64 %next.i202, ptr %dict.i186, align 4
  br i1 %key.eq201, label %dict.found183, label %dict.search.header181

dict.found183:                                    ; preds = %dict.search.body182
  %i.found203 = load i64, ptr %dict.i186, align 4
  %val.byte.offset204 = mul i64 %i.found203, 8
  %val.gep205 = getelementptr inbounds i8, ptr %vals180, i64 %val.byte.offset204
  %val206 = load ptr, ptr %val.gep205, align 8
  store ptr %val206, ptr %idx.result162, align 8
  br label %dict.search.merge185

dict.notfound184:                                 ; preds = %dict.search.header181
  br label %dict.search.merge185

dict.search.merge185:                             ; preds = %dict.notfound184, %dict.found183
  br label %idx.merge169

sout.int212:                                      ; preds = %idx.merge169
  %int.val219 = ptrtoint ptr %payload211 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.29, i64 %int.val219)
  br label %sout.merge218

sout.float213:                                    ; preds = %idx.merge169
  %float.bits220 = ptrtoint ptr %payload211 to i64
  %float.val221 = bitcast i64 %float.bits220 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.30, double %float.val221)
  br label %sout.merge218

sout.bool214:                                     ; preds = %idx.merge169
  %bool.val222 = ptrtoint ptr %payload211 to i64
  %is.true223 = icmp ne i64 %bool.val222, 0
  br i1 %is.true223, label %sout.true224, label %sout.false225

sout.str215:                                      ; preds = %idx.merge169
  %28 = call i32 (ptr, ...) @printf(ptr @.str.35, ptr %payload211)
  br label %sout.merge218

sout.null216:                                     ; preds = %idx.merge169
  %29 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.merge218

sout.default217:                                  ; preds = %idx.merge169
  %30 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr %payload211)
  br label %sout.merge218

sout.merge218:                                    ; preds = %sout.default217, %sout.null216, %sout.str215, %sout.bool.merge226, %sout.float213, %sout.int212
  %31 = call i32 @putchar(i32 10)
  %rcc.val.heap227 = call ptr @malloc(i64 16)
  %tag.ptr228 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap227, i32 0, i32 0
  store i64 0, ptr %tag.ptr228, align 4
  %payload.ptr229 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap227, i32 0, i32 1
  store ptr null, ptr %payload.ptr229, align 8
  %dic2.load230 = load ptr, ptr %dic2, align 8
  %rcc.val.heap231 = call ptr @malloc(i64 16)
  %tag.ptr232 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap231, i32 0, i32 0
  store i64 4, ptr %tag.ptr232, align 4
  %payload.ptr233 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap231, i32 0, i32 1
  store ptr @.str.39, ptr %payload.ptr233, align 8
  %payload.ptr234 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap231, i32 0, i32 1
  %payload235 = load ptr, ptr %payload.ptr234, align 8
  %index.int236 = ptrtoint ptr %payload235 to i64
  %payload.ptr237 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load230, i32 0, i32 1
  %payload238 = load ptr, ptr %payload.ptr237, align 8
  %tag.ptr239 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load230, i32 0, i32 0
  %tag240 = load i64, ptr %tag.ptr239, align 4
  %idx.result241 = alloca ptr, align 8
  %rcc.val.heap242 = call ptr @malloc(i64 16)
  %tag.ptr243 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap242, i32 0, i32 0
  store i64 0, ptr %tag.ptr243, align 4
  %payload.ptr244 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap242, i32 0, i32 1
  store ptr null, ptr %payload.ptr244, align 8
  store ptr %rcc.val.heap242, ptr %idx.result241, align 8
  switch i64 %tag240, label %idx.default247 [
    i64 5, label %idx.list245
    i64 6, label %idx.dict246
  ]

sout.true224:                                     ; preds = %sout.bool214
  %32 = call i32 (ptr, ...) @printf(ptr @.str.31, ptr @.str.32)
  br label %sout.bool.merge226

sout.false225:                                    ; preds = %sout.bool214
  %33 = call i32 (ptr, ...) @printf(ptr @.str.33, ptr @.str.34)
  br label %sout.bool.merge226

sout.bool.merge226:                               ; preds = %sout.false225, %sout.true224
  br label %sout.merge218

idx.list245:                                      ; preds = %sout.merge218
  %list.data249 = getelementptr inbounds nuw %RCCList.6, ptr %payload238, i32 0, i32 2
  %data250 = load ptr, ptr %list.data249, align 8
  %idx.byte.offset251 = mul i64 %index.int236, 8
  %elem.ptr252 = getelementptr inbounds i8, ptr %data250, i64 %idx.byte.offset251
  %elem253 = load ptr, ptr %elem.ptr252, align 8
  store ptr %elem253, ptr %idx.result241, align 8
  br label %idx.merge248

idx.dict246:                                      ; preds = %sout.merge218
  %dict.size254 = getelementptr inbounds nuw %RCCDict.7, ptr %payload238, i32 0, i32 1
  %size255 = load i64, ptr %dict.size254, align 4
  %dict.keys256 = getelementptr inbounds nuw %RCCDict.7, ptr %payload238, i32 0, i32 2
  %keys257 = load ptr, ptr %dict.keys256, align 8
  %dict.values258 = getelementptr inbounds nuw %RCCDict.7, ptr %payload238, i32 0, i32 3
  %vals259 = load ptr, ptr %dict.values258, align 8
  %dict.i265 = alloca i64, align 8
  store i64 0, ptr %dict.i265, align 4
  br label %dict.search.header260

idx.default247:                                   ; preds = %sout.merge218
  br label %idx.merge248

idx.merge248:                                     ; preds = %idx.default247, %dict.search.merge264, %idx.list245
  %idx.result286 = load ptr, ptr %idx.result241, align 8
  %tag.ptr287 = getelementptr inbounds nuw %RCCValue, ptr %idx.result286, i32 0, i32 0
  %tag288 = load i64, ptr %tag.ptr287, align 4
  %payload.ptr289 = getelementptr inbounds nuw %RCCValue, ptr %idx.result286, i32 0, i32 1
  %payload290 = load ptr, ptr %payload.ptr289, align 8
  switch i64 %tag288, label %sout.default296 [
    i64 1, label %sout.int291
    i64 2, label %sout.float292
    i64 3, label %sout.bool293
    i64 4, label %sout.str294
    i64 0, label %sout.null295
  ]

dict.search.header260:                            ; preds = %dict.search.body261, %idx.dict246
  %i266 = load i64, ptr %dict.i265, align 4
  %in.bounds267 = icmp slt i64 %i266, %size255
  br i1 %in.bounds267, label %dict.search.body261, label %dict.notfound263

dict.search.body261:                              ; preds = %dict.search.header260
  %i.body268 = load i64, ptr %dict.i265, align 4
  %key.byte.offset269 = mul i64 %i.body268, 8
  %key.gep270 = getelementptr inbounds i8, ptr %keys257, i64 %key.byte.offset269
  %key.val271 = load ptr, ptr %key.gep270, align 8
  %tag.ptr272 = getelementptr inbounds nuw %RCCValue, ptr %key.val271, i32 0, i32 0
  %tag273 = load i64, ptr %tag.ptr272, align 4
  %tag.ptr274 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap231, i32 0, i32 0
  %tag275 = load i64, ptr %tag.ptr274, align 4
  %tag.eq276 = icmp eq i64 %tag273, %tag275
  %payload.ptr277 = getelementptr inbounds nuw %RCCValue, ptr %key.val271, i32 0, i32 1
  %payload278 = load ptr, ptr %payload.ptr277, align 8
  %payload.eq279 = icmp eq ptr %payload278, %payload235
  %key.eq280 = and i1 %tag.eq276, %payload.eq279
  %next.i281 = add i64 %i.body268, 1
  store i64 %next.i281, ptr %dict.i265, align 4
  br i1 %key.eq280, label %dict.found262, label %dict.search.header260

dict.found262:                                    ; preds = %dict.search.body261
  %i.found282 = load i64, ptr %dict.i265, align 4
  %val.byte.offset283 = mul i64 %i.found282, 8
  %val.gep284 = getelementptr inbounds i8, ptr %vals259, i64 %val.byte.offset283
  %val285 = load ptr, ptr %val.gep284, align 8
  store ptr %val285, ptr %idx.result241, align 8
  br label %dict.search.merge264

dict.notfound263:                                 ; preds = %dict.search.header260
  br label %dict.search.merge264

dict.search.merge264:                             ; preds = %dict.notfound263, %dict.found262
  br label %idx.merge248

sout.int291:                                      ; preds = %idx.merge248
  %int.val298 = ptrtoint ptr %payload290 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, i64 %int.val298)
  br label %sout.merge297

sout.float292:                                    ; preds = %idx.merge248
  %float.bits299 = ptrtoint ptr %payload290 to i64
  %float.val300 = bitcast i64 %float.bits299 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val300)
  br label %sout.merge297

sout.bool293:                                     ; preds = %idx.merge248
  %bool.val301 = ptrtoint ptr %payload290 to i64
  %is.true302 = icmp ne i64 %bool.val301, 0
  br i1 %is.true302, label %sout.true303, label %sout.false304

sout.str294:                                      ; preds = %idx.merge248
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload290)
  br label %sout.merge297

sout.null295:                                     ; preds = %idx.merge248
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge297

sout.default296:                                  ; preds = %idx.merge248
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload290)
  br label %sout.merge297

sout.merge297:                                    ; preds = %sout.default296, %sout.null295, %sout.str294, %sout.bool.merge305, %sout.float292, %sout.int291
  %39 = call i32 @putchar(i32 10)
  %rcc.val.heap306 = call ptr @malloc(i64 16)
  %tag.ptr307 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap306, i32 0, i32 0
  store i64 0, ptr %tag.ptr307, align 4
  %payload.ptr308 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap306, i32 0, i32 1
  store ptr null, ptr %payload.ptr308, align 8
  %dic2.load309 = load ptr, ptr %dic2, align 8
  %rcc.val.heap310 = call ptr @malloc(i64 16)
  %tag.ptr311 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap310, i32 0, i32 0
  store i64 4, ptr %tag.ptr311, align 4
  %payload.ptr312 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap310, i32 0, i32 1
  store ptr @.str.50, ptr %payload.ptr312, align 8
  %payload.ptr313 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap310, i32 0, i32 1
  %payload314 = load ptr, ptr %payload.ptr313, align 8
  %index.int315 = ptrtoint ptr %payload314 to i64
  %payload.ptr316 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load309, i32 0, i32 1
  %payload317 = load ptr, ptr %payload.ptr316, align 8
  %tag.ptr318 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load309, i32 0, i32 0
  %tag319 = load i64, ptr %tag.ptr318, align 4
  %idx.result320 = alloca ptr, align 8
  %rcc.val.heap321 = call ptr @malloc(i64 16)
  %tag.ptr322 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap321, i32 0, i32 0
  store i64 0, ptr %tag.ptr322, align 4
  %payload.ptr323 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap321, i32 0, i32 1
  store ptr null, ptr %payload.ptr323, align 8
  store ptr %rcc.val.heap321, ptr %idx.result320, align 8
  switch i64 %tag319, label %idx.default326 [
    i64 5, label %idx.list324
    i64 6, label %idx.dict325
  ]

sout.true303:                                     ; preds = %sout.bool293
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge305

sout.false304:                                    ; preds = %sout.bool293
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge305

sout.bool.merge305:                               ; preds = %sout.false304, %sout.true303
  br label %sout.merge297

idx.list324:                                      ; preds = %sout.merge297
  %list.data328 = getelementptr inbounds nuw %RCCList.8, ptr %payload317, i32 0, i32 2
  %data329 = load ptr, ptr %list.data328, align 8
  %idx.byte.offset330 = mul i64 %index.int315, 8
  %elem.ptr331 = getelementptr inbounds i8, ptr %data329, i64 %idx.byte.offset330
  %elem332 = load ptr, ptr %elem.ptr331, align 8
  store ptr %elem332, ptr %idx.result320, align 8
  br label %idx.merge327

idx.dict325:                                      ; preds = %sout.merge297
  %dict.size333 = getelementptr inbounds nuw %RCCDict.9, ptr %payload317, i32 0, i32 1
  %size334 = load i64, ptr %dict.size333, align 4
  %dict.keys335 = getelementptr inbounds nuw %RCCDict.9, ptr %payload317, i32 0, i32 2
  %keys336 = load ptr, ptr %dict.keys335, align 8
  %dict.values337 = getelementptr inbounds nuw %RCCDict.9, ptr %payload317, i32 0, i32 3
  %vals338 = load ptr, ptr %dict.values337, align 8
  %dict.i344 = alloca i64, align 8
  store i64 0, ptr %dict.i344, align 4
  br label %dict.search.header339

idx.default326:                                   ; preds = %sout.merge297
  br label %idx.merge327

idx.merge327:                                     ; preds = %idx.default326, %dict.search.merge343, %idx.list324
  %idx.result365 = load ptr, ptr %idx.result320, align 8
  %tag.ptr366 = getelementptr inbounds nuw %RCCValue, ptr %idx.result365, i32 0, i32 0
  %tag367 = load i64, ptr %tag.ptr366, align 4
  %payload.ptr368 = getelementptr inbounds nuw %RCCValue, ptr %idx.result365, i32 0, i32 1
  %payload369 = load ptr, ptr %payload.ptr368, align 8
  switch i64 %tag367, label %sout.default375 [
    i64 1, label %sout.int370
    i64 2, label %sout.float371
    i64 3, label %sout.bool372
    i64 4, label %sout.str373
    i64 0, label %sout.null374
  ]

dict.search.header339:                            ; preds = %dict.search.body340, %idx.dict325
  %i345 = load i64, ptr %dict.i344, align 4
  %in.bounds346 = icmp slt i64 %i345, %size334
  br i1 %in.bounds346, label %dict.search.body340, label %dict.notfound342

dict.search.body340:                              ; preds = %dict.search.header339
  %i.body347 = load i64, ptr %dict.i344, align 4
  %key.byte.offset348 = mul i64 %i.body347, 8
  %key.gep349 = getelementptr inbounds i8, ptr %keys336, i64 %key.byte.offset348
  %key.val350 = load ptr, ptr %key.gep349, align 8
  %tag.ptr351 = getelementptr inbounds nuw %RCCValue, ptr %key.val350, i32 0, i32 0
  %tag352 = load i64, ptr %tag.ptr351, align 4
  %tag.ptr353 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap310, i32 0, i32 0
  %tag354 = load i64, ptr %tag.ptr353, align 4
  %tag.eq355 = icmp eq i64 %tag352, %tag354
  %payload.ptr356 = getelementptr inbounds nuw %RCCValue, ptr %key.val350, i32 0, i32 1
  %payload357 = load ptr, ptr %payload.ptr356, align 8
  %payload.eq358 = icmp eq ptr %payload357, %payload314
  %key.eq359 = and i1 %tag.eq355, %payload.eq358
  %next.i360 = add i64 %i.body347, 1
  store i64 %next.i360, ptr %dict.i344, align 4
  br i1 %key.eq359, label %dict.found341, label %dict.search.header339

dict.found341:                                    ; preds = %dict.search.body340
  %i.found361 = load i64, ptr %dict.i344, align 4
  %val.byte.offset362 = mul i64 %i.found361, 8
  %val.gep363 = getelementptr inbounds i8, ptr %vals338, i64 %val.byte.offset362
  %val364 = load ptr, ptr %val.gep363, align 8
  store ptr %val364, ptr %idx.result320, align 8
  br label %dict.search.merge343

dict.notfound342:                                 ; preds = %dict.search.header339
  br label %dict.search.merge343

dict.search.merge343:                             ; preds = %dict.notfound342, %dict.found341
  br label %idx.merge327

sout.int370:                                      ; preds = %idx.merge327
  %int.val377 = ptrtoint ptr %payload369 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.51, i64 %int.val377)
  br label %sout.merge376

sout.float371:                                    ; preds = %idx.merge327
  %float.bits378 = ptrtoint ptr %payload369 to i64
  %float.val379 = bitcast i64 %float.bits378 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.52, double %float.val379)
  br label %sout.merge376

sout.bool372:                                     ; preds = %idx.merge327
  %bool.val380 = ptrtoint ptr %payload369 to i64
  %is.true381 = icmp ne i64 %bool.val380, 0
  br i1 %is.true381, label %sout.true382, label %sout.false383

sout.str373:                                      ; preds = %idx.merge327
  %44 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr %payload369)
  br label %sout.merge376

sout.null374:                                     ; preds = %idx.merge327
  %45 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.merge376

sout.default375:                                  ; preds = %idx.merge327
  %46 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload369)
  br label %sout.merge376

sout.merge376:                                    ; preds = %sout.default375, %sout.null374, %sout.str373, %sout.bool.merge384, %sout.float371, %sout.int370
  %47 = call i32 @putchar(i32 10)
  %rcc.val.heap385 = call ptr @malloc(i64 16)
  %tag.ptr386 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap385, i32 0, i32 0
  store i64 0, ptr %tag.ptr386, align 4
  %payload.ptr387 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap385, i32 0, i32 1
  store ptr null, ptr %payload.ptr387, align 8
  ret i32 0

sout.true382:                                     ; preds = %sout.bool372
  %48 = call i32 (ptr, ...) @printf(ptr @.str.53, ptr @.str.54)
  br label %sout.bool.merge384

sout.false383:                                    ; preds = %sout.bool372
  %49 = call i32 (ptr, ...) @printf(ptr @.str.55, ptr @.str.56)
  br label %sout.bool.merge384

sout.bool.merge384:                               ; preds = %sout.false383, %sout.true382
  br label %sout.merge376
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
