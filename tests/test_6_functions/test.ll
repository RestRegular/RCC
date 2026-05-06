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
  %int.val = ptrtoint ptr %payload to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
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
  %int.val23 = ptrtoint ptr %payload15 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val23)
  br label %sout.merge22

sout.float17:                                     ; preds = %sout.merge
  %float.bits24 = ptrtoint ptr %payload15 to i64
  %float.val25 = bitcast i64 %float.bits24 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val25)
  br label %sout.merge22

sout.bool18:                                      ; preds = %sout.merge
  %bool.val26 = ptrtoint ptr %payload15 to i64
  %is.true27 = icmp ne i64 %bool.val26, 0
  br i1 %is.true27, label %sout.true28, label %sout.false29

sout.str19:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload15)
  br label %sout.merge22

sout.null20:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge22

sout.default21:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload15)
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
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge30

sout.false29:                                     ; preds = %sout.bool18
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge30

sout.bool.merge30:                                ; preds = %sout.false29, %sout.true28
  br label %sout.merge22
}

define private ptr @add(ptr %a, ptr %b) {
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
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 2, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr8, align 8
  store ptr %rcc.val, ptr %arith.result, align 8
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
  %rcc.val13 = alloca %RCCValue, align 8
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 0
  store i64 1, ptr %tag.ptr14, align 4
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr15, align 8
  store ptr %rcc.val13, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  ret ptr %arith.result.load
}

define private ptr @factorial(ptr %n) {
entry:
  %arith.result79 = alloca ptr, align 8
  %arith.result40 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
  %n1 = alloca ptr, align 8
  store ptr %n, ptr %n1, align 8
  %n.load = load ptr, ptr %n1, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr2, align 4
  %tag.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  %tag4 = load i64, ptr %tag.ptr3, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag4, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

cond.merge:                                       ; preds = %cond.next.0
  %n.load25 = load ptr, ptr %n1, align 8
  %n.load26 = load ptr, ptr %n1, align 8
  %rcc.val27 = alloca %RCCValue, align 8
  %tag.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 0
  store i64 1, ptr %tag.ptr28, align 4
  %payload.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr29, align 8
  %tag.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %n.load26, i32 0, i32 0
  %tag31 = load i64, ptr %tag.ptr30, align 4
  %tag.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 0
  %tag33 = load i64, ptr %tag.ptr32, align 4
  %is_float.left34 = icmp eq i64 %tag31, 2
  %is_float.right35 = icmp eq i64 %tag33, 2
  %is_float36 = or i1 %is_float.left34, %is_float.right35
  store ptr null, ptr %arith.result40, align 8
  br i1 %is_float36, label %arith.float37, label %arith.int38

arith.float:                                      ; preds = %entry
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr5, align 8
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload7 = load ptr, ptr %payload.ptr6, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload7 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val8 = alloca %RCCValue, align 8
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val8, i32 0, i32 0
  store i64 2, ptr %tag.ptr9, align 4
  %payload.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val8, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr10, align 8
  store ptr %rcc.val8, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload14 = load ptr, ptr %payload.ptr13, align 8
  %left.int = ptrtoint ptr %payload12 to i64
  %right.int = ptrtoint ptr %payload14 to i64
  %cmp.le = icmp sle i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.le to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val15 = alloca %RCCValue, align 8
  %tag.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val15, i32 0, i32 0
  store i64 3, ptr %tag.ptr16, align 4
  %payload.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val15, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr17, align 8
  store ptr %rcc.val15, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  %tag.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 0
  %tag19 = load i64, ptr %tag.ptr18, align 4
  %payload.ptr20 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 1
  %payload21 = load ptr, ptr %payload.ptr20, align 8
  %is_null = icmp eq i64 %tag19, 0
  %is_int = icmp eq i64 %tag19, 1
  %is_bool = icmp eq i64 %tag19, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload21 to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  br i1 %coerced_bool, label %cond.then.0, label %cond.next.0

cond.then.0:                                      ; preds = %arith.merge
  %rcc.val22 = alloca %RCCValue, align 8
  %tag.ptr23 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val22, i32 0, i32 0
  store i64 1, ptr %tag.ptr23, align 4
  %payload.ptr24 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val22, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr24, align 8
  ret ptr %rcc.val22

cond.next.0:                                      ; preds = %arith.merge
  br label %cond.merge

arith.float37:                                    ; preds = %cond.merge
  %payload.ptr41 = getelementptr inbounds nuw %RCCValue, ptr %n.load26, i32 0, i32 1
  %payload42 = load ptr, ptr %payload.ptr41, align 8
  %payload.ptr43 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  %payload44 = load ptr, ptr %payload.ptr43, align 8
  %left.bits45 = ptrtoint ptr %payload42 to i64
  %right.bits46 = ptrtoint ptr %payload44 to i64
  %left.sitofp47 = sitofp i64 %left.bits45 to double
  %left.bitcast48 = bitcast i64 %left.bits45 to double
  %left.double49 = select i1 %is_float.left34, double %left.bitcast48, double %left.sitofp47
  %right.sitofp50 = sitofp i64 %right.bits46 to double
  %right.bitcast51 = bitcast i64 %right.bits46 to double
  %right.double52 = select i1 %is_float.right35, double %right.bitcast51, double %right.sitofp50
  %fsub = fsub double %left.double49, %right.double52
  %result.bits53 = bitcast double %fsub to i64
  %result.float.ptr54 = inttoptr i64 %result.bits53 to ptr
  %rcc.val55 = alloca %RCCValue, align 8
  %tag.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val55, i32 0, i32 0
  store i64 2, ptr %tag.ptr56, align 4
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val55, i32 0, i32 1
  store ptr %result.float.ptr54, ptr %payload.ptr57, align 8
  store ptr %rcc.val55, ptr %arith.result40, align 8
  br label %arith.merge39

arith.int38:                                      ; preds = %cond.merge
  %payload.ptr58 = getelementptr inbounds nuw %RCCValue, ptr %n.load26, i32 0, i32 1
  %payload59 = load ptr, ptr %payload.ptr58, align 8
  %payload.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  %payload61 = load ptr, ptr %payload.ptr60, align 8
  %left.int62 = ptrtoint ptr %payload59 to i64
  %right.int63 = ptrtoint ptr %payload61 to i64
  %sub = sub i64 %left.int62, %right.int63
  %int.ptr64 = inttoptr i64 %sub to ptr
  %rcc.val65 = alloca %RCCValue, align 8
  %tag.ptr66 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val65, i32 0, i32 0
  store i64 1, ptr %tag.ptr66, align 4
  %payload.ptr67 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val65, i32 0, i32 1
  store ptr %int.ptr64, ptr %payload.ptr67, align 8
  store ptr %rcc.val65, ptr %arith.result40, align 8
  br label %arith.merge39

arith.merge39:                                    ; preds = %arith.int38, %arith.float37
  %arith.result.load68 = load ptr, ptr %arith.result40, align 8
  %call.factorial = call ptr @factorial(ptr %arith.result.load68)
  %tag.ptr69 = getelementptr inbounds nuw %RCCValue, ptr %n.load25, i32 0, i32 0
  %tag70 = load i64, ptr %tag.ptr69, align 4
  %tag.ptr71 = getelementptr inbounds nuw %RCCValue, ptr %call.factorial, i32 0, i32 0
  %tag72 = load i64, ptr %tag.ptr71, align 4
  %is_float.left73 = icmp eq i64 %tag70, 2
  %is_float.right74 = icmp eq i64 %tag72, 2
  %is_float75 = or i1 %is_float.left73, %is_float.right74
  store ptr null, ptr %arith.result79, align 8
  br i1 %is_float75, label %arith.float76, label %arith.int77

arith.float76:                                    ; preds = %arith.merge39
  %payload.ptr80 = getelementptr inbounds nuw %RCCValue, ptr %n.load25, i32 0, i32 1
  %payload81 = load ptr, ptr %payload.ptr80, align 8
  %payload.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %call.factorial, i32 0, i32 1
  %payload83 = load ptr, ptr %payload.ptr82, align 8
  %left.bits84 = ptrtoint ptr %payload81 to i64
  %right.bits85 = ptrtoint ptr %payload83 to i64
  %left.sitofp86 = sitofp i64 %left.bits84 to double
  %left.bitcast87 = bitcast i64 %left.bits84 to double
  %left.double88 = select i1 %is_float.left73, double %left.bitcast87, double %left.sitofp86
  %right.sitofp89 = sitofp i64 %right.bits85 to double
  %right.bitcast90 = bitcast i64 %right.bits85 to double
  %right.double91 = select i1 %is_float.right74, double %right.bitcast90, double %right.sitofp89
  %fmul = fmul double %left.double88, %right.double91
  %result.bits92 = bitcast double %fmul to i64
  %result.float.ptr93 = inttoptr i64 %result.bits92 to ptr
  %rcc.val94 = alloca %RCCValue, align 8
  %tag.ptr95 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val94, i32 0, i32 0
  store i64 2, ptr %tag.ptr95, align 4
  %payload.ptr96 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val94, i32 0, i32 1
  store ptr %result.float.ptr93, ptr %payload.ptr96, align 8
  store ptr %rcc.val94, ptr %arith.result79, align 8
  br label %arith.merge78

arith.int77:                                      ; preds = %arith.merge39
  %payload.ptr97 = getelementptr inbounds nuw %RCCValue, ptr %n.load25, i32 0, i32 1
  %payload98 = load ptr, ptr %payload.ptr97, align 8
  %payload.ptr99 = getelementptr inbounds nuw %RCCValue, ptr %call.factorial, i32 0, i32 1
  %payload100 = load ptr, ptr %payload.ptr99, align 8
  %left.int101 = ptrtoint ptr %payload98 to i64
  %right.int102 = ptrtoint ptr %payload100 to i64
  %mul = mul i64 %left.int101, %right.int102
  %int.ptr103 = inttoptr i64 %mul to ptr
  %rcc.val104 = alloca %RCCValue, align 8
  %tag.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val104, i32 0, i32 0
  store i64 1, ptr %tag.ptr105, align 4
  %payload.ptr106 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val104, i32 0, i32 1
  store ptr %int.ptr103, ptr %payload.ptr106, align 8
  store ptr %rcc.val104, ptr %arith.result79, align 8
  br label %arith.merge78

arith.merge78:                                    ; preds = %arith.int77, %arith.float76
  %arith.result.load107 = load ptr, ptr %arith.result79, align 8
  ret ptr %arith.result.load107
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
