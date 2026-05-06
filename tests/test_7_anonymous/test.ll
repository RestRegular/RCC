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
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 7, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr @__anon_func_0, ptr %payload.ptr, align 8
  store ptr %rcc.val.heap, ptr %add, align 8
  %rcc.val.heap1 = call ptr @malloc(i64 16)
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr3, align 8
  %rcc.val.heap4 = call ptr @malloc(i64 16)
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 0
  store i64 1, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr6, align 8
  %add.load = load ptr, ptr %add, align 8
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %add.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr7, align 8
  %call.indirect.add = call ptr %payload(ptr %rcc.val.heap1, ptr %rcc.val.heap4)
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.add, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.add, i32 0, i32 1
  %payload10 = load ptr, ptr %payload.ptr9, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %int.val = ptrtoint ptr %payload10 to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload10 to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload10 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload10)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %5 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %6 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload10)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val.heap11 = call ptr @malloc(i64 16)
  %tag.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap11, i32 0, i32 0
  store i64 0, ptr %tag.ptr12, align 4
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap11, i32 0, i32 1
  store ptr null, ptr %payload.ptr13, align 8
  store ptr null, ptr %double, align 8
  %rcc.val.heap14 = call ptr @malloc(i64 16)
  %tag.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap14, i32 0, i32 0
  store i64 7, ptr %tag.ptr15, align 4
  %payload.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap14, i32 0, i32 1
  store ptr @__anon_func_1, ptr %payload.ptr16, align 8
  store ptr %rcc.val.heap14, ptr %double, align 8
  %rcc.val.heap17 = call ptr @malloc(i64 16)
  %tag.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap17, i32 0, i32 0
  store i64 1, ptr %tag.ptr18, align 4
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap17, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr19, align 8
  %double.load = load ptr, ptr %double, align 8
  %payload.ptr20 = getelementptr inbounds nuw %RCCValue, ptr %double.load, i32 0, i32 1
  %payload21 = load ptr, ptr %payload.ptr20, align 8
  %call.indirect.double = call ptr %payload21(ptr %rcc.val.heap17)
  %tag.ptr22 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.double, i32 0, i32 0
  %tag23 = load i64, ptr %tag.ptr22, align 4
  %payload.ptr24 = getelementptr inbounds nuw %RCCValue, ptr %call.indirect.double, i32 0, i32 1
  %payload25 = load ptr, ptr %payload.ptr24, align 8
  switch i64 %tag23, label %sout.default31 [
    i64 1, label %sout.int26
    i64 2, label %sout.float27
    i64 3, label %sout.bool28
    i64 4, label %sout.str29
    i64 0, label %sout.null30
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int26:                                       ; preds = %sout.merge
  %int.val33 = ptrtoint ptr %payload25 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val33)
  br label %sout.merge32

sout.float27:                                     ; preds = %sout.merge
  %float.bits34 = ptrtoint ptr %payload25 to i64
  %float.val35 = bitcast i64 %float.bits34 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val35)
  br label %sout.merge32

sout.bool28:                                      ; preds = %sout.merge
  %bool.val36 = ptrtoint ptr %payload25 to i64
  %is.true37 = icmp ne i64 %bool.val36, 0
  br i1 %is.true37, label %sout.true38, label %sout.false39

sout.str29:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload25)
  br label %sout.merge32

sout.null30:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge32

sout.default31:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload25)
  br label %sout.merge32

sout.merge32:                                     ; preds = %sout.default31, %sout.null30, %sout.str29, %sout.bool.merge40, %sout.float27, %sout.int26
  %15 = call i32 @putchar(i32 10)
  %rcc.val.heap41 = call ptr @malloc(i64 16)
  %tag.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap41, i32 0, i32 0
  store i64 0, ptr %tag.ptr42, align 4
  %payload.ptr43 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap41, i32 0, i32 1
  store ptr null, ptr %payload.ptr43, align 8
  %rcc.val.heap44 = call ptr @malloc(i64 16)
  %tag.ptr45 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap44, i32 0, i32 0
  store i64 7, ptr %tag.ptr45, align 4
  %payload.ptr46 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap44, i32 0, i32 1
  store ptr @__anon_func_2, ptr %payload.ptr46, align 8
  %rcc.val.heap47 = call ptr @malloc(i64 16)
  %tag.ptr48 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap47, i32 0, i32 0
  store i64 1, ptr %tag.ptr48, align 4
  %payload.ptr49 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap47, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr49, align 8
  %call.apply = call ptr @apply(ptr %rcc.val.heap44, ptr %rcc.val.heap47)
  %tag.ptr50 = getelementptr inbounds nuw %RCCValue, ptr %call.apply, i32 0, i32 0
  %tag51 = load i64, ptr %tag.ptr50, align 4
  %payload.ptr52 = getelementptr inbounds nuw %RCCValue, ptr %call.apply, i32 0, i32 1
  %payload53 = load ptr, ptr %payload.ptr52, align 8
  switch i64 %tag51, label %sout.default59 [
    i64 1, label %sout.int54
    i64 2, label %sout.float55
    i64 3, label %sout.bool56
    i64 4, label %sout.str57
    i64 0, label %sout.null58
  ]

sout.true38:                                      ; preds = %sout.bool28
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge40

sout.false39:                                     ; preds = %sout.bool28
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge40

sout.bool.merge40:                                ; preds = %sout.false39, %sout.true38
  br label %sout.merge32

sout.int54:                                       ; preds = %sout.merge32
  %int.val61 = ptrtoint ptr %payload53 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val61)
  br label %sout.merge60

sout.float55:                                     ; preds = %sout.merge32
  %float.bits62 = ptrtoint ptr %payload53 to i64
  %float.val63 = bitcast i64 %float.bits62 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val63)
  br label %sout.merge60

sout.bool56:                                      ; preds = %sout.merge32
  %bool.val64 = ptrtoint ptr %payload53 to i64
  %is.true65 = icmp ne i64 %bool.val64, 0
  br i1 %is.true65, label %sout.true66, label %sout.false67

sout.str57:                                       ; preds = %sout.merge32
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload53)
  br label %sout.merge60

sout.null58:                                      ; preds = %sout.merge32
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge60

sout.default59:                                   ; preds = %sout.merge32
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload53)
  br label %sout.merge60

sout.merge60:                                     ; preds = %sout.default59, %sout.null58, %sout.str57, %sout.bool.merge68, %sout.float55, %sout.int54
  %23 = call i32 @putchar(i32 10)
  %rcc.val.heap69 = call ptr @malloc(i64 16)
  %tag.ptr70 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap69, i32 0, i32 0
  store i64 0, ptr %tag.ptr70, align 4
  %payload.ptr71 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap69, i32 0, i32 1
  store ptr null, ptr %payload.ptr71, align 8
  ret i32 0

sout.true66:                                      ; preds = %sout.bool56
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge68

sout.false67:                                     ; preds = %sout.bool56
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge68

sout.bool.merge68:                                ; preds = %sout.false67, %sout.true66
  br label %sout.merge60
}

define private ptr @__anon_func_0(ptr %a, ptr %b) {
entry:
  %arith.result = alloca ptr, align 8
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  %a.load = load ptr, ptr %a1, align 8
  %b.load = load ptr, ptr %b2, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr, align 4
  %tag.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 0
  %tag4 = load i64, ptr %tag.ptr3, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag4, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr, align 8
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload6 = load ptr, ptr %payload.ptr5, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload6 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %fadd = fadd double %left.double, %right.double
  %result.bits = bitcast double %fadd to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 2, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr8, align 8
  store ptr %rcc.val.heap, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload10 = load ptr, ptr %payload.ptr9, align 8
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %left.int = ptrtoint ptr %payload10 to i64
  %right.int = ptrtoint ptr %payload12 to i64
  %add = add i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %add to ptr
  %rcc.val.heap13 = call ptr @malloc(i64 16)
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap13, i32 0, i32 0
  store i64 1, ptr %tag.ptr14, align 4
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap13, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr15, align 8
  store ptr %rcc.val.heap13, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  ret ptr %arith.result.load
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)

define private ptr @__anon_func_1(ptr %x) {
entry:
  %arith.result = alloca ptr, align 8
  %x1 = alloca ptr, align 8
  store ptr %x, ptr %x1, align 8
  %x.load = load ptr, ptr %x1, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr2, align 4
  %tag.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  %tag4 = load i64, ptr %tag.ptr3, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag4, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr5, align 8
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  %payload7 = load ptr, ptr %payload.ptr6, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload7 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %fmul = fmul double %left.double, %right.double
  %result.bits = bitcast double %fmul to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val.heap8 = call ptr @malloc(i64 16)
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 0
  store i64 2, ptr %tag.ptr9, align 4
  %payload.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr10, align 8
  store ptr %rcc.val.heap8, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  %payload14 = load ptr, ptr %payload.ptr13, align 8
  %left.int = ptrtoint ptr %payload12 to i64
  %right.int = ptrtoint ptr %payload14 to i64
  %mul = mul i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %mul to ptr
  %rcc.val.heap15 = call ptr @malloc(i64 16)
  %tag.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 0
  store i64 1, ptr %tag.ptr16, align 4
  %payload.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr17, align 8
  store ptr %rcc.val.heap15, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  ret ptr %arith.result.load
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

define private ptr @__anon_func_2(ptr %n) {
entry:
  %arith.result = alloca ptr, align 8
  %n1 = alloca ptr, align 8
  store ptr %n, ptr %n1, align 8
  %n.load = load ptr, ptr %n1, align 8
  %n.load2 = load ptr, ptr %n1, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr, align 4
  %tag.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %n.load2, i32 0, i32 0
  %tag4 = load i64, ptr %tag.ptr3, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag4, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr, align 8
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %n.load2, i32 0, i32 1
  %payload6 = load ptr, ptr %payload.ptr5, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload6 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %fmul = fmul double %left.double, %right.double
  %result.bits = bitcast double %fmul to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 2, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr8, align 8
  store ptr %rcc.val.heap, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
  %payload10 = load ptr, ptr %payload.ptr9, align 8
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %n.load2, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %left.int = ptrtoint ptr %payload10 to i64
  %right.int = ptrtoint ptr %payload12 to i64
  %mul = mul i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %mul to ptr
  %rcc.val.heap13 = call ptr @malloc(i64 16)
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap13, i32 0, i32 0
  store i64 1, ptr %tag.ptr14, align 4
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap13, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr15, align 8
  store ptr %rcc.val.heap13, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  ret ptr %arith.result.load
}
