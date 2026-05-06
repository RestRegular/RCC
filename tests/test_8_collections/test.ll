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
  %elem.gep7 = getelementptr inbounds i8, ptr %list.data, i64 8
  store ptr %rcc.val1, ptr %elem.gep7, align 8
  %elem.gep8 = getelementptr inbounds i8, ptr %list.data, i64 16
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
  %idx.byte.offset = mul i64 %index.int, 8
  %elem.ptr = getelementptr inbounds i8, ptr %data, i64 %idx.byte.offset
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
  %key.byte.offset = mul i64 %i.body, 8
  %key.gep = getelementptr inbounds i8, ptr %keys, i64 %key.byte.offset
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
  %val.byte.offset = mul i64 %i.found, 8
  %val.gep = getelementptr inbounds i8, ptr %vals, i64 %val.byte.offset
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
  %idx.byte.offset58 = mul i64 %index.int43, 8
  %elem.ptr59 = getelementptr inbounds i8, ptr %data57, i64 %idx.byte.offset58
  %elem60 = load ptr, ptr %elem.ptr59, align 8
  store ptr %elem60, ptr %idx.result48, align 8
  br label %idx.merge55

idx.dict53:                                       ; preds = %sout.merge
  %dict.size61 = getelementptr inbounds nuw %RCCDict.2, ptr %payload45, i32 0, i32 1
  %size62 = load i64, ptr %dict.size61, align 4
  %dict.keys63 = getelementptr inbounds nuw %RCCDict.2, ptr %payload45, i32 0, i32 2
  %keys64 = load ptr, ptr %dict.keys63, align 8
  %dict.values65 = getelementptr inbounds nuw %RCCDict.2, ptr %payload45, i32 0, i32 3
  %vals66 = load ptr, ptr %dict.values65, align 8
  %dict.i72 = alloca i64, align 8
  store i64 0, ptr %dict.i72, align 4
  br label %dict.search.header67

idx.default54:                                    ; preds = %sout.merge
  br label %idx.merge55

idx.merge55:                                      ; preds = %idx.default54, %dict.search.merge71, %idx.list52
  %idx.result93 = load ptr, ptr %idx.result48, align 8
  %tag.ptr94 = getelementptr inbounds nuw %RCCValue, ptr %idx.result93, i32 0, i32 0
  %tag95 = load i64, ptr %tag.ptr94, align 4
  %payload.ptr96 = getelementptr inbounds nuw %RCCValue, ptr %idx.result93, i32 0, i32 1
  %payload97 = load ptr, ptr %payload.ptr96, align 8
  switch i64 %tag95, label %sout.default103 [
    i64 1, label %sout.int98
    i64 2, label %sout.float99
    i64 3, label %sout.bool100
    i64 4, label %sout.str101
    i64 0, label %sout.null102
  ]

dict.search.header67:                             ; preds = %dict.search.body68, %idx.dict53
  %i73 = load i64, ptr %dict.i72, align 4
  %in.bounds74 = icmp slt i64 %i73, %size62
  br i1 %in.bounds74, label %dict.search.body68, label %dict.notfound70

dict.search.body68:                               ; preds = %dict.search.header67
  %i.body75 = load i64, ptr %dict.i72, align 4
  %key.byte.offset76 = mul i64 %i.body75, 8
  %key.gep77 = getelementptr inbounds i8, ptr %keys64, i64 %key.byte.offset76
  %key.val78 = load ptr, ptr %key.gep77, align 8
  %tag.ptr79 = getelementptr inbounds nuw %RCCValue, ptr %key.val78, i32 0, i32 0
  %tag80 = load i64, ptr %tag.ptr79, align 4
  %tag.ptr81 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 0
  %tag82 = load i64, ptr %tag.ptr81, align 4
  %tag.eq83 = icmp eq i64 %tag80, %tag82
  %payload.ptr84 = getelementptr inbounds nuw %RCCValue, ptr %key.val78, i32 0, i32 1
  %payload85 = load ptr, ptr %payload.ptr84, align 8
  %payload.eq86 = icmp eq ptr %payload85, %payload42
  %key.eq87 = and i1 %tag.eq83, %payload.eq86
  %next.i88 = add i64 %i.body75, 1
  store i64 %next.i88, ptr %dict.i72, align 4
  br i1 %key.eq87, label %dict.found69, label %dict.search.header67

dict.found69:                                     ; preds = %dict.search.body68
  %i.found89 = load i64, ptr %dict.i72, align 4
  %val.byte.offset90 = mul i64 %i.found89, 8
  %val.gep91 = getelementptr inbounds i8, ptr %vals66, i64 %val.byte.offset90
  %val92 = load ptr, ptr %val.gep91, align 8
  store ptr %val92, ptr %idx.result48, align 8
  br label %dict.search.merge71

dict.notfound70:                                  ; preds = %dict.search.header67
  br label %dict.search.merge71

dict.search.merge71:                              ; preds = %dict.notfound70, %dict.found69
  br label %idx.merge55

sout.int98:                                       ; preds = %idx.merge55
  %int.val105 = ptrtoint ptr %payload97 to i64
  %13 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val105)
  br label %sout.merge104

sout.float99:                                     ; preds = %idx.merge55
  %float.bits106 = ptrtoint ptr %payload97 to i64
  %float.val107 = bitcast i64 %float.bits106 to double
  %14 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val107)
  br label %sout.merge104

sout.bool100:                                     ; preds = %idx.merge55
  %bool.val108 = ptrtoint ptr %payload97 to i64
  %is.true109 = icmp ne i64 %bool.val108, 0
  br i1 %is.true109, label %sout.true110, label %sout.false111

sout.str101:                                      ; preds = %idx.merge55
  %15 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload97)
  br label %sout.merge104

sout.null102:                                     ; preds = %idx.merge55
  %16 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge104

sout.default103:                                  ; preds = %idx.merge55
  %17 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload97)
  br label %sout.merge104

sout.merge104:                                    ; preds = %sout.default103, %sout.null102, %sout.str101, %sout.bool.merge112, %sout.float99, %sout.int98
  %18 = call i32 @putchar(i32 10)
  %rcc.val113 = alloca %RCCValue, align 8
  %tag.ptr114 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val113, i32 0, i32 0
  store i64 0, ptr %tag.ptr114, align 4
  %payload.ptr115 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val113, i32 0, i32 1
  store ptr null, ptr %payload.ptr115, align 8
  %arr.load116 = load ptr, ptr %arr, align 8
  %rcc.val117 = alloca %RCCValue, align 8
  %tag.ptr118 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val117, i32 0, i32 0
  store i64 1, ptr %tag.ptr118, align 4
  %payload.ptr119 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val117, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr119, align 8
  %payload.ptr120 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val117, i32 0, i32 1
  %payload121 = load ptr, ptr %payload.ptr120, align 8
  %index.int122 = ptrtoint ptr %payload121 to i64
  %payload.ptr123 = getelementptr inbounds nuw %RCCValue, ptr %arr.load116, i32 0, i32 1
  %payload124 = load ptr, ptr %payload.ptr123, align 8
  %tag.ptr125 = getelementptr inbounds nuw %RCCValue, ptr %arr.load116, i32 0, i32 0
  %tag126 = load i64, ptr %tag.ptr125, align 4
  %idx.result127 = alloca ptr, align 8
  %rcc.val128 = alloca %RCCValue, align 8
  %tag.ptr129 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val128, i32 0, i32 0
  store i64 0, ptr %tag.ptr129, align 4
  %payload.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val128, i32 0, i32 1
  store ptr null, ptr %payload.ptr130, align 8
  store ptr %rcc.val128, ptr %idx.result127, align 8
  switch i64 %tag126, label %idx.default133 [
    i64 5, label %idx.list131
    i64 6, label %idx.dict132
  ]

sout.true110:                                     ; preds = %sout.bool100
  %19 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge112

sout.false111:                                    ; preds = %sout.bool100
  %20 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge112

sout.bool.merge112:                               ; preds = %sout.false111, %sout.true110
  br label %sout.merge104

idx.list131:                                      ; preds = %sout.merge104
  %list.data135 = getelementptr inbounds nuw %RCCList.3, ptr %payload124, i32 0, i32 2
  %data136 = load ptr, ptr %list.data135, align 8
  %idx.byte.offset137 = mul i64 %index.int122, 8
  %elem.ptr138 = getelementptr inbounds i8, ptr %data136, i64 %idx.byte.offset137
  %elem139 = load ptr, ptr %elem.ptr138, align 8
  store ptr %elem139, ptr %idx.result127, align 8
  br label %idx.merge134

idx.dict132:                                      ; preds = %sout.merge104
  %dict.size140 = getelementptr inbounds nuw %RCCDict.4, ptr %payload124, i32 0, i32 1
  %size141 = load i64, ptr %dict.size140, align 4
  %dict.keys142 = getelementptr inbounds nuw %RCCDict.4, ptr %payload124, i32 0, i32 2
  %keys143 = load ptr, ptr %dict.keys142, align 8
  %dict.values144 = getelementptr inbounds nuw %RCCDict.4, ptr %payload124, i32 0, i32 3
  %vals145 = load ptr, ptr %dict.values144, align 8
  %dict.i151 = alloca i64, align 8
  store i64 0, ptr %dict.i151, align 4
  br label %dict.search.header146

idx.default133:                                   ; preds = %sout.merge104
  br label %idx.merge134

idx.merge134:                                     ; preds = %idx.default133, %dict.search.merge150, %idx.list131
  %idx.result172 = load ptr, ptr %idx.result127, align 8
  %tag.ptr173 = getelementptr inbounds nuw %RCCValue, ptr %idx.result172, i32 0, i32 0
  %tag174 = load i64, ptr %tag.ptr173, align 4
  %payload.ptr175 = getelementptr inbounds nuw %RCCValue, ptr %idx.result172, i32 0, i32 1
  %payload176 = load ptr, ptr %payload.ptr175, align 8
  switch i64 %tag174, label %sout.default182 [
    i64 1, label %sout.int177
    i64 2, label %sout.float178
    i64 3, label %sout.bool179
    i64 4, label %sout.str180
    i64 0, label %sout.null181
  ]

dict.search.header146:                            ; preds = %dict.search.body147, %idx.dict132
  %i152 = load i64, ptr %dict.i151, align 4
  %in.bounds153 = icmp slt i64 %i152, %size141
  br i1 %in.bounds153, label %dict.search.body147, label %dict.notfound149

dict.search.body147:                              ; preds = %dict.search.header146
  %i.body154 = load i64, ptr %dict.i151, align 4
  %key.byte.offset155 = mul i64 %i.body154, 8
  %key.gep156 = getelementptr inbounds i8, ptr %keys143, i64 %key.byte.offset155
  %key.val157 = load ptr, ptr %key.gep156, align 8
  %tag.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %key.val157, i32 0, i32 0
  %tag159 = load i64, ptr %tag.ptr158, align 4
  %tag.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val117, i32 0, i32 0
  %tag161 = load i64, ptr %tag.ptr160, align 4
  %tag.eq162 = icmp eq i64 %tag159, %tag161
  %payload.ptr163 = getelementptr inbounds nuw %RCCValue, ptr %key.val157, i32 0, i32 1
  %payload164 = load ptr, ptr %payload.ptr163, align 8
  %payload.eq165 = icmp eq ptr %payload164, %payload121
  %key.eq166 = and i1 %tag.eq162, %payload.eq165
  %next.i167 = add i64 %i.body154, 1
  store i64 %next.i167, ptr %dict.i151, align 4
  br i1 %key.eq166, label %dict.found148, label %dict.search.header146

dict.found148:                                    ; preds = %dict.search.body147
  %i.found168 = load i64, ptr %dict.i151, align 4
  %val.byte.offset169 = mul i64 %i.found168, 8
  %val.gep170 = getelementptr inbounds i8, ptr %vals145, i64 %val.byte.offset169
  %val171 = load ptr, ptr %val.gep170, align 8
  store ptr %val171, ptr %idx.result127, align 8
  br label %dict.search.merge150

dict.notfound149:                                 ; preds = %dict.search.header146
  br label %dict.search.merge150

dict.search.merge150:                             ; preds = %dict.notfound149, %dict.found148
  br label %idx.merge134

sout.int177:                                      ; preds = %idx.merge134
  %int.val184 = ptrtoint ptr %payload176 to i64
  %21 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val184)
  br label %sout.merge183

sout.float178:                                    ; preds = %idx.merge134
  %float.bits185 = ptrtoint ptr %payload176 to i64
  %float.val186 = bitcast i64 %float.bits185 to double
  %22 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val186)
  br label %sout.merge183

sout.bool179:                                     ; preds = %idx.merge134
  %bool.val187 = ptrtoint ptr %payload176 to i64
  %is.true188 = icmp ne i64 %bool.val187, 0
  br i1 %is.true188, label %sout.true189, label %sout.false190

sout.str180:                                      ; preds = %idx.merge134
  %23 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload176)
  br label %sout.merge183

sout.null181:                                     ; preds = %idx.merge134
  %24 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge183

sout.default182:                                  ; preds = %idx.merge134
  %25 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload176)
  br label %sout.merge183

sout.merge183:                                    ; preds = %sout.default182, %sout.null181, %sout.str180, %sout.bool.merge191, %sout.float178, %sout.int177
  %26 = call i32 @putchar(i32 10)
  %rcc.val192 = alloca %RCCValue, align 8
  %tag.ptr193 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val192, i32 0, i32 0
  store i64 0, ptr %tag.ptr193, align 4
  %payload.ptr194 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val192, i32 0, i32 1
  store ptr null, ptr %payload.ptr194, align 8
  ret i32 0

sout.true189:                                     ; preds = %sout.bool179
  %27 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge191

sout.false190:                                    ; preds = %sout.bool179
  %28 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge191

sout.bool.merge191:                               ; preds = %sout.false190, %sout.true189
  br label %sout.merge183
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
