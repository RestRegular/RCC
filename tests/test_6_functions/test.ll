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

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
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
  %call.add = call ptr @add(ptr %rcc.val, ptr %rcc.val1)
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %call.add, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %call.add, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr5, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str, ptr %payload)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %5 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %6 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val6 = alloca %RCCValue, align 8
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 0
  store i64 0, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 1
  store ptr null, ptr %payload.ptr8, align 8
  %rcc.val9 = alloca %RCCValue, align 8
  %tag.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 0
  store i64 1, ptr %tag.ptr10, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr11, align 8
  %call.factorial = call ptr @factorial(ptr %rcc.val9)
  %tag.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %call.factorial, i32 0, i32 0
  %tag13 = load i64, ptr %tag.ptr12, align 4
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %call.factorial, i32 0, i32 1
  %payload15 = load ptr, ptr %payload.ptr14, align 8
  switch i64 %tag13, label %sout.default21 [
    i64 1, label %sout.int16
    i64 2, label %sout.float17
    i64 3, label %sout.bool18
    i64 4, label %sout.str19
    i64 0, label %sout.null20
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int16:                                       ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload15)
  br label %sout.merge22

sout.float17:                                     ; preds = %sout.merge
  %float.bits23 = ptrtoint ptr %payload15 to i64
  %float.val24 = bitcast i64 %float.bits23 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val24)
  br label %sout.merge22

sout.bool18:                                      ; preds = %sout.merge
  %bool.val25 = ptrtoint ptr %payload15 to i64
  %is.true26 = icmp ne i64 %bool.val25, 0
  br i1 %is.true26, label %sout.true27, label %sout.false28

sout.str19:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload15)
  br label %sout.merge22

sout.null20:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge22

sout.default21:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload15)
  br label %sout.merge22

sout.merge22:                                     ; preds = %sout.default21, %sout.null20, %sout.str19, %sout.bool.merge29, %sout.float17, %sout.int16
  %15 = call i32 @putchar(i32 10)
  %rcc.val30 = alloca %RCCValue, align 8
  %tag.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 0
  store i64 0, ptr %tag.ptr31, align 4
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 1
  store ptr null, ptr %payload.ptr32, align 8
  ret i32 0

sout.true27:                                      ; preds = %sout.bool18
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge29

sout.false28:                                     ; preds = %sout.bool18
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge29

sout.bool.merge29:                                ; preds = %sout.false28, %sout.true27
  br label %sout.merge22
}

define private ptr @add(ptr %a, ptr %b) {
entry:
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  %a.load = load ptr, ptr %a1, align 8
  %b.load = load ptr, ptr %b2, align 8
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr, align 8
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload4 = load ptr, ptr %payload.ptr3, align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload4 to i64
  %add = add i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %add to ptr
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr5, align 8
  ret ptr %rcc.val
}

define private ptr @factorial(ptr %n) {
entry:
  %n1 = alloca ptr, align 8
  store ptr %n, ptr %n1, align 8
  %n.load = load ptr, ptr %n1, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr, align 8
  %payload.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr2, align 8
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload4 = load ptr, ptr %payload.ptr3, align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload4 to i64
  %cmp.le = icmp sle i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.le to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val5 = alloca %RCCValue, align 8
  %tag.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val5, i32 0, i32 0
  store i64 1, ptr %tag.ptr6, align 4
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val5, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr7, align 8
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val5, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val5, i32 0, i32 1
  %payload10 = load ptr, ptr %payload.ptr9, align 8
  %is_null = icmp eq i64 %tag, 0
  %is_int = icmp eq i64 %tag, 1
  %is_bool = icmp eq i64 %tag, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload10 to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  br i1 %coerced_bool, label %cond.then.0, label %cond.next.0

cond.merge:                                       ; preds = %cond.next.0
  %n.load14 = load ptr, ptr %n1, align 8
  %n.load15 = load ptr, ptr %n1, align 8
  %rcc.val16 = alloca %RCCValue, align 8
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 0
  store i64 1, ptr %tag.ptr17, align 4
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr18, align 8
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %n.load15, i32 0, i32 1
  %payload20 = load ptr, ptr %payload.ptr19, align 8
  %payload.ptr21 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 1
  %payload22 = load ptr, ptr %payload.ptr21, align 8
  %left.int23 = ptrtoint ptr %payload20 to i64
  %right.int24 = ptrtoint ptr %payload22 to i64
  %sub = sub i64 %left.int23, %right.int24
  %int.ptr25 = inttoptr i64 %sub to ptr
  %rcc.val26 = alloca %RCCValue, align 8
  %tag.ptr27 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val26, i32 0, i32 0
  store i64 1, ptr %tag.ptr27, align 4
  %payload.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val26, i32 0, i32 1
  store ptr %int.ptr25, ptr %payload.ptr28, align 8
  %call.factorial = call ptr @factorial(ptr %rcc.val26)
  %payload.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %n.load14, i32 0, i32 1
  %payload30 = load ptr, ptr %payload.ptr29, align 8
  %payload.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %call.factorial, i32 0, i32 1
  %payload32 = load ptr, ptr %payload.ptr31, align 8
  %left.int33 = ptrtoint ptr %payload30 to i64
  %right.int34 = ptrtoint ptr %payload32 to i64
  %mul = mul i64 %left.int33, %right.int34
  %int.ptr35 = inttoptr i64 %mul to ptr
  %rcc.val36 = alloca %RCCValue, align 8
  %tag.ptr37 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val36, i32 0, i32 0
  store i64 1, ptr %tag.ptr37, align 4
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val36, i32 0, i32 1
  store ptr %int.ptr35, ptr %payload.ptr38, align 8
  ret ptr %rcc.val36

cond.then.0:                                      ; preds = %entry
  %rcc.val11 = alloca %RCCValue, align 8
  %tag.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val11, i32 0, i32 0
  store i64 1, ptr %tag.ptr12, align 4
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val11, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr13, align 8
  ret ptr %rcc.val11

cond.next.0:                                      ; preds = %entry
  br label %cond.merge
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
