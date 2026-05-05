; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }
%RCCList = type { i64, i64, ptr }
%RCCList.0 = type { i64, i64, ptr }
%RCCDict = type { i64, i64, ptr, ptr }
%RCCList.1 = type { i64, i64, ptr }
%RCCDict.2 = type { i64, i64, ptr, ptr }
%RCCList.3 = type { i64, i64, ptr }
%RCCDict.4 = type { i64, i64, ptr, ptr }

@.str = private constant [5 x i8] c"%lld\00"
@.str.1 = private constant [3 x i8] c"%g\00"
@.str.2 = private constant [3 x i8] c"%s\00"
@.str.3 = private constant [5 x i8] c"true\00"
@.str.4 = private constant [3 x i8] c"%s\00"
@.str.5 = private constant [6 x i8] c"false\00"
@.str.6 = private constant [3 x i8] c"%s\00"
@.str.7 = private constant [3 x i8] c"%s\00"
@.str.8 = private constant [5 x i8] c"null\00"
@.str.9 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.10 = private constant [5 x i8] c"%lld\00"
@.str.11 = private constant [3 x i8] c"%g\00"
@.str.12 = private constant [3 x i8] c"%s\00"
@.str.13 = private constant [5 x i8] c"true\00"
@.str.14 = private constant [3 x i8] c"%s\00"
@.str.15 = private constant [6 x i8] c"false\00"
@.str.16 = private constant [3 x i8] c"%s\00"
@.str.17 = private constant [3 x i8] c"%s\00"
@.str.18 = private constant [5 x i8] c"null\00"
@.str.19 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.20 = private constant [5 x i8] c"%lld\00"
@.str.21 = private constant [3 x i8] c"%g\00"
@.str.22 = private constant [3 x i8] c"%s\00"
@.str.23 = private constant [5 x i8] c"true\00"
@.str.24 = private constant [3 x i8] c"%s\00"
@.str.25 = private constant [6 x i8] c"false\00"
@.str.26 = private constant [3 x i8] c"%s\00"
@.str.27 = private constant [3 x i8] c"%s\00"
@.str.28 = private constant [5 x i8] c"null\00"
@.str.29 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %arr = alloca ptr, align 8
  store ptr null, ptr %arr, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr, align 8
  %rcc.val1 = alloca %RCCValue, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr3, align 8
  %rcc.val4 = alloca %RCCValue, align 8
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val4, i32 0, i32 0
  store i64 1, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val4, i32 0, i32 1
  store ptr inttoptr (i64 30 to ptr), ptr %payload.ptr6, align 8
  %list.data = call ptr @malloc(i64 24)
  %elem.gep = getelementptr inbounds i8, ptr %list.data, i64 0
  store ptr %rcc.val, ptr %elem.gep, align 8
  %elem.gep7 = getelementptr inbounds i8, ptr %list.data, i64 1
  store ptr %rcc.val1, ptr %elem.gep7, align 8
  %elem.gep8 = getelementptr inbounds i8, ptr %list.data, i64 2
  store ptr %rcc.val4, ptr %elem.gep8, align 8
  %list = alloca %RCCList, align 8
  %2 = getelementptr inbounds nuw %RCCList, ptr %list, i32 0, i32 0
  store i64 3, ptr %2, align 4
  %3 = getelementptr inbounds nuw %RCCList, ptr %list, i32 0, i32 1
  store i64 3, ptr %3, align 4
  %4 = getelementptr inbounds nuw %RCCList, ptr %list, i32 0, i32 2
  store ptr %list.data, ptr %4, align 8
  %rcc.val9 = alloca %RCCValue, align 8
  %tag.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 0
  store i64 5, ptr %tag.ptr10, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 1
  store ptr %list, ptr %payload.ptr11, align 8
  store ptr %rcc.val9, ptr %arr, align 8
  %arr.load = load ptr, ptr %arr, align 8
  %rcc.val12 = alloca %RCCValue, align 8
  %tag.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val12, i32 0, i32 0
  store i64 1, ptr %tag.ptr13, align 4
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val12, i32 0, i32 1
  store ptr null, ptr %payload.ptr14, align 8
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val12, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr15, align 8
  %index.int = ptrtoint ptr %payload to i64
  %payload.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %arr.load, i32 0, i32 1
  %payload17 = load ptr, ptr %payload.ptr16, align 8
  %tag.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %arr.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr18, align 4
  %idx.result = alloca ptr, align 8
  %rcc.val19 = alloca %RCCValue, align 8
  %tag.ptr20 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val19, i32 0, i32 0
  store i64 0, ptr %tag.ptr20, align 4
  %payload.ptr21 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val19, i32 0, i32 1
  store ptr null, ptr %payload.ptr21, align 8
  store ptr %rcc.val19, ptr %idx.result, align 8
  switch i64 %tag, label %idx.default [
    i64 5, label %idx.list
    i64 6, label %idx.dict
  ]

idx.list:                                         ; preds = %entry
  %list.data22 = getelementptr inbounds nuw %RCCList.0, ptr %payload17, i32 0, i32 2
  %data = load ptr, ptr %list.data22, align 8
  %elem.ptr = getelementptr inbounds i8, ptr %data, i64 %index.int
  %elem = load ptr, ptr %elem.ptr, align 8
  store ptr %elem, ptr %idx.result, align 8
  br label %idx.merge

idx.dict:                                         ; preds = %entry
  %dict.size = getelementptr inbounds nuw %RCCDict, ptr %payload17, i32 0, i32 1
  %size = load i64, ptr %dict.size, align 4
  %dict.keys = getelementptr inbounds nuw %RCCDict, ptr %payload17, i32 0, i32 2
  %keys = load ptr, ptr %dict.keys, align 8
  %dict.values = getelementptr inbounds nuw %RCCDict, ptr %payload17, i32 0, i32 3
  %vals = load ptr, ptr %dict.values, align 8
  %dict.i = alloca i64, align 8
  store i64 0, ptr %dict.i, align 4
  br label %dict.search.header

idx.default:                                      ; preds = %entry
  br label %idx.merge

idx.merge:                                        ; preds = %idx.default, %dict.search.merge, %idx.list
  %idx.result29 = load ptr, ptr %idx.result, align 8
  %tag.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %idx.result29, i32 0, i32 0
  %tag31 = load i64, ptr %tag.ptr30, align 4
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %idx.result29, i32 0, i32 1
  %payload33 = load ptr, ptr %payload.ptr32, align 8
  switch i64 %tag31, label %sout.default [
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
  %key.gep = getelementptr inbounds i8, ptr %keys, i64 %i.body
  %key.val = load ptr, ptr %key.gep, align 8
  %tag.ptr23 = getelementptr inbounds nuw %RCCValue, ptr %key.val, i32 0, i32 0
  %tag24 = load i64, ptr %tag.ptr23, align 4
  %tag.ptr25 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val12, i32 0, i32 0
  %tag26 = load i64, ptr %tag.ptr25, align 4
  %tag.eq = icmp eq i64 %tag24, %tag26
  %payload.ptr27 = getelementptr inbounds nuw %RCCValue, ptr %key.val, i32 0, i32 1
  %payload28 = load ptr, ptr %payload.ptr27, align 8
  %payload.eq = icmp eq ptr %payload28, %payload
  %key.eq = and i1 %tag.eq, %payload.eq
  %next.i = add i64 %i.body, 1
  store i64 %next.i, ptr %dict.i, align 4
  br i1 %key.eq, label %dict.found, label %dict.search.header

dict.found:                                       ; preds = %dict.search.body
  %i.found = load i64, ptr %dict.i, align 4
  %val.gep = getelementptr inbounds i8, ptr %vals, i64 %i.found
  %val = load ptr, ptr %val.gep, align 8
  store ptr %val, ptr %idx.result, align 8
  br label %dict.search.merge

dict.notfound:                                    ; preds = %dict.search.header
  br label %dict.search.merge

dict.search.merge:                                ; preds = %dict.notfound, %dict.found
  br label %idx.merge

sout.int:                                         ; preds = %idx.merge
  %int.val = ptrtoint ptr %payload33 to i64
  %5 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %idx.merge
  %float.bits = ptrtoint ptr %payload33 to i64
  %float.val = bitcast i64 %float.bits to double
  %6 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %idx.merge
  %bool.val = ptrtoint ptr %payload33 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %idx.merge
  %7 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload33)
  br label %sout.merge

sout.null:                                        ; preds = %idx.merge
  %8 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %idx.merge
  %9 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload33)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %10 = call i32 @putchar(i32 10)
  %rcc.val34 = alloca %RCCValue, align 8
  %tag.ptr35 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val34, i32 0, i32 0
  store i64 0, ptr %tag.ptr35, align 4
  %payload.ptr36 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val34, i32 0, i32 1
  store ptr null, ptr %payload.ptr36, align 8
  %arr.load37 = load ptr, ptr %arr, align 8
  %rcc.val38 = alloca %RCCValue, align 8
  %tag.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 0
  store i64 1, ptr %tag.ptr39, align 4
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr40, align 8
  %payload.ptr41 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 1
  %payload42 = load ptr, ptr %payload.ptr41, align 8
  %index.int43 = ptrtoint ptr %payload42 to i64
  %payload.ptr44 = getelementptr inbounds nuw %RCCValue, ptr %arr.load37, i32 0, i32 1
  %payload45 = load ptr, ptr %payload.ptr44, align 8
  %tag.ptr46 = getelementptr inbounds nuw %RCCValue, ptr %arr.load37, i32 0, i32 0
  %tag47 = load i64, ptr %tag.ptr46, align 4
  %idx.result48 = alloca ptr, align 8
  %rcc.val49 = alloca %RCCValue, align 8
  %tag.ptr50 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val49, i32 0, i32 0
  store i64 0, ptr %tag.ptr50, align 4
  %payload.ptr51 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val49, i32 0, i32 1
  store ptr null, ptr %payload.ptr51, align 8
  store ptr %rcc.val49, ptr %idx.result48, align 8
  switch i64 %tag47, label %idx.default54 [
    i64 5, label %idx.list52
    i64 6, label %idx.dict53
  ]

sout.true:                                        ; preds = %sout.bool
  %11 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %12 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

idx.list52:                                       ; preds = %sout.merge
  %list.data56 = getelementptr inbounds nuw %RCCList.1, ptr %payload45, i32 0, i32 2
  %data57 = load ptr, ptr %list.data56, align 8
  %elem.ptr58 = getelementptr inbounds i8, ptr %data57, i64 %index.int43
  %elem59 = load ptr, ptr %elem.ptr58, align 8
  store ptr %elem59, ptr %idx.result48, align 8
  br label %idx.merge55

idx.dict53:                                       ; preds = %sout.merge
  %dict.size60 = getelementptr inbounds nuw %RCCDict.2, ptr %payload45, i32 0, i32 1
  %size61 = load i64, ptr %dict.size60, align 4
  %dict.keys62 = getelementptr inbounds nuw %RCCDict.2, ptr %payload45, i32 0, i32 2
  %keys63 = load ptr, ptr %dict.keys62, align 8
  %dict.values64 = getelementptr inbounds nuw %RCCDict.2, ptr %payload45, i32 0, i32 3
  %vals65 = load ptr, ptr %dict.values64, align 8
  %dict.i71 = alloca i64, align 8
  store i64 0, ptr %dict.i71, align 4
  br label %dict.search.header66

idx.default54:                                    ; preds = %sout.merge
  br label %idx.merge55

idx.merge55:                                      ; preds = %idx.default54, %dict.search.merge70, %idx.list52
  %idx.result90 = load ptr, ptr %idx.result48, align 8
  %tag.ptr91 = getelementptr inbounds nuw %RCCValue, ptr %idx.result90, i32 0, i32 0
  %tag92 = load i64, ptr %tag.ptr91, align 4
  %payload.ptr93 = getelementptr inbounds nuw %RCCValue, ptr %idx.result90, i32 0, i32 1
  %payload94 = load ptr, ptr %payload.ptr93, align 8
  switch i64 %tag92, label %sout.default100 [
    i64 1, label %sout.int95
    i64 2, label %sout.float96
    i64 3, label %sout.bool97
    i64 4, label %sout.str98
    i64 0, label %sout.null99
  ]

dict.search.header66:                             ; preds = %dict.search.body67, %idx.dict53
  %i72 = load i64, ptr %dict.i71, align 4
  %in.bounds73 = icmp slt i64 %i72, %size61
  br i1 %in.bounds73, label %dict.search.body67, label %dict.notfound69

dict.search.body67:                               ; preds = %dict.search.header66
  %i.body74 = load i64, ptr %dict.i71, align 4
  %key.gep75 = getelementptr inbounds i8, ptr %keys63, i64 %i.body74
  %key.val76 = load ptr, ptr %key.gep75, align 8
  %tag.ptr77 = getelementptr inbounds nuw %RCCValue, ptr %key.val76, i32 0, i32 0
  %tag78 = load i64, ptr %tag.ptr77, align 4
  %tag.ptr79 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 0
  %tag80 = load i64, ptr %tag.ptr79, align 4
  %tag.eq81 = icmp eq i64 %tag78, %tag80
  %payload.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %key.val76, i32 0, i32 1
  %payload83 = load ptr, ptr %payload.ptr82, align 8
  %payload.eq84 = icmp eq ptr %payload83, %payload42
  %key.eq85 = and i1 %tag.eq81, %payload.eq84
  %next.i86 = add i64 %i.body74, 1
  store i64 %next.i86, ptr %dict.i71, align 4
  br i1 %key.eq85, label %dict.found68, label %dict.search.header66

dict.found68:                                     ; preds = %dict.search.body67
  %i.found87 = load i64, ptr %dict.i71, align 4
  %val.gep88 = getelementptr inbounds i8, ptr %vals65, i64 %i.found87
  %val89 = load ptr, ptr %val.gep88, align 8
  store ptr %val89, ptr %idx.result48, align 8
  br label %dict.search.merge70

dict.notfound69:                                  ; preds = %dict.search.header66
  br label %dict.search.merge70

dict.search.merge70:                              ; preds = %dict.notfound69, %dict.found68
  br label %idx.merge55

sout.int95:                                       ; preds = %idx.merge55
  %int.val102 = ptrtoint ptr %payload94 to i64
  %13 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val102)
  br label %sout.merge101

sout.float96:                                     ; preds = %idx.merge55
  %float.bits103 = ptrtoint ptr %payload94 to i64
  %float.val104 = bitcast i64 %float.bits103 to double
  %14 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val104)
  br label %sout.merge101

sout.bool97:                                      ; preds = %idx.merge55
  %bool.val105 = ptrtoint ptr %payload94 to i64
  %is.true106 = icmp ne i64 %bool.val105, 0
  br i1 %is.true106, label %sout.true107, label %sout.false108

sout.str98:                                       ; preds = %idx.merge55
  %15 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload94)
  br label %sout.merge101

sout.null99:                                      ; preds = %idx.merge55
  %16 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge101

sout.default100:                                  ; preds = %idx.merge55
  %17 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload94)
  br label %sout.merge101

sout.merge101:                                    ; preds = %sout.default100, %sout.null99, %sout.str98, %sout.bool.merge109, %sout.float96, %sout.int95
  %18 = call i32 @putchar(i32 10)
  %rcc.val110 = alloca %RCCValue, align 8
  %tag.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val110, i32 0, i32 0
  store i64 0, ptr %tag.ptr111, align 4
  %payload.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val110, i32 0, i32 1
  store ptr null, ptr %payload.ptr112, align 8
  %arr.load113 = load ptr, ptr %arr, align 8
  %rcc.val114 = alloca %RCCValue, align 8
  %tag.ptr115 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val114, i32 0, i32 0
  store i64 1, ptr %tag.ptr115, align 4
  %payload.ptr116 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val114, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr116, align 8
  %payload.ptr117 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val114, i32 0, i32 1
  %payload118 = load ptr, ptr %payload.ptr117, align 8
  %index.int119 = ptrtoint ptr %payload118 to i64
  %payload.ptr120 = getelementptr inbounds nuw %RCCValue, ptr %arr.load113, i32 0, i32 1
  %payload121 = load ptr, ptr %payload.ptr120, align 8
  %tag.ptr122 = getelementptr inbounds nuw %RCCValue, ptr %arr.load113, i32 0, i32 0
  %tag123 = load i64, ptr %tag.ptr122, align 4
  %idx.result124 = alloca ptr, align 8
  %rcc.val125 = alloca %RCCValue, align 8
  %tag.ptr126 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val125, i32 0, i32 0
  store i64 0, ptr %tag.ptr126, align 4
  %payload.ptr127 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val125, i32 0, i32 1
  store ptr null, ptr %payload.ptr127, align 8
  store ptr %rcc.val125, ptr %idx.result124, align 8
  switch i64 %tag123, label %idx.default130 [
    i64 5, label %idx.list128
    i64 6, label %idx.dict129
  ]

sout.true107:                                     ; preds = %sout.bool97
  %19 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge109

sout.false108:                                    ; preds = %sout.bool97
  %20 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge109

sout.bool.merge109:                               ; preds = %sout.false108, %sout.true107
  br label %sout.merge101

idx.list128:                                      ; preds = %sout.merge101
  %list.data132 = getelementptr inbounds nuw %RCCList.3, ptr %payload121, i32 0, i32 2
  %data133 = load ptr, ptr %list.data132, align 8
  %elem.ptr134 = getelementptr inbounds i8, ptr %data133, i64 %index.int119
  %elem135 = load ptr, ptr %elem.ptr134, align 8
  store ptr %elem135, ptr %idx.result124, align 8
  br label %idx.merge131

idx.dict129:                                      ; preds = %sout.merge101
  %dict.size136 = getelementptr inbounds nuw %RCCDict.4, ptr %payload121, i32 0, i32 1
  %size137 = load i64, ptr %dict.size136, align 4
  %dict.keys138 = getelementptr inbounds nuw %RCCDict.4, ptr %payload121, i32 0, i32 2
  %keys139 = load ptr, ptr %dict.keys138, align 8
  %dict.values140 = getelementptr inbounds nuw %RCCDict.4, ptr %payload121, i32 0, i32 3
  %vals141 = load ptr, ptr %dict.values140, align 8
  %dict.i147 = alloca i64, align 8
  store i64 0, ptr %dict.i147, align 4
  br label %dict.search.header142

idx.default130:                                   ; preds = %sout.merge101
  br label %idx.merge131

idx.merge131:                                     ; preds = %idx.default130, %dict.search.merge146, %idx.list128
  %idx.result166 = load ptr, ptr %idx.result124, align 8
  %tag.ptr167 = getelementptr inbounds nuw %RCCValue, ptr %idx.result166, i32 0, i32 0
  %tag168 = load i64, ptr %tag.ptr167, align 4
  %payload.ptr169 = getelementptr inbounds nuw %RCCValue, ptr %idx.result166, i32 0, i32 1
  %payload170 = load ptr, ptr %payload.ptr169, align 8
  switch i64 %tag168, label %sout.default176 [
    i64 1, label %sout.int171
    i64 2, label %sout.float172
    i64 3, label %sout.bool173
    i64 4, label %sout.str174
    i64 0, label %sout.null175
  ]

dict.search.header142:                            ; preds = %dict.search.body143, %idx.dict129
  %i148 = load i64, ptr %dict.i147, align 4
  %in.bounds149 = icmp slt i64 %i148, %size137
  br i1 %in.bounds149, label %dict.search.body143, label %dict.notfound145

dict.search.body143:                              ; preds = %dict.search.header142
  %i.body150 = load i64, ptr %dict.i147, align 4
  %key.gep151 = getelementptr inbounds i8, ptr %keys139, i64 %i.body150
  %key.val152 = load ptr, ptr %key.gep151, align 8
  %tag.ptr153 = getelementptr inbounds nuw %RCCValue, ptr %key.val152, i32 0, i32 0
  %tag154 = load i64, ptr %tag.ptr153, align 4
  %tag.ptr155 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val114, i32 0, i32 0
  %tag156 = load i64, ptr %tag.ptr155, align 4
  %tag.eq157 = icmp eq i64 %tag154, %tag156
  %payload.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %key.val152, i32 0, i32 1
  %payload159 = load ptr, ptr %payload.ptr158, align 8
  %payload.eq160 = icmp eq ptr %payload159, %payload118
  %key.eq161 = and i1 %tag.eq157, %payload.eq160
  %next.i162 = add i64 %i.body150, 1
  store i64 %next.i162, ptr %dict.i147, align 4
  br i1 %key.eq161, label %dict.found144, label %dict.search.header142

dict.found144:                                    ; preds = %dict.search.body143
  %i.found163 = load i64, ptr %dict.i147, align 4
  %val.gep164 = getelementptr inbounds i8, ptr %vals141, i64 %i.found163
  %val165 = load ptr, ptr %val.gep164, align 8
  store ptr %val165, ptr %idx.result124, align 8
  br label %dict.search.merge146

dict.notfound145:                                 ; preds = %dict.search.header142
  br label %dict.search.merge146

dict.search.merge146:                             ; preds = %dict.notfound145, %dict.found144
  br label %idx.merge131

sout.int171:                                      ; preds = %idx.merge131
  %int.val178 = ptrtoint ptr %payload170 to i64
  %21 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val178)
  br label %sout.merge177

sout.float172:                                    ; preds = %idx.merge131
  %float.bits179 = ptrtoint ptr %payload170 to i64
  %float.val180 = bitcast i64 %float.bits179 to double
  %22 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val180)
  br label %sout.merge177

sout.bool173:                                     ; preds = %idx.merge131
  %bool.val181 = ptrtoint ptr %payload170 to i64
  %is.true182 = icmp ne i64 %bool.val181, 0
  br i1 %is.true182, label %sout.true183, label %sout.false184

sout.str174:                                      ; preds = %idx.merge131
  %23 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload170)
  br label %sout.merge177

sout.null175:                                     ; preds = %idx.merge131
  %24 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge177

sout.default176:                                  ; preds = %idx.merge131
  %25 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload170)
  br label %sout.merge177

sout.merge177:                                    ; preds = %sout.default176, %sout.null175, %sout.str174, %sout.bool.merge185, %sout.float172, %sout.int171
  %26 = call i32 @putchar(i32 10)
  %rcc.val186 = alloca %RCCValue, align 8
  %tag.ptr187 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val186, i32 0, i32 0
  store i64 0, ptr %tag.ptr187, align 4
  %payload.ptr188 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val186, i32 0, i32 1
  store ptr null, ptr %payload.ptr188, align 8
  ret i32 0

sout.true183:                                     ; preds = %sout.bool173
  %27 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge185

sout.false184:                                    ; preds = %sout.bool173
  %28 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge185

sout.bool.merge185:                               ; preds = %sout.false184, %sout.true183
  br label %sout.merge177
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
