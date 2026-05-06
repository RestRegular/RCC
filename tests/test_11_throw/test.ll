; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [17 x i8] c"division by zero\00"
@.str.1 = private constant [5 x i8] c"%lld\00"
@.str.2 = private constant [3 x i8] c"%g\00"
@.str.3 = private constant [3 x i8] c"%s\00"
@.str.4 = private constant [5 x i8] c"true\00"
@.str.5 = private constant [3 x i8] c"%s\00"
@.str.6 = private constant [6 x i8] c"false\00"
@.str.7 = private constant [3 x i8] c"%s\00"
@.str.8 = private constant [3 x i8] c"%s\00"
@.str.9 = private constant [5 x i8] c"null\00"
@.str.10 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.11 = private constant [17 x i8] c"after throw test\00"
@.str.12 = private constant [5 x i8] c"%lld\00"
@.str.13 = private constant [3 x i8] c"%g\00"
@.str.14 = private constant [3 x i8] c"%s\00"
@.str.15 = private constant [5 x i8] c"true\00"
@.str.16 = private constant [3 x i8] c"%s\00"
@.str.17 = private constant [6 x i8] c"false\00"
@.str.18 = private constant [3 x i8] c"%s\00"
@.str.19 = private constant [3 x i8] c"%s\00"
@.str.20 = private constant [5 x i8] c"null\00"
@.str.21 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr, align 8
  %rcc.val1 = alloca %RCCValue, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr3, align 8
  %call.safeDivide = call ptr @safeDivide(ptr %rcc.val, ptr %rcc.val1)
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %call.safeDivide, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %call.safeDivide, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr5, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %int.val = ptrtoint ptr %payload to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str.1, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.2, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr %payload)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %5 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr @.str.9)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %6 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload)
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
  store i64 4, ptr %tag.ptr10, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 1
  store ptr @.str.11, ptr %payload.ptr11, align 8
  %tag.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 0
  %tag13 = load i64, ptr %tag.ptr12, align 4
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 1
  %payload15 = load ptr, ptr %payload.ptr14, align 8
  switch i64 %tag13, label %sout.default21 [
    i64 1, label %sout.int16
    i64 2, label %sout.float17
    i64 3, label %sout.bool18
    i64 4, label %sout.str19
    i64 0, label %sout.null20
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.3, ptr @.str.4)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.5, ptr @.str.6)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int16:                                       ; preds = %sout.merge
  %int.val23 = ptrtoint ptr %payload15 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.12, i64 %int.val23)
  br label %sout.merge22

sout.float17:                                     ; preds = %sout.merge
  %float.bits24 = ptrtoint ptr %payload15 to i64
  %float.val25 = bitcast i64 %float.bits24 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.13, double %float.val25)
  br label %sout.merge22

sout.bool18:                                      ; preds = %sout.merge
  %bool.val26 = ptrtoint ptr %payload15 to i64
  %is.true27 = icmp ne i64 %bool.val26, 0
  br i1 %is.true27, label %sout.true28, label %sout.false29

sout.str19:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr %payload15)
  br label %sout.merge22

sout.null20:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.merge22

sout.default21:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload15)
  br label %sout.merge22

sout.merge22:                                     ; preds = %sout.default21, %sout.null20, %sout.str19, %sout.bool.merge30, %sout.float17, %sout.int16
  %15 = call i32 @putchar(i32 10)
  %rcc.val31 = alloca %RCCValue, align 8
  %tag.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 0
  store i64 0, ptr %tag.ptr32, align 4
  %payload.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 1
  store ptr null, ptr %payload.ptr33, align 8
  ret i32 0

sout.true28:                                      ; preds = %sout.bool18
  %16 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge30

sout.false29:                                     ; preds = %sout.bool18
  %17 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.17)
  br label %sout.bool.merge30

sout.bool.merge30:                                ; preds = %sout.false29, %sout.true28
  br label %sout.merge22
}

define private ptr @safeDivide(ptr %a, ptr %b) {
entry:
  %arith.result37 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  %b.load = load ptr, ptr %b2, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr null, ptr %payload.ptr, align 8
  %tag.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr3, align 4
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  %tag5 = load i64, ptr %tag.ptr4, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag5, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

cond.merge:                                       ; preds = %cond.next.0, %throw.unreachable
  %a.load = load ptr, ptr %a1, align 8
  %b.load26 = load ptr, ptr %b2, align 8
  %tag.ptr27 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 0
  %tag28 = load i64, ptr %tag.ptr27, align 4
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %b.load26, i32 0, i32 0
  %tag30 = load i64, ptr %tag.ptr29, align 4
  %is_float.left31 = icmp eq i64 %tag28, 2
  %is_float.right32 = icmp eq i64 %tag30, 2
  %is_float33 = or i1 %is_float.left31, %is_float.right32
  store ptr null, ptr %arith.result37, align 8
  br i1 %is_float33, label %arith.float34, label %arith.int35

arith.float:                                      ; preds = %entry
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr6, align 8
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload8 = load ptr, ptr %payload.ptr7, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload8 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val9 = alloca %RCCValue, align 8
  %tag.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 0
  store i64 2, ptr %tag.ptr10, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val9, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr11, align 8
  store ptr %rcc.val9, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload13 = load ptr, ptr %payload.ptr12, align 8
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload15 = load ptr, ptr %payload.ptr14, align 8
  %left.int = ptrtoint ptr %payload13 to i64
  %right.int = ptrtoint ptr %payload15 to i64
  %cmp.eq = icmp eq i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.eq to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val16 = alloca %RCCValue, align 8
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 0
  store i64 3, ptr %tag.ptr17, align 4
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr18, align 8
  store ptr %rcc.val16, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  %tag.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 0
  %tag20 = load i64, ptr %tag.ptr19, align 4
  %payload.ptr21 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 1
  %payload22 = load ptr, ptr %payload.ptr21, align 8
  %is_null = icmp eq i64 %tag20, 0
  %is_int = icmp eq i64 %tag20, 1
  %is_bool = icmp eq i64 %tag20, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload22 to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  br i1 %coerced_bool, label %cond.then.0, label %cond.next.0

cond.then.0:                                      ; preds = %arith.merge
  %rcc.val23 = alloca %RCCValue, align 8
  %tag.ptr24 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val23, i32 0, i32 0
  store i64 4, ptr %tag.ptr24, align 4
  %payload.ptr25 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val23, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr25, align 8
  call void @exit(i32 1)
  unreachable

cond.next.0:                                      ; preds = %arith.merge
  br label %cond.merge

throw.unreachable:                                ; No predecessors!
  br label %cond.merge

arith.float34:                                    ; preds = %cond.merge
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload39 = load ptr, ptr %payload.ptr38, align 8
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %b.load26, i32 0, i32 1
  %payload41 = load ptr, ptr %payload.ptr40, align 8
  %left.bits42 = ptrtoint ptr %payload39 to i64
  %right.bits43 = ptrtoint ptr %payload41 to i64
  %left.sitofp44 = sitofp i64 %left.bits42 to double
  %left.bitcast45 = bitcast i64 %left.bits42 to double
  %left.double46 = select i1 %is_float.left31, double %left.bitcast45, double %left.sitofp44
  %right.sitofp47 = sitofp i64 %right.bits43 to double
  %right.bitcast48 = bitcast i64 %right.bits43 to double
  %right.double49 = select i1 %is_float.right32, double %right.bitcast48, double %right.sitofp47
  %fdiv = fdiv double %left.double46, %right.double49
  %result.bits50 = bitcast double %fdiv to i64
  %result.float.ptr51 = inttoptr i64 %result.bits50 to ptr
  %rcc.val52 = alloca %RCCValue, align 8
  %tag.ptr53 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val52, i32 0, i32 0
  store i64 2, ptr %tag.ptr53, align 4
  %payload.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val52, i32 0, i32 1
  store ptr %result.float.ptr51, ptr %payload.ptr54, align 8
  store ptr %rcc.val52, ptr %arith.result37, align 8
  br label %arith.merge36

arith.int35:                                      ; preds = %cond.merge
  %payload.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload56 = load ptr, ptr %payload.ptr55, align 8
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %b.load26, i32 0, i32 1
  %payload58 = load ptr, ptr %payload.ptr57, align 8
  %left.int59 = ptrtoint ptr %payload56 to i64
  %right.int60 = ptrtoint ptr %payload58 to i64
  %div = sdiv i64 %left.int59, %right.int60
  %int.ptr61 = inttoptr i64 %div to ptr
  %rcc.val62 = alloca %RCCValue, align 8
  %tag.ptr63 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val62, i32 0, i32 0
  store i64 1, ptr %tag.ptr63, align 4
  %payload.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val62, i32 0, i32 1
  store ptr %int.ptr61, ptr %payload.ptr64, align 8
  store ptr %rcc.val62, ptr %arith.result37, align 8
  br label %arith.merge36

arith.merge36:                                    ; preds = %arith.int35, %arith.float34
  %arith.result.load65 = load ptr, ptr %arith.result37, align 8
  ret ptr %arith.result.load65
}

declare void @exit(i32)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
