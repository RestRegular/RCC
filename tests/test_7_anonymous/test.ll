; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

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
  %double = alloca ptr, align 8
  %add = alloca ptr, align 8
  store ptr null, ptr %add, align 8
  store ptr @__anon_func_0, ptr %add, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr, align 8
  %rcc.val1 = alloca %RCCValue, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr3, align 8
  %add.load = load ptr, ptr %add, align 8
  %payload.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %add.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr4, align 8
  %call.indirect.add = call ptr %payload(ptr %rcc.val, ptr %rcc.val1)
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.add, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.add, i32 0, i32 1
  %payload7 = load ptr, ptr %payload.ptr6, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str, ptr %payload7)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload7 to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload7 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload7)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %5 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %6 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload7)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val8 = alloca %RCCValue, align 8
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val8, i32 0, i32 0
  store i64 0, ptr %tag.ptr9, align 4
  %payload.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val8, i32 0, i32 1
  store ptr null, ptr %payload.ptr10, align 8
  store ptr null, ptr %double, align 8
  store ptr @__anon_func_1, ptr %double, align 8
  %rcc.val11 = alloca %RCCValue, align 8
  %tag.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val11, i32 0, i32 0
  store i64 1, ptr %tag.ptr12, align 4
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val11, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr13, align 8
  %double.load = load ptr, ptr %double, align 8
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %double.load, i32 0, i32 1
  %payload15 = load ptr, ptr %payload.ptr14, align 8
  %call.indirect.double = call ptr %payload15(ptr %rcc.val11)
  %tag.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.double, i32 0, i32 0
  %tag17 = load i64, ptr %tag.ptr16, align 4
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.double, i32 0, i32 1
  %payload19 = load ptr, ptr %payload.ptr18, align 8
  switch i64 %tag17, label %sout.default25 [
    i64 1, label %sout.int20
    i64 2, label %sout.float21
    i64 3, label %sout.bool22
    i64 4, label %sout.str23
    i64 0, label %sout.null24
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int20:                                       ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload19)
  br label %sout.merge26

sout.float21:                                     ; preds = %sout.merge
  %float.bits27 = ptrtoint ptr %payload19 to i64
  %float.val28 = bitcast i64 %float.bits27 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val28)
  br label %sout.merge26

sout.bool22:                                      ; preds = %sout.merge
  %bool.val29 = ptrtoint ptr %payload19 to i64
  %is.true30 = icmp ne i64 %bool.val29, 0
  br i1 %is.true30, label %sout.true31, label %sout.false32

sout.str23:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload19)
  br label %sout.merge26

sout.null24:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge26

sout.default25:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload19)
  br label %sout.merge26

sout.merge26:                                     ; preds = %sout.default25, %sout.null24, %sout.str23, %sout.bool.merge33, %sout.float21, %sout.int20
  %15 = call i32 @putchar(i32 10)
  %rcc.val34 = alloca %RCCValue, align 8
  %tag.ptr35 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val34, i32 0, i32 0
  store i64 0, ptr %tag.ptr35, align 4
  %payload.ptr36 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val34, i32 0, i32 1
  store ptr null, ptr %payload.ptr36, align 8
  %rcc.val37 = alloca %RCCValue, align 8
  %tag.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val37, i32 0, i32 0
  store i64 1, ptr %tag.ptr38, align 4
  %payload.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val37, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr39, align 8
  %call.apply = call ptr @apply(ptr @__anon_func_2, ptr %rcc.val37)
  %tag.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %call.apply, i32 0, i32 0
  %tag41 = load i64, ptr %tag.ptr40, align 4
  %payload.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %call.apply, i32 0, i32 1
  %payload43 = load ptr, ptr %payload.ptr42, align 8
  switch i64 %tag41, label %sout.default49 [
    i64 1, label %sout.int44
    i64 2, label %sout.float45
    i64 3, label %sout.bool46
    i64 4, label %sout.str47
    i64 0, label %sout.null48
  ]

sout.true31:                                      ; preds = %sout.bool22
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge33

sout.false32:                                     ; preds = %sout.bool22
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge33

sout.bool.merge33:                                ; preds = %sout.false32, %sout.true31
  br label %sout.merge26

sout.int44:                                       ; preds = %sout.merge26
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, ptr %payload43)
  br label %sout.merge50

sout.float45:                                     ; preds = %sout.merge26
  %float.bits51 = ptrtoint ptr %payload43 to i64
  %float.val52 = bitcast i64 %float.bits51 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val52)
  br label %sout.merge50

sout.bool46:                                      ; preds = %sout.merge26
  %bool.val53 = ptrtoint ptr %payload43 to i64
  %is.true54 = icmp ne i64 %bool.val53, 0
  br i1 %is.true54, label %sout.true55, label %sout.false56

sout.str47:                                       ; preds = %sout.merge26
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload43)
  br label %sout.merge50

sout.null48:                                      ; preds = %sout.merge26
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge50

sout.default49:                                   ; preds = %sout.merge26
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload43)
  br label %sout.merge50

sout.merge50:                                     ; preds = %sout.default49, %sout.null48, %sout.str47, %sout.bool.merge57, %sout.float45, %sout.int44
  %23 = call i32 @putchar(i32 10)
  %rcc.val58 = alloca %RCCValue, align 8
  %tag.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val58, i32 0, i32 0
  store i64 0, ptr %tag.ptr59, align 4
  %payload.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val58, i32 0, i32 1
  store ptr null, ptr %payload.ptr60, align 8
  ret i32 0

sout.true55:                                      ; preds = %sout.bool46
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge57

sout.false56:                                     ; preds = %sout.bool46
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge57

sout.bool.merge57:                                ; preds = %sout.false56, %sout.true55
  br label %sout.merge50
}

define private ptr @__anon_func_0() {
entry:
  %payload = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %payload1 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload1 to i64
  %add = add i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %add to ptr
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr, align 8
  ret ptr %rcc.val
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)

define private ptr @__anon_func_1() {
entry:
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr, align 8
  %payload = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %payload.ptr1 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload2 = load ptr, ptr %payload.ptr1, align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload2 to i64
  %mul = mul i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %mul to ptr
  %rcc.val3 = alloca %RCCValue, align 8
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 0
  store i64 1, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr5, align 8
  ret ptr %rcc.val3
}

define private ptr @apply(ptr %f, ptr %x) {
entry:
  %x2 = alloca ptr, align 8
  %f1 = alloca ptr, align 8
  store ptr %f, ptr %f1, align 8
  store ptr %x, ptr %x2, align 8
  %x.load = load ptr, ptr %x2, align 8
  %f.load = load ptr, ptr %f1, align 8
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %f.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr, align 8
  %call.indirect.f = call ptr %payload(ptr %x.load)
  ret ptr %call.indirect.f
}

define private ptr @__anon_func_2() {
entry:
  %payload = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %payload1 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload1 to i64
  %mul = mul i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %mul to ptr
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr, align 8
  ret ptr %rcc.val
}
