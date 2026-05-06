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
  %is.string = icmp eq i64 %tag29, 4
  %strcmp.result = call i32 @strcmp(ptr %payload33, ptr %payload)
  %strcmp.eq = icmp eq i32 %strcmp.result, 0
  %ptr.eq = icmp eq ptr %payload33, %payload
  %payload.eq = select i1 %is.string, i1 %strcmp.eq, i1 %ptr.eq
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
  %idx.result103 = load ptr, ptr %idx.result54, align 8
  %tag.ptr104 = getelementptr inbounds nuw %RCCValue, ptr %idx.result103, i32 0, i32 0
  %tag105 = load i64, ptr %tag.ptr104, align 4
  %payload.ptr106 = getelementptr inbounds nuw %RCCValue, ptr %idx.result103, i32 0, i32 1
  %payload107 = load ptr, ptr %payload.ptr106, align 8
  switch i64 %tag105, label %sout.default113 [
    i64 1, label %sout.int108
    i64 2, label %sout.float109
    i64 3, label %sout.bool110
    i64 4, label %sout.str111
    i64 0, label %sout.null112
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
  %is.string92 = icmp eq i64 %tag86, 4
  %strcmp.result93 = call i32 @strcmp(ptr %payload91, ptr %payload48)
  %strcmp.eq94 = icmp eq i32 %strcmp.result93, 0
  %ptr.eq95 = icmp eq ptr %payload91, %payload48
  %payload.eq96 = select i1 %is.string92, i1 %strcmp.eq94, i1 %ptr.eq95
  %key.eq97 = and i1 %tag.eq89, %payload.eq96
  %next.i98 = add i64 %i.body81, 1
  store i64 %next.i98, ptr %dict.i78, align 4
  br i1 %key.eq97, label %dict.found75, label %dict.search.header73

dict.found75:                                     ; preds = %dict.search.body74
  %i.found99 = load i64, ptr %dict.i78, align 4
  %val.byte.offset100 = mul i64 %i.found99, 8
  %val.gep101 = getelementptr inbounds i8, ptr %vals72, i64 %val.byte.offset100
  %val102 = load ptr, ptr %val.gep101, align 8
  store ptr %val102, ptr %idx.result54, align 8
  br label %dict.search.merge77

dict.notfound76:                                  ; preds = %dict.search.header73
  br label %dict.search.merge77

dict.search.merge77:                              ; preds = %dict.notfound76, %dict.found75
  br label %idx.merge61

sout.int108:                                      ; preds = %idx.merge61
  %int.val115 = ptrtoint ptr %payload107 to i64
  %14 = call i32 (ptr, ...) @printf(ptr @.str.15, i64 %int.val115)
  br label %sout.merge114

sout.float109:                                    ; preds = %idx.merge61
  %float.bits116 = ptrtoint ptr %payload107 to i64
  %float.val117 = bitcast i64 %float.bits116 to double
  %15 = call i32 (ptr, ...) @printf(ptr @.str.16, double %float.val117)
  br label %sout.merge114

sout.bool110:                                     ; preds = %idx.merge61
  %bool.val118 = ptrtoint ptr %payload107 to i64
  %is.true119 = icmp ne i64 %bool.val118, 0
  br i1 %is.true119, label %sout.true120, label %sout.false121

sout.str111:                                      ; preds = %idx.merge61
  %16 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload107)
  br label %sout.merge114

sout.null112:                                     ; preds = %idx.merge61
  %17 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.merge114

sout.default113:                                  ; preds = %idx.merge61
  %18 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr %payload107)
  br label %sout.merge114

sout.merge114:                                    ; preds = %sout.default113, %sout.null112, %sout.str111, %sout.bool.merge122, %sout.float109, %sout.int108
  %19 = call i32 @putchar(i32 10)
  %rcc.val.heap123 = call ptr @malloc(i64 16)
  %tag.ptr124 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap123, i32 0, i32 0
  store i64 0, ptr %tag.ptr124, align 4
  %payload.ptr125 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap123, i32 0, i32 1
  store ptr null, ptr %payload.ptr125, align 8
  store ptr null, ptr %dic2, align 8
  %rcc.val.heap126 = call ptr @malloc(i64 16)
  %tag.ptr127 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap126, i32 0, i32 0
  store i64 4, ptr %tag.ptr127, align 4
  %payload.ptr128 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap126, i32 0, i32 1
  store ptr @.str.25, ptr %payload.ptr128, align 8
  %rcc.val.heap129 = call ptr @malloc(i64 16)
  %tag.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap129, i32 0, i32 0
  store i64 1, ptr %tag.ptr130, align 4
  %payload.ptr131 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap129, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr131, align 8
  %rcc.val.heap132 = call ptr @malloc(i64 16)
  %tag.ptr133 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap132, i32 0, i32 0
  store i64 4, ptr %tag.ptr133, align 4
  %payload.ptr134 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap132, i32 0, i32 1
  store ptr @.str.26, ptr %payload.ptr134, align 8
  %rcc.val.heap135 = call ptr @malloc(i64 16)
  %tag.ptr136 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap135, i32 0, i32 0
  store i64 1, ptr %tag.ptr136, align 4
  %payload.ptr137 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap135, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr137, align 8
  %rcc.val.heap138 = call ptr @malloc(i64 16)
  %tag.ptr139 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap138, i32 0, i32 0
  store i64 4, ptr %tag.ptr139, align 4
  %payload.ptr140 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap138, i32 0, i32 1
  store ptr @.str.27, ptr %payload.ptr140, align 8
  %rcc.val.heap141 = call ptr @malloc(i64 16)
  %tag.ptr142 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap141, i32 0, i32 0
  store i64 1, ptr %tag.ptr142, align 4
  %payload.ptr143 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap141, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr143, align 8
  %dict.keys144 = call ptr @malloc(i64 24)
  %dict.values145 = call ptr @malloc(i64 24)
  %key.gep146 = getelementptr inbounds i8, ptr %dict.keys144, i64 0
  store ptr %rcc.val.heap126, ptr %key.gep146, align 8
  %key.gep147 = getelementptr inbounds i8, ptr %dict.keys144, i64 8
  store ptr %rcc.val.heap132, ptr %key.gep147, align 8
  %key.gep148 = getelementptr inbounds i8, ptr %dict.keys144, i64 16
  store ptr %rcc.val.heap138, ptr %key.gep148, align 8
  %val.gep149 = getelementptr inbounds i8, ptr %dict.values145, i64 0
  store ptr %rcc.val.heap129, ptr %val.gep149, align 8
  %val.gep150 = getelementptr inbounds i8, ptr %dict.values145, i64 8
  store ptr %rcc.val.heap135, ptr %val.gep150, align 8
  %val.gep151 = getelementptr inbounds i8, ptr %dict.values145, i64 16
  store ptr %rcc.val.heap141, ptr %val.gep151, align 8
  %dict.struct152 = call ptr @malloc(i64 32)
  %20 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct152, i32 0, i32 0
  store i64 3, ptr %20, align 4
  %21 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct152, i32 0, i32 1
  store i64 3, ptr %21, align 4
  %22 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct152, i32 0, i32 2
  store ptr %dict.keys144, ptr %22, align 8
  %23 = getelementptr inbounds nuw %RCCDict.3, ptr %dict.struct152, i32 0, i32 3
  store ptr %dict.values145, ptr %23, align 8
  %rcc.val.heap153 = call ptr @malloc(i64 16)
  %tag.ptr154 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap153, i32 0, i32 0
  store i64 6, ptr %tag.ptr154, align 4
  %payload.ptr155 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap153, i32 0, i32 1
  store ptr %dict.struct152, ptr %payload.ptr155, align 8
  store ptr %rcc.val.heap153, ptr %dic2, align 8
  %dic2.load = load ptr, ptr %dic2, align 8
  %rcc.val.heap156 = call ptr @malloc(i64 16)
  %tag.ptr157 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap156, i32 0, i32 0
  store i64 4, ptr %tag.ptr157, align 4
  %payload.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap156, i32 0, i32 1
  store ptr @.str.28, ptr %payload.ptr158, align 8
  %payload.ptr159 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap156, i32 0, i32 1
  %payload160 = load ptr, ptr %payload.ptr159, align 8
  %index.int161 = ptrtoint ptr %payload160 to i64
  %payload.ptr162 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load, i32 0, i32 1
  %payload163 = load ptr, ptr %payload.ptr162, align 8
  %tag.ptr164 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load, i32 0, i32 0
  %tag165 = load i64, ptr %tag.ptr164, align 4
  %idx.result166 = alloca ptr, align 8
  %rcc.val.heap167 = call ptr @malloc(i64 16)
  %tag.ptr168 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap167, i32 0, i32 0
  store i64 0, ptr %tag.ptr168, align 4
  %payload.ptr169 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap167, i32 0, i32 1
  store ptr null, ptr %payload.ptr169, align 8
  store ptr %rcc.val.heap167, ptr %idx.result166, align 8
  switch i64 %tag165, label %idx.default172 [
    i64 5, label %idx.list170
    i64 6, label %idx.dict171
  ]

sout.true120:                                     ; preds = %sout.bool110
  %24 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.bool.merge122

sout.false121:                                    ; preds = %sout.bool110
  %25 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.bool.merge122

sout.bool.merge122:                               ; preds = %sout.false121, %sout.true120
  br label %sout.merge114

idx.list170:                                      ; preds = %sout.merge114
  %list.data174 = getelementptr inbounds nuw %RCCList.4, ptr %payload163, i32 0, i32 2
  %data175 = load ptr, ptr %list.data174, align 8
  %idx.byte.offset176 = mul i64 %index.int161, 8
  %elem.ptr177 = getelementptr inbounds i8, ptr %data175, i64 %idx.byte.offset176
  %elem178 = load ptr, ptr %elem.ptr177, align 8
  store ptr %elem178, ptr %idx.result166, align 8
  br label %idx.merge173

idx.dict171:                                      ; preds = %sout.merge114
  %dict.size179 = getelementptr inbounds nuw %RCCDict.5, ptr %payload163, i32 0, i32 1
  %size180 = load i64, ptr %dict.size179, align 4
  %dict.keys181 = getelementptr inbounds nuw %RCCDict.5, ptr %payload163, i32 0, i32 2
  %keys182 = load ptr, ptr %dict.keys181, align 8
  %dict.values183 = getelementptr inbounds nuw %RCCDict.5, ptr %payload163, i32 0, i32 3
  %vals184 = load ptr, ptr %dict.values183, align 8
  %dict.i190 = alloca i64, align 8
  store i64 0, ptr %dict.i190, align 4
  br label %dict.search.header185

idx.default172:                                   ; preds = %sout.merge114
  br label %idx.merge173

idx.merge173:                                     ; preds = %idx.default172, %dict.search.merge189, %idx.list170
  %idx.result215 = load ptr, ptr %idx.result166, align 8
  %tag.ptr216 = getelementptr inbounds nuw %RCCValue, ptr %idx.result215, i32 0, i32 0
  %tag217 = load i64, ptr %tag.ptr216, align 4
  %payload.ptr218 = getelementptr inbounds nuw %RCCValue, ptr %idx.result215, i32 0, i32 1
  %payload219 = load ptr, ptr %payload.ptr218, align 8
  switch i64 %tag217, label %sout.default225 [
    i64 1, label %sout.int220
    i64 2, label %sout.float221
    i64 3, label %sout.bool222
    i64 4, label %sout.str223
    i64 0, label %sout.null224
  ]

dict.search.header185:                            ; preds = %dict.search.body186, %idx.dict171
  %i191 = load i64, ptr %dict.i190, align 4
  %in.bounds192 = icmp slt i64 %i191, %size180
  br i1 %in.bounds192, label %dict.search.body186, label %dict.notfound188

dict.search.body186:                              ; preds = %dict.search.header185
  %i.body193 = load i64, ptr %dict.i190, align 4
  %key.byte.offset194 = mul i64 %i.body193, 8
  %key.gep195 = getelementptr inbounds i8, ptr %keys182, i64 %key.byte.offset194
  %key.val196 = load ptr, ptr %key.gep195, align 8
  %tag.ptr197 = getelementptr inbounds nuw %RCCValue, ptr %key.val196, i32 0, i32 0
  %tag198 = load i64, ptr %tag.ptr197, align 4
  %tag.ptr199 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap156, i32 0, i32 0
  %tag200 = load i64, ptr %tag.ptr199, align 4
  %tag.eq201 = icmp eq i64 %tag198, %tag200
  %payload.ptr202 = getelementptr inbounds nuw %RCCValue, ptr %key.val196, i32 0, i32 1
  %payload203 = load ptr, ptr %payload.ptr202, align 8
  %is.string204 = icmp eq i64 %tag198, 4
  %strcmp.result205 = call i32 @strcmp(ptr %payload203, ptr %payload160)
  %strcmp.eq206 = icmp eq i32 %strcmp.result205, 0
  %ptr.eq207 = icmp eq ptr %payload203, %payload160
  %payload.eq208 = select i1 %is.string204, i1 %strcmp.eq206, i1 %ptr.eq207
  %key.eq209 = and i1 %tag.eq201, %payload.eq208
  %next.i210 = add i64 %i.body193, 1
  store i64 %next.i210, ptr %dict.i190, align 4
  br i1 %key.eq209, label %dict.found187, label %dict.search.header185

dict.found187:                                    ; preds = %dict.search.body186
  %i.found211 = load i64, ptr %dict.i190, align 4
  %val.byte.offset212 = mul i64 %i.found211, 8
  %val.gep213 = getelementptr inbounds i8, ptr %vals184, i64 %val.byte.offset212
  %val214 = load ptr, ptr %val.gep213, align 8
  store ptr %val214, ptr %idx.result166, align 8
  br label %dict.search.merge189

dict.notfound188:                                 ; preds = %dict.search.header185
  br label %dict.search.merge189

dict.search.merge189:                             ; preds = %dict.notfound188, %dict.found187
  br label %idx.merge173

sout.int220:                                      ; preds = %idx.merge173
  %int.val227 = ptrtoint ptr %payload219 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.29, i64 %int.val227)
  br label %sout.merge226

sout.float221:                                    ; preds = %idx.merge173
  %float.bits228 = ptrtoint ptr %payload219 to i64
  %float.val229 = bitcast i64 %float.bits228 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.30, double %float.val229)
  br label %sout.merge226

sout.bool222:                                     ; preds = %idx.merge173
  %bool.val230 = ptrtoint ptr %payload219 to i64
  %is.true231 = icmp ne i64 %bool.val230, 0
  br i1 %is.true231, label %sout.true232, label %sout.false233

sout.str223:                                      ; preds = %idx.merge173
  %28 = call i32 (ptr, ...) @printf(ptr @.str.35, ptr %payload219)
  br label %sout.merge226

sout.null224:                                     ; preds = %idx.merge173
  %29 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.merge226

sout.default225:                                  ; preds = %idx.merge173
  %30 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr %payload219)
  br label %sout.merge226

sout.merge226:                                    ; preds = %sout.default225, %sout.null224, %sout.str223, %sout.bool.merge234, %sout.float221, %sout.int220
  %31 = call i32 @putchar(i32 10)
  %rcc.val.heap235 = call ptr @malloc(i64 16)
  %tag.ptr236 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap235, i32 0, i32 0
  store i64 0, ptr %tag.ptr236, align 4
  %payload.ptr237 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap235, i32 0, i32 1
  store ptr null, ptr %payload.ptr237, align 8
  %dic2.load238 = load ptr, ptr %dic2, align 8
  %rcc.val.heap239 = call ptr @malloc(i64 16)
  %tag.ptr240 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap239, i32 0, i32 0
  store i64 4, ptr %tag.ptr240, align 4
  %payload.ptr241 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap239, i32 0, i32 1
  store ptr @.str.39, ptr %payload.ptr241, align 8
  %payload.ptr242 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap239, i32 0, i32 1
  %payload243 = load ptr, ptr %payload.ptr242, align 8
  %index.int244 = ptrtoint ptr %payload243 to i64
  %payload.ptr245 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load238, i32 0, i32 1
  %payload246 = load ptr, ptr %payload.ptr245, align 8
  %tag.ptr247 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load238, i32 0, i32 0
  %tag248 = load i64, ptr %tag.ptr247, align 4
  %idx.result249 = alloca ptr, align 8
  %rcc.val.heap250 = call ptr @malloc(i64 16)
  %tag.ptr251 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap250, i32 0, i32 0
  store i64 0, ptr %tag.ptr251, align 4
  %payload.ptr252 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap250, i32 0, i32 1
  store ptr null, ptr %payload.ptr252, align 8
  store ptr %rcc.val.heap250, ptr %idx.result249, align 8
  switch i64 %tag248, label %idx.default255 [
    i64 5, label %idx.list253
    i64 6, label %idx.dict254
  ]

sout.true232:                                     ; preds = %sout.bool222
  %32 = call i32 (ptr, ...) @printf(ptr @.str.31, ptr @.str.32)
  br label %sout.bool.merge234

sout.false233:                                    ; preds = %sout.bool222
  %33 = call i32 (ptr, ...) @printf(ptr @.str.33, ptr @.str.34)
  br label %sout.bool.merge234

sout.bool.merge234:                               ; preds = %sout.false233, %sout.true232
  br label %sout.merge226

idx.list253:                                      ; preds = %sout.merge226
  %list.data257 = getelementptr inbounds nuw %RCCList.6, ptr %payload246, i32 0, i32 2
  %data258 = load ptr, ptr %list.data257, align 8
  %idx.byte.offset259 = mul i64 %index.int244, 8
  %elem.ptr260 = getelementptr inbounds i8, ptr %data258, i64 %idx.byte.offset259
  %elem261 = load ptr, ptr %elem.ptr260, align 8
  store ptr %elem261, ptr %idx.result249, align 8
  br label %idx.merge256

idx.dict254:                                      ; preds = %sout.merge226
  %dict.size262 = getelementptr inbounds nuw %RCCDict.7, ptr %payload246, i32 0, i32 1
  %size263 = load i64, ptr %dict.size262, align 4
  %dict.keys264 = getelementptr inbounds nuw %RCCDict.7, ptr %payload246, i32 0, i32 2
  %keys265 = load ptr, ptr %dict.keys264, align 8
  %dict.values266 = getelementptr inbounds nuw %RCCDict.7, ptr %payload246, i32 0, i32 3
  %vals267 = load ptr, ptr %dict.values266, align 8
  %dict.i273 = alloca i64, align 8
  store i64 0, ptr %dict.i273, align 4
  br label %dict.search.header268

idx.default255:                                   ; preds = %sout.merge226
  br label %idx.merge256

idx.merge256:                                     ; preds = %idx.default255, %dict.search.merge272, %idx.list253
  %idx.result298 = load ptr, ptr %idx.result249, align 8
  %tag.ptr299 = getelementptr inbounds nuw %RCCValue, ptr %idx.result298, i32 0, i32 0
  %tag300 = load i64, ptr %tag.ptr299, align 4
  %payload.ptr301 = getelementptr inbounds nuw %RCCValue, ptr %idx.result298, i32 0, i32 1
  %payload302 = load ptr, ptr %payload.ptr301, align 8
  switch i64 %tag300, label %sout.default308 [
    i64 1, label %sout.int303
    i64 2, label %sout.float304
    i64 3, label %sout.bool305
    i64 4, label %sout.str306
    i64 0, label %sout.null307
  ]

dict.search.header268:                            ; preds = %dict.search.body269, %idx.dict254
  %i274 = load i64, ptr %dict.i273, align 4
  %in.bounds275 = icmp slt i64 %i274, %size263
  br i1 %in.bounds275, label %dict.search.body269, label %dict.notfound271

dict.search.body269:                              ; preds = %dict.search.header268
  %i.body276 = load i64, ptr %dict.i273, align 4
  %key.byte.offset277 = mul i64 %i.body276, 8
  %key.gep278 = getelementptr inbounds i8, ptr %keys265, i64 %key.byte.offset277
  %key.val279 = load ptr, ptr %key.gep278, align 8
  %tag.ptr280 = getelementptr inbounds nuw %RCCValue, ptr %key.val279, i32 0, i32 0
  %tag281 = load i64, ptr %tag.ptr280, align 4
  %tag.ptr282 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap239, i32 0, i32 0
  %tag283 = load i64, ptr %tag.ptr282, align 4
  %tag.eq284 = icmp eq i64 %tag281, %tag283
  %payload.ptr285 = getelementptr inbounds nuw %RCCValue, ptr %key.val279, i32 0, i32 1
  %payload286 = load ptr, ptr %payload.ptr285, align 8
  %is.string287 = icmp eq i64 %tag281, 4
  %strcmp.result288 = call i32 @strcmp(ptr %payload286, ptr %payload243)
  %strcmp.eq289 = icmp eq i32 %strcmp.result288, 0
  %ptr.eq290 = icmp eq ptr %payload286, %payload243
  %payload.eq291 = select i1 %is.string287, i1 %strcmp.eq289, i1 %ptr.eq290
  %key.eq292 = and i1 %tag.eq284, %payload.eq291
  %next.i293 = add i64 %i.body276, 1
  store i64 %next.i293, ptr %dict.i273, align 4
  br i1 %key.eq292, label %dict.found270, label %dict.search.header268

dict.found270:                                    ; preds = %dict.search.body269
  %i.found294 = load i64, ptr %dict.i273, align 4
  %val.byte.offset295 = mul i64 %i.found294, 8
  %val.gep296 = getelementptr inbounds i8, ptr %vals267, i64 %val.byte.offset295
  %val297 = load ptr, ptr %val.gep296, align 8
  store ptr %val297, ptr %idx.result249, align 8
  br label %dict.search.merge272

dict.notfound271:                                 ; preds = %dict.search.header268
  br label %dict.search.merge272

dict.search.merge272:                             ; preds = %dict.notfound271, %dict.found270
  br label %idx.merge256

sout.int303:                                      ; preds = %idx.merge256
  %int.val310 = ptrtoint ptr %payload302 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, i64 %int.val310)
  br label %sout.merge309

sout.float304:                                    ; preds = %idx.merge256
  %float.bits311 = ptrtoint ptr %payload302 to i64
  %float.val312 = bitcast i64 %float.bits311 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val312)
  br label %sout.merge309

sout.bool305:                                     ; preds = %idx.merge256
  %bool.val313 = ptrtoint ptr %payload302 to i64
  %is.true314 = icmp ne i64 %bool.val313, 0
  br i1 %is.true314, label %sout.true315, label %sout.false316

sout.str306:                                      ; preds = %idx.merge256
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload302)
  br label %sout.merge309

sout.null307:                                     ; preds = %idx.merge256
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge309

sout.default308:                                  ; preds = %idx.merge256
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload302)
  br label %sout.merge309

sout.merge309:                                    ; preds = %sout.default308, %sout.null307, %sout.str306, %sout.bool.merge317, %sout.float304, %sout.int303
  %39 = call i32 @putchar(i32 10)
  %rcc.val.heap318 = call ptr @malloc(i64 16)
  %tag.ptr319 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap318, i32 0, i32 0
  store i64 0, ptr %tag.ptr319, align 4
  %payload.ptr320 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap318, i32 0, i32 1
  store ptr null, ptr %payload.ptr320, align 8
  %dic2.load321 = load ptr, ptr %dic2, align 8
  %rcc.val.heap322 = call ptr @malloc(i64 16)
  %tag.ptr323 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap322, i32 0, i32 0
  store i64 4, ptr %tag.ptr323, align 4
  %payload.ptr324 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap322, i32 0, i32 1
  store ptr @.str.50, ptr %payload.ptr324, align 8
  %payload.ptr325 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap322, i32 0, i32 1
  %payload326 = load ptr, ptr %payload.ptr325, align 8
  %index.int327 = ptrtoint ptr %payload326 to i64
  %payload.ptr328 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load321, i32 0, i32 1
  %payload329 = load ptr, ptr %payload.ptr328, align 8
  %tag.ptr330 = getelementptr inbounds nuw %RCCValue, ptr %dic2.load321, i32 0, i32 0
  %tag331 = load i64, ptr %tag.ptr330, align 4
  %idx.result332 = alloca ptr, align 8
  %rcc.val.heap333 = call ptr @malloc(i64 16)
  %tag.ptr334 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap333, i32 0, i32 0
  store i64 0, ptr %tag.ptr334, align 4
  %payload.ptr335 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap333, i32 0, i32 1
  store ptr null, ptr %payload.ptr335, align 8
  store ptr %rcc.val.heap333, ptr %idx.result332, align 8
  switch i64 %tag331, label %idx.default338 [
    i64 5, label %idx.list336
    i64 6, label %idx.dict337
  ]

sout.true315:                                     ; preds = %sout.bool305
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge317

sout.false316:                                    ; preds = %sout.bool305
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge317

sout.bool.merge317:                               ; preds = %sout.false316, %sout.true315
  br label %sout.merge309

idx.list336:                                      ; preds = %sout.merge309
  %list.data340 = getelementptr inbounds nuw %RCCList.8, ptr %payload329, i32 0, i32 2
  %data341 = load ptr, ptr %list.data340, align 8
  %idx.byte.offset342 = mul i64 %index.int327, 8
  %elem.ptr343 = getelementptr inbounds i8, ptr %data341, i64 %idx.byte.offset342
  %elem344 = load ptr, ptr %elem.ptr343, align 8
  store ptr %elem344, ptr %idx.result332, align 8
  br label %idx.merge339

idx.dict337:                                      ; preds = %sout.merge309
  %dict.size345 = getelementptr inbounds nuw %RCCDict.9, ptr %payload329, i32 0, i32 1
  %size346 = load i64, ptr %dict.size345, align 4
  %dict.keys347 = getelementptr inbounds nuw %RCCDict.9, ptr %payload329, i32 0, i32 2
  %keys348 = load ptr, ptr %dict.keys347, align 8
  %dict.values349 = getelementptr inbounds nuw %RCCDict.9, ptr %payload329, i32 0, i32 3
  %vals350 = load ptr, ptr %dict.values349, align 8
  %dict.i356 = alloca i64, align 8
  store i64 0, ptr %dict.i356, align 4
  br label %dict.search.header351

idx.default338:                                   ; preds = %sout.merge309
  br label %idx.merge339

idx.merge339:                                     ; preds = %idx.default338, %dict.search.merge355, %idx.list336
  %idx.result381 = load ptr, ptr %idx.result332, align 8
  %tag.ptr382 = getelementptr inbounds nuw %RCCValue, ptr %idx.result381, i32 0, i32 0
  %tag383 = load i64, ptr %tag.ptr382, align 4
  %payload.ptr384 = getelementptr inbounds nuw %RCCValue, ptr %idx.result381, i32 0, i32 1
  %payload385 = load ptr, ptr %payload.ptr384, align 8
  switch i64 %tag383, label %sout.default391 [
    i64 1, label %sout.int386
    i64 2, label %sout.float387
    i64 3, label %sout.bool388
    i64 4, label %sout.str389
    i64 0, label %sout.null390
  ]

dict.search.header351:                            ; preds = %dict.search.body352, %idx.dict337
  %i357 = load i64, ptr %dict.i356, align 4
  %in.bounds358 = icmp slt i64 %i357, %size346
  br i1 %in.bounds358, label %dict.search.body352, label %dict.notfound354

dict.search.body352:                              ; preds = %dict.search.header351
  %i.body359 = load i64, ptr %dict.i356, align 4
  %key.byte.offset360 = mul i64 %i.body359, 8
  %key.gep361 = getelementptr inbounds i8, ptr %keys348, i64 %key.byte.offset360
  %key.val362 = load ptr, ptr %key.gep361, align 8
  %tag.ptr363 = getelementptr inbounds nuw %RCCValue, ptr %key.val362, i32 0, i32 0
  %tag364 = load i64, ptr %tag.ptr363, align 4
  %tag.ptr365 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap322, i32 0, i32 0
  %tag366 = load i64, ptr %tag.ptr365, align 4
  %tag.eq367 = icmp eq i64 %tag364, %tag366
  %payload.ptr368 = getelementptr inbounds nuw %RCCValue, ptr %key.val362, i32 0, i32 1
  %payload369 = load ptr, ptr %payload.ptr368, align 8
  %is.string370 = icmp eq i64 %tag364, 4
  %strcmp.result371 = call i32 @strcmp(ptr %payload369, ptr %payload326)
  %strcmp.eq372 = icmp eq i32 %strcmp.result371, 0
  %ptr.eq373 = icmp eq ptr %payload369, %payload326
  %payload.eq374 = select i1 %is.string370, i1 %strcmp.eq372, i1 %ptr.eq373
  %key.eq375 = and i1 %tag.eq367, %payload.eq374
  %next.i376 = add i64 %i.body359, 1
  store i64 %next.i376, ptr %dict.i356, align 4
  br i1 %key.eq375, label %dict.found353, label %dict.search.header351

dict.found353:                                    ; preds = %dict.search.body352
  %i.found377 = load i64, ptr %dict.i356, align 4
  %val.byte.offset378 = mul i64 %i.found377, 8
  %val.gep379 = getelementptr inbounds i8, ptr %vals350, i64 %val.byte.offset378
  %val380 = load ptr, ptr %val.gep379, align 8
  store ptr %val380, ptr %idx.result332, align 8
  br label %dict.search.merge355

dict.notfound354:                                 ; preds = %dict.search.header351
  br label %dict.search.merge355

dict.search.merge355:                             ; preds = %dict.notfound354, %dict.found353
  br label %idx.merge339

sout.int386:                                      ; preds = %idx.merge339
  %int.val393 = ptrtoint ptr %payload385 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.51, i64 %int.val393)
  br label %sout.merge392

sout.float387:                                    ; preds = %idx.merge339
  %float.bits394 = ptrtoint ptr %payload385 to i64
  %float.val395 = bitcast i64 %float.bits394 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.52, double %float.val395)
  br label %sout.merge392

sout.bool388:                                     ; preds = %idx.merge339
  %bool.val396 = ptrtoint ptr %payload385 to i64
  %is.true397 = icmp ne i64 %bool.val396, 0
  br i1 %is.true397, label %sout.true398, label %sout.false399

sout.str389:                                      ; preds = %idx.merge339
  %44 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr %payload385)
  br label %sout.merge392

sout.null390:                                     ; preds = %idx.merge339
  %45 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.merge392

sout.default391:                                  ; preds = %idx.merge339
  %46 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload385)
  br label %sout.merge392

sout.merge392:                                    ; preds = %sout.default391, %sout.null390, %sout.str389, %sout.bool.merge400, %sout.float387, %sout.int386
  %47 = call i32 @putchar(i32 10)
  %rcc.val.heap401 = call ptr @malloc(i64 16)
  %tag.ptr402 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap401, i32 0, i32 0
  store i64 0, ptr %tag.ptr402, align 4
  %payload.ptr403 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap401, i32 0, i32 1
  store ptr null, ptr %payload.ptr403, align 8
  ret i32 0

sout.true398:                                     ; preds = %sout.bool388
  %48 = call i32 (ptr, ...) @printf(ptr @.str.53, ptr @.str.54)
  br label %sout.bool.merge400

sout.false399:                                    ; preds = %sout.bool388
  %49 = call i32 (ptr, ...) @printf(ptr @.str.55, ptr @.str.56)
  br label %sout.bool.merge400

sout.bool.merge400:                               ; preds = %sout.false399, %sout.true398
  br label %sout.merge392
}

declare ptr @malloc(i64)

declare i32 @strcmp(ptr, ptr)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)
