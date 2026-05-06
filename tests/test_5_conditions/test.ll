; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [14 x i8] c"if-true: pass\00"
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
@.str.11 = private constant [15 x i8] c"if-false: FAIL\00"
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
@.str.22 = private constant [14 x i8] c"if-else: pass\00"
@.str.23 = private constant [5 x i8] c"%lld\00"
@.str.24 = private constant [3 x i8] c"%g\00"
@.str.25 = private constant [3 x i8] c"%s\00"
@.str.26 = private constant [5 x i8] c"true\00"
@.str.27 = private constant [3 x i8] c"%s\00"
@.str.28 = private constant [6 x i8] c"false\00"
@.str.29 = private constant [3 x i8] c"%s\00"
@.str.30 = private constant [3 x i8] c"%s\00"
@.str.31 = private constant [5 x i8] c"null\00"
@.str.32 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.33 = private constant [13 x i8] c"elif: FAIL-A\00"
@.str.34 = private constant [5 x i8] c"%lld\00"
@.str.35 = private constant [3 x i8] c"%g\00"
@.str.36 = private constant [3 x i8] c"%s\00"
@.str.37 = private constant [5 x i8] c"true\00"
@.str.38 = private constant [3 x i8] c"%s\00"
@.str.39 = private constant [6 x i8] c"false\00"
@.str.40 = private constant [3 x i8] c"%s\00"
@.str.41 = private constant [3 x i8] c"%s\00"
@.str.42 = private constant [5 x i8] c"null\00"
@.str.43 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.44 = private constant [13 x i8] c"elif: pass-B\00"
@.str.45 = private constant [5 x i8] c"%lld\00"
@.str.46 = private constant [3 x i8] c"%g\00"
@.str.47 = private constant [3 x i8] c"%s\00"
@.str.48 = private constant [5 x i8] c"true\00"
@.str.49 = private constant [3 x i8] c"%s\00"
@.str.50 = private constant [6 x i8] c"false\00"
@.str.51 = private constant [3 x i8] c"%s\00"
@.str.52 = private constant [3 x i8] c"%s\00"
@.str.53 = private constant [5 x i8] c"null\00"
@.str.54 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.55 = private constant [13 x i8] c"elif: FAIL-C\00"
@.str.56 = private constant [5 x i8] c"%lld\00"
@.str.57 = private constant [3 x i8] c"%g\00"
@.str.58 = private constant [3 x i8] c"%s\00"
@.str.59 = private constant [5 x i8] c"true\00"
@.str.60 = private constant [3 x i8] c"%s\00"
@.str.61 = private constant [6 x i8] c"false\00"
@.str.62 = private constant [3 x i8] c"%s\00"
@.str.63 = private constant [3 x i8] c"%s\00"
@.str.64 = private constant [5 x i8] c"null\00"
@.str.65 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %arith.result177 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
  %x = alloca ptr, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 3, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr, align 8
  %tag.ptr1 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr1, align 4
  %payload.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr2, align 8
  %is_null = icmp eq i64 %tag, 0
  %is_int = icmp eq i64 %tag, 1
  %is_bool = icmp eq i64 %tag, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  br i1 %coerced_bool, label %cond.then.0, label %cond.next.0

cond.merge:                                       ; preds = %cond.next.0, %sout.merge
  %rcc.val14 = alloca %RCCValue, align 8
  %tag.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val14, i32 0, i32 0
  store i64 3, ptr %tag.ptr15, align 4
  %payload.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val14, i32 0, i32 1
  store ptr null, ptr %payload.ptr16, align 8
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val14, i32 0, i32 0
  %tag18 = load i64, ptr %tag.ptr17, align 4
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val14, i32 0, i32 1
  %payload20 = load ptr, ptr %payload.ptr19, align 8
  %is_null21 = icmp eq i64 %tag18, 0
  %is_int22 = icmp eq i64 %tag18, 1
  %is_bool23 = icmp eq i64 %tag18, 3
  %is_bool_or_int24 = or i1 %is_bool23, %is_int22
  %payload.int25 = ptrtoint ptr %payload20 to i64
  %payload_nz26 = icmp ne i64 %payload.int25, 0
  %not_null27 = xor i1 %is_null21, true
  %bool_int_result28 = select i1 %is_bool_or_int24, i1 %payload_nz26, i1 true
  %coerced_bool29 = select i1 %is_null21, i1 false, i1 %bool_int_result28
  br i1 %coerced_bool29, label %cond.then.030, label %cond.next.031

cond.then.0:                                      ; preds = %entry
  %rcc.val3 = alloca %RCCValue, align 8
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 0
  store i64 4, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr5, align 8
  %tag.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 0
  %tag7 = load i64, ptr %tag.ptr6, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 1
  %payload9 = load ptr, ptr %payload.ptr8, align 8
  switch i64 %tag7, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

cond.next.0:                                      ; preds = %entry
  br label %cond.merge

sout.int:                                         ; preds = %cond.then.0
  %int.val = ptrtoint ptr %payload9 to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str.1, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %cond.then.0
  %float.bits = ptrtoint ptr %payload9 to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.2, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %cond.then.0
  %bool.val = ptrtoint ptr %payload9 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %cond.then.0
  %4 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr %payload9)
  br label %sout.merge

sout.null:                                        ; preds = %cond.then.0
  %5 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr @.str.9)
  br label %sout.merge

sout.default:                                     ; preds = %cond.then.0
  %6 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload9)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val10 = alloca %RCCValue, align 8
  %tag.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val10, i32 0, i32 0
  store i64 0, ptr %tag.ptr11, align 4
  %payload.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val10, i32 0, i32 1
  store ptr null, ptr %payload.ptr12, align 8
  br label %cond.merge

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.3, ptr @.str.4)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.5, ptr @.str.6)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

cond.merge13:                                     ; preds = %cond.next.1, %sout.merge86, %sout.merge45
  store ptr null, ptr %x, align 8
  %rcc.val98 = alloca %RCCValue, align 8
  %tag.ptr99 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val98, i32 0, i32 0
  store i64 1, ptr %tag.ptr99, align 4
  %payload.ptr100 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val98, i32 0, i32 1
  store ptr inttoptr (i64 15 to ptr), ptr %payload.ptr100, align 8
  store ptr %rcc.val98, ptr %x, align 8
  %x.load = load ptr, ptr %x, align 8
  %rcc.val102 = alloca %RCCValue, align 8
  %tag.ptr103 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 0
  store i64 1, ptr %tag.ptr103, align 4
  %payload.ptr104 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr104, align 8
  %tag.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 0
  %tag106 = load i64, ptr %tag.ptr105, align 4
  %tag.ptr107 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 0
  %tag108 = load i64, ptr %tag.ptr107, align 4
  %is_float.left = icmp eq i64 %tag106, 2
  %is_float.right = icmp eq i64 %tag108, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

cond.then.030:                                    ; preds = %cond.merge
  %rcc.val32 = alloca %RCCValue, align 8
  %tag.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val32, i32 0, i32 0
  store i64 4, ptr %tag.ptr33, align 4
  %payload.ptr34 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val32, i32 0, i32 1
  store ptr @.str.11, ptr %payload.ptr34, align 8
  %tag.ptr35 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val32, i32 0, i32 0
  %tag36 = load i64, ptr %tag.ptr35, align 4
  %payload.ptr37 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val32, i32 0, i32 1
  %payload38 = load ptr, ptr %payload.ptr37, align 8
  switch i64 %tag36, label %sout.default44 [
    i64 1, label %sout.int39
    i64 2, label %sout.float40
    i64 3, label %sout.bool41
    i64 4, label %sout.str42
    i64 0, label %sout.null43
  ]

cond.next.031:                                    ; preds = %cond.merge
  %rcc.val57 = alloca %RCCValue, align 8
  %tag.ptr58 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val57, i32 0, i32 0
  store i64 3, ptr %tag.ptr58, align 4
  %payload.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val57, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr59, align 8
  %tag.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val57, i32 0, i32 0
  %tag61 = load i64, ptr %tag.ptr60, align 4
  %payload.ptr62 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val57, i32 0, i32 1
  %payload63 = load ptr, ptr %payload.ptr62, align 8
  %is_null64 = icmp eq i64 %tag61, 0
  %is_int65 = icmp eq i64 %tag61, 1
  %is_bool66 = icmp eq i64 %tag61, 3
  %is_bool_or_int67 = or i1 %is_bool66, %is_int65
  %payload.int68 = ptrtoint ptr %payload63 to i64
  %payload_nz69 = icmp ne i64 %payload.int68, 0
  %not_null70 = xor i1 %is_null64, true
  %bool_int_result71 = select i1 %is_bool_or_int67, i1 %payload_nz69, i1 true
  %coerced_bool72 = select i1 %is_null64, i1 false, i1 %bool_int_result71
  br i1 %coerced_bool72, label %cond.then.1, label %cond.next.1

sout.int39:                                       ; preds = %cond.then.030
  %int.val46 = ptrtoint ptr %payload38 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.12, i64 %int.val46)
  br label %sout.merge45

sout.float40:                                     ; preds = %cond.then.030
  %float.bits47 = ptrtoint ptr %payload38 to i64
  %float.val48 = bitcast i64 %float.bits47 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.13, double %float.val48)
  br label %sout.merge45

sout.bool41:                                      ; preds = %cond.then.030
  %bool.val49 = ptrtoint ptr %payload38 to i64
  %is.true50 = icmp ne i64 %bool.val49, 0
  br i1 %is.true50, label %sout.true51, label %sout.false52

sout.str42:                                       ; preds = %cond.then.030
  %12 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr %payload38)
  br label %sout.merge45

sout.null43:                                      ; preds = %cond.then.030
  %13 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.merge45

sout.default44:                                   ; preds = %cond.then.030
  %14 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload38)
  br label %sout.merge45

sout.merge45:                                     ; preds = %sout.default44, %sout.null43, %sout.str42, %sout.bool.merge53, %sout.float40, %sout.int39
  %15 = call i32 @putchar(i32 10)
  %rcc.val54 = alloca %RCCValue, align 8
  %tag.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 0
  store i64 0, ptr %tag.ptr55, align 4
  %payload.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 1
  store ptr null, ptr %payload.ptr56, align 8
  br label %cond.merge13

sout.true51:                                      ; preds = %sout.bool41
  %16 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge53

sout.false52:                                     ; preds = %sout.bool41
  %17 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.17)
  br label %sout.bool.merge53

sout.bool.merge53:                                ; preds = %sout.false52, %sout.true51
  br label %sout.merge45

cond.then.1:                                      ; preds = %cond.next.031
  %rcc.val73 = alloca %RCCValue, align 8
  %tag.ptr74 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val73, i32 0, i32 0
  store i64 4, ptr %tag.ptr74, align 4
  %payload.ptr75 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val73, i32 0, i32 1
  store ptr @.str.22, ptr %payload.ptr75, align 8
  %tag.ptr76 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val73, i32 0, i32 0
  %tag77 = load i64, ptr %tag.ptr76, align 4
  %payload.ptr78 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val73, i32 0, i32 1
  %payload79 = load ptr, ptr %payload.ptr78, align 8
  switch i64 %tag77, label %sout.default85 [
    i64 1, label %sout.int80
    i64 2, label %sout.float81
    i64 3, label %sout.bool82
    i64 4, label %sout.str83
    i64 0, label %sout.null84
  ]

cond.next.1:                                      ; preds = %cond.next.031
  br label %cond.merge13

sout.int80:                                       ; preds = %cond.then.1
  %int.val87 = ptrtoint ptr %payload79 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.23, i64 %int.val87)
  br label %sout.merge86

sout.float81:                                     ; preds = %cond.then.1
  %float.bits88 = ptrtoint ptr %payload79 to i64
  %float.val89 = bitcast i64 %float.bits88 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.24, double %float.val89)
  br label %sout.merge86

sout.bool82:                                      ; preds = %cond.then.1
  %bool.val90 = ptrtoint ptr %payload79 to i64
  %is.true91 = icmp ne i64 %bool.val90, 0
  br i1 %is.true91, label %sout.true92, label %sout.false93

sout.str83:                                       ; preds = %cond.then.1
  %20 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload79)
  br label %sout.merge86

sout.null84:                                      ; preds = %cond.then.1
  %21 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr @.str.31)
  br label %sout.merge86

sout.default85:                                   ; preds = %cond.then.1
  %22 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr %payload79)
  br label %sout.merge86

sout.merge86:                                     ; preds = %sout.default85, %sout.null84, %sout.str83, %sout.bool.merge94, %sout.float81, %sout.int80
  %23 = call i32 @putchar(i32 10)
  %rcc.val95 = alloca %RCCValue, align 8
  %tag.ptr96 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val95, i32 0, i32 0
  store i64 0, ptr %tag.ptr96, align 4
  %payload.ptr97 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val95, i32 0, i32 1
  store ptr null, ptr %payload.ptr97, align 8
  br label %cond.merge13

sout.true92:                                      ; preds = %sout.bool82
  %24 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge94

sout.false93:                                     ; preds = %sout.bool82
  %25 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.bool.merge94

sout.bool.merge94:                                ; preds = %sout.false93, %sout.true92
  br label %sout.merge86

cond.merge101:                                    ; preds = %cond.next.2, %sout.merge277, %sout.merge236, %sout.merge151
  ret i32 0

arith.float:                                      ; preds = %cond.merge13
  %payload.ptr109 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload110 = load ptr, ptr %payload.ptr109, align 8
  %payload.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 1
  %payload112 = load ptr, ptr %payload.ptr111, align 8
  %left.bits = ptrtoint ptr %payload110 to i64
  %right.bits = ptrtoint ptr %payload112 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val113 = alloca %RCCValue, align 8
  %tag.ptr114 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val113, i32 0, i32 0
  store i64 2, ptr %tag.ptr114, align 4
  %payload.ptr115 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val113, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr115, align 8
  store ptr %rcc.val113, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %cond.merge13
  %payload.ptr116 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload117 = load ptr, ptr %payload.ptr116, align 8
  %payload.ptr118 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 1
  %payload119 = load ptr, ptr %payload.ptr118, align 8
  %left.int = ptrtoint ptr %payload117 to i64
  %right.int = ptrtoint ptr %payload119 to i64
  %cmp.gt = icmp sgt i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.gt to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val120 = alloca %RCCValue, align 8
  %tag.ptr121 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val120, i32 0, i32 0
  store i64 3, ptr %tag.ptr121, align 4
  %payload.ptr122 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val120, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr122, align 8
  store ptr %rcc.val120, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  %tag.ptr123 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 0
  %tag124 = load i64, ptr %tag.ptr123, align 4
  %payload.ptr125 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 1
  %payload126 = load ptr, ptr %payload.ptr125, align 8
  %is_null127 = icmp eq i64 %tag124, 0
  %is_int128 = icmp eq i64 %tag124, 1
  %is_bool129 = icmp eq i64 %tag124, 3
  %is_bool_or_int130 = or i1 %is_bool129, %is_int128
  %payload.int131 = ptrtoint ptr %payload126 to i64
  %payload_nz132 = icmp ne i64 %payload.int131, 0
  %not_null133 = xor i1 %is_null127, true
  %bool_int_result134 = select i1 %is_bool_or_int130, i1 %payload_nz132, i1 true
  %coerced_bool135 = select i1 %is_null127, i1 false, i1 %bool_int_result134
  br i1 %coerced_bool135, label %cond.then.0136, label %cond.next.0137

cond.then.0136:                                   ; preds = %arith.merge
  %rcc.val138 = alloca %RCCValue, align 8
  %tag.ptr139 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val138, i32 0, i32 0
  store i64 4, ptr %tag.ptr139, align 4
  %payload.ptr140 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val138, i32 0, i32 1
  store ptr @.str.33, ptr %payload.ptr140, align 8
  %tag.ptr141 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val138, i32 0, i32 0
  %tag142 = load i64, ptr %tag.ptr141, align 4
  %payload.ptr143 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val138, i32 0, i32 1
  %payload144 = load ptr, ptr %payload.ptr143, align 8
  switch i64 %tag142, label %sout.default150 [
    i64 1, label %sout.int145
    i64 2, label %sout.float146
    i64 3, label %sout.bool147
    i64 4, label %sout.str148
    i64 0, label %sout.null149
  ]

cond.next.0137:                                   ; preds = %arith.merge
  %x.load163 = load ptr, ptr %x, align 8
  %rcc.val164 = alloca %RCCValue, align 8
  %tag.ptr165 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val164, i32 0, i32 0
  store i64 1, ptr %tag.ptr165, align 4
  %payload.ptr166 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val164, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr166, align 8
  %tag.ptr167 = getelementptr inbounds nuw %RCCValue, ptr %x.load163, i32 0, i32 0
  %tag168 = load i64, ptr %tag.ptr167, align 4
  %tag.ptr169 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val164, i32 0, i32 0
  %tag170 = load i64, ptr %tag.ptr169, align 4
  %is_float.left171 = icmp eq i64 %tag168, 2
  %is_float.right172 = icmp eq i64 %tag170, 2
  %is_float173 = or i1 %is_float.left171, %is_float.right172
  store ptr null, ptr %arith.result177, align 8
  br i1 %is_float173, label %arith.float174, label %arith.int175

sout.int145:                                      ; preds = %cond.then.0136
  %int.val152 = ptrtoint ptr %payload144 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.34, i64 %int.val152)
  br label %sout.merge151

sout.float146:                                    ; preds = %cond.then.0136
  %float.bits153 = ptrtoint ptr %payload144 to i64
  %float.val154 = bitcast i64 %float.bits153 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.35, double %float.val154)
  br label %sout.merge151

sout.bool147:                                     ; preds = %cond.then.0136
  %bool.val155 = ptrtoint ptr %payload144 to i64
  %is.true156 = icmp ne i64 %bool.val155, 0
  br i1 %is.true156, label %sout.true157, label %sout.false158

sout.str148:                                      ; preds = %cond.then.0136
  %28 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload144)
  br label %sout.merge151

sout.null149:                                     ; preds = %cond.then.0136
  %29 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr @.str.42)
  br label %sout.merge151

sout.default150:                                  ; preds = %cond.then.0136
  %30 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr %payload144)
  br label %sout.merge151

sout.merge151:                                    ; preds = %sout.default150, %sout.null149, %sout.str148, %sout.bool.merge159, %sout.float146, %sout.int145
  %31 = call i32 @putchar(i32 10)
  %rcc.val160 = alloca %RCCValue, align 8
  %tag.ptr161 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val160, i32 0, i32 0
  store i64 0, ptr %tag.ptr161, align 4
  %payload.ptr162 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val160, i32 0, i32 1
  store ptr null, ptr %payload.ptr162, align 8
  br label %cond.merge101

sout.true157:                                     ; preds = %sout.bool147
  %32 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.bool.merge159

sout.false158:                                    ; preds = %sout.bool147
  %33 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr @.str.39)
  br label %sout.bool.merge159

sout.bool.merge159:                               ; preds = %sout.false158, %sout.true157
  br label %sout.merge151

arith.float174:                                   ; preds = %cond.next.0137
  %payload.ptr178 = getelementptr inbounds nuw %RCCValue, ptr %x.load163, i32 0, i32 1
  %payload179 = load ptr, ptr %payload.ptr178, align 8
  %payload.ptr180 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val164, i32 0, i32 1
  %payload181 = load ptr, ptr %payload.ptr180, align 8
  %left.bits182 = ptrtoint ptr %payload179 to i64
  %right.bits183 = ptrtoint ptr %payload181 to i64
  %left.sitofp184 = sitofp i64 %left.bits182 to double
  %left.bitcast185 = bitcast i64 %left.bits182 to double
  %left.double186 = select i1 %is_float.left171, double %left.bitcast185, double %left.sitofp184
  %right.sitofp187 = sitofp i64 %right.bits183 to double
  %right.bitcast188 = bitcast i64 %right.bits183 to double
  %right.double189 = select i1 %is_float.right172, double %right.bitcast188, double %right.sitofp187
  %result.bits190 = bitcast double %left.double186 to i64
  %result.float.ptr191 = inttoptr i64 %result.bits190 to ptr
  %rcc.val192 = alloca %RCCValue, align 8
  %tag.ptr193 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val192, i32 0, i32 0
  store i64 2, ptr %tag.ptr193, align 4
  %payload.ptr194 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val192, i32 0, i32 1
  store ptr %result.float.ptr191, ptr %payload.ptr194, align 8
  store ptr %rcc.val192, ptr %arith.result177, align 8
  br label %arith.merge176

arith.int175:                                     ; preds = %cond.next.0137
  %payload.ptr195 = getelementptr inbounds nuw %RCCValue, ptr %x.load163, i32 0, i32 1
  %payload196 = load ptr, ptr %payload.ptr195, align 8
  %payload.ptr197 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val164, i32 0, i32 1
  %payload198 = load ptr, ptr %payload.ptr197, align 8
  %left.int199 = ptrtoint ptr %payload196 to i64
  %right.int200 = ptrtoint ptr %payload198 to i64
  %cmp.gt201 = icmp sgt i64 %left.int199, %right.int200
  %cmp.ext202 = zext i1 %cmp.gt201 to i64
  %int.ptr203 = inttoptr i64 %cmp.ext202 to ptr
  %rcc.val204 = alloca %RCCValue, align 8
  %tag.ptr205 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val204, i32 0, i32 0
  store i64 3, ptr %tag.ptr205, align 4
  %payload.ptr206 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val204, i32 0, i32 1
  store ptr %int.ptr203, ptr %payload.ptr206, align 8
  store ptr %rcc.val204, ptr %arith.result177, align 8
  br label %arith.merge176

arith.merge176:                                   ; preds = %arith.int175, %arith.float174
  %arith.result.load207 = load ptr, ptr %arith.result177, align 8
  %tag.ptr208 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load207, i32 0, i32 0
  %tag209 = load i64, ptr %tag.ptr208, align 4
  %payload.ptr210 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load207, i32 0, i32 1
  %payload211 = load ptr, ptr %payload.ptr210, align 8
  %is_null212 = icmp eq i64 %tag209, 0
  %is_int213 = icmp eq i64 %tag209, 1
  %is_bool214 = icmp eq i64 %tag209, 3
  %is_bool_or_int215 = or i1 %is_bool214, %is_int213
  %payload.int216 = ptrtoint ptr %payload211 to i64
  %payload_nz217 = icmp ne i64 %payload.int216, 0
  %not_null218 = xor i1 %is_null212, true
  %bool_int_result219 = select i1 %is_bool_or_int215, i1 %payload_nz217, i1 true
  %coerced_bool220 = select i1 %is_null212, i1 false, i1 %bool_int_result219
  br i1 %coerced_bool220, label %cond.then.1221, label %cond.next.1222

cond.then.1221:                                   ; preds = %arith.merge176
  %rcc.val223 = alloca %RCCValue, align 8
  %tag.ptr224 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val223, i32 0, i32 0
  store i64 4, ptr %tag.ptr224, align 4
  %payload.ptr225 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val223, i32 0, i32 1
  store ptr @.str.44, ptr %payload.ptr225, align 8
  %tag.ptr226 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val223, i32 0, i32 0
  %tag227 = load i64, ptr %tag.ptr226, align 4
  %payload.ptr228 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val223, i32 0, i32 1
  %payload229 = load ptr, ptr %payload.ptr228, align 8
  switch i64 %tag227, label %sout.default235 [
    i64 1, label %sout.int230
    i64 2, label %sout.float231
    i64 3, label %sout.bool232
    i64 4, label %sout.str233
    i64 0, label %sout.null234
  ]

cond.next.1222:                                   ; preds = %arith.merge176
  %rcc.val248 = alloca %RCCValue, align 8
  %tag.ptr249 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val248, i32 0, i32 0
  store i64 3, ptr %tag.ptr249, align 4
  %payload.ptr250 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val248, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr250, align 8
  %tag.ptr251 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val248, i32 0, i32 0
  %tag252 = load i64, ptr %tag.ptr251, align 4
  %payload.ptr253 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val248, i32 0, i32 1
  %payload254 = load ptr, ptr %payload.ptr253, align 8
  %is_null255 = icmp eq i64 %tag252, 0
  %is_int256 = icmp eq i64 %tag252, 1
  %is_bool257 = icmp eq i64 %tag252, 3
  %is_bool_or_int258 = or i1 %is_bool257, %is_int256
  %payload.int259 = ptrtoint ptr %payload254 to i64
  %payload_nz260 = icmp ne i64 %payload.int259, 0
  %not_null261 = xor i1 %is_null255, true
  %bool_int_result262 = select i1 %is_bool_or_int258, i1 %payload_nz260, i1 true
  %coerced_bool263 = select i1 %is_null255, i1 false, i1 %bool_int_result262
  br i1 %coerced_bool263, label %cond.then.2, label %cond.next.2

sout.int230:                                      ; preds = %cond.then.1221
  %int.val237 = ptrtoint ptr %payload229 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.45, i64 %int.val237)
  br label %sout.merge236

sout.float231:                                    ; preds = %cond.then.1221
  %float.bits238 = ptrtoint ptr %payload229 to i64
  %float.val239 = bitcast i64 %float.bits238 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.46, double %float.val239)
  br label %sout.merge236

sout.bool232:                                     ; preds = %cond.then.1221
  %bool.val240 = ptrtoint ptr %payload229 to i64
  %is.true241 = icmp ne i64 %bool.val240, 0
  br i1 %is.true241, label %sout.true242, label %sout.false243

sout.str233:                                      ; preds = %cond.then.1221
  %36 = call i32 (ptr, ...) @printf(ptr @.str.51, ptr %payload229)
  br label %sout.merge236

sout.null234:                                     ; preds = %cond.then.1221
  %37 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.merge236

sout.default235:                                  ; preds = %cond.then.1221
  %38 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr %payload229)
  br label %sout.merge236

sout.merge236:                                    ; preds = %sout.default235, %sout.null234, %sout.str233, %sout.bool.merge244, %sout.float231, %sout.int230
  %39 = call i32 @putchar(i32 10)
  %rcc.val245 = alloca %RCCValue, align 8
  %tag.ptr246 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val245, i32 0, i32 0
  store i64 0, ptr %tag.ptr246, align 4
  %payload.ptr247 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val245, i32 0, i32 1
  store ptr null, ptr %payload.ptr247, align 8
  br label %cond.merge101

sout.true242:                                     ; preds = %sout.bool232
  %40 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.bool.merge244

sout.false243:                                    ; preds = %sout.bool232
  %41 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr @.str.50)
  br label %sout.bool.merge244

sout.bool.merge244:                               ; preds = %sout.false243, %sout.true242
  br label %sout.merge236

cond.then.2:                                      ; preds = %cond.next.1222
  %rcc.val264 = alloca %RCCValue, align 8
  %tag.ptr265 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val264, i32 0, i32 0
  store i64 4, ptr %tag.ptr265, align 4
  %payload.ptr266 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val264, i32 0, i32 1
  store ptr @.str.55, ptr %payload.ptr266, align 8
  %tag.ptr267 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val264, i32 0, i32 0
  %tag268 = load i64, ptr %tag.ptr267, align 4
  %payload.ptr269 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val264, i32 0, i32 1
  %payload270 = load ptr, ptr %payload.ptr269, align 8
  switch i64 %tag268, label %sout.default276 [
    i64 1, label %sout.int271
    i64 2, label %sout.float272
    i64 3, label %sout.bool273
    i64 4, label %sout.str274
    i64 0, label %sout.null275
  ]

cond.next.2:                                      ; preds = %cond.next.1222
  br label %cond.merge101

sout.int271:                                      ; preds = %cond.then.2
  %int.val278 = ptrtoint ptr %payload270 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.56, i64 %int.val278)
  br label %sout.merge277

sout.float272:                                    ; preds = %cond.then.2
  %float.bits279 = ptrtoint ptr %payload270 to i64
  %float.val280 = bitcast i64 %float.bits279 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.57, double %float.val280)
  br label %sout.merge277

sout.bool273:                                     ; preds = %cond.then.2
  %bool.val281 = ptrtoint ptr %payload270 to i64
  %is.true282 = icmp ne i64 %bool.val281, 0
  br i1 %is.true282, label %sout.true283, label %sout.false284

sout.str274:                                      ; preds = %cond.then.2
  %44 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr %payload270)
  br label %sout.merge277

sout.null275:                                     ; preds = %cond.then.2
  %45 = call i32 (ptr, ...) @printf(ptr @.str.63, ptr @.str.64)
  br label %sout.merge277

sout.default276:                                  ; preds = %cond.then.2
  %46 = call i32 (ptr, ...) @printf(ptr @.str.65, ptr %payload270)
  br label %sout.merge277

sout.merge277:                                    ; preds = %sout.default276, %sout.null275, %sout.str274, %sout.bool.merge285, %sout.float272, %sout.int271
  %47 = call i32 @putchar(i32 10)
  %rcc.val286 = alloca %RCCValue, align 8
  %tag.ptr287 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 0
  store i64 0, ptr %tag.ptr287, align 4
  %payload.ptr288 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 1
  store ptr null, ptr %payload.ptr288, align 8
  br label %cond.merge101

sout.true283:                                     ; preds = %sout.bool273
  %48 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.bool.merge285

sout.false284:                                    ; preds = %sout.bool273
  %49 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr @.str.61)
  br label %sout.bool.merge285

sout.bool.merge285:                               ; preds = %sout.false284, %sout.true283
  br label %sout.merge277
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
