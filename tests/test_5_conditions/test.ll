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
  %payload.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload106 = load ptr, ptr %payload.ptr105, align 8
  %payload.ptr107 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 1
  %payload108 = load ptr, ptr %payload.ptr107, align 8
  %left.int = ptrtoint ptr %payload106 to i64
  %right.int = ptrtoint ptr %payload108 to i64
  %cmp.gt = icmp sgt i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.gt to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val109 = alloca %RCCValue, align 8
  %tag.ptr110 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val109, i32 0, i32 0
  store i64 3, ptr %tag.ptr110, align 4
  %payload.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val109, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr111, align 8
  %tag.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val109, i32 0, i32 0
  %tag113 = load i64, ptr %tag.ptr112, align 4
  %payload.ptr114 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val109, i32 0, i32 1
  %payload115 = load ptr, ptr %payload.ptr114, align 8
  %is_null116 = icmp eq i64 %tag113, 0
  %is_int117 = icmp eq i64 %tag113, 1
  %is_bool118 = icmp eq i64 %tag113, 3
  %is_bool_or_int119 = or i1 %is_bool118, %is_int117
  %payload.int120 = ptrtoint ptr %payload115 to i64
  %payload_nz121 = icmp ne i64 %payload.int120, 0
  %not_null122 = xor i1 %is_null116, true
  %bool_int_result123 = select i1 %is_bool_or_int119, i1 %payload_nz121, i1 true
  %coerced_bool124 = select i1 %is_null116, i1 false, i1 %bool_int_result123
  br i1 %coerced_bool124, label %cond.then.0125, label %cond.next.0126

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

cond.merge101:                                    ; preds = %cond.next.2, %sout.merge237, %sout.merge196, %sout.merge140
  ret i32 0

cond.then.0125:                                   ; preds = %cond.merge13
  %rcc.val127 = alloca %RCCValue, align 8
  %tag.ptr128 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val127, i32 0, i32 0
  store i64 4, ptr %tag.ptr128, align 4
  %payload.ptr129 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val127, i32 0, i32 1
  store ptr @.str.33, ptr %payload.ptr129, align 8
  %tag.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val127, i32 0, i32 0
  %tag131 = load i64, ptr %tag.ptr130, align 4
  %payload.ptr132 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val127, i32 0, i32 1
  %payload133 = load ptr, ptr %payload.ptr132, align 8
  switch i64 %tag131, label %sout.default139 [
    i64 1, label %sout.int134
    i64 2, label %sout.float135
    i64 3, label %sout.bool136
    i64 4, label %sout.str137
    i64 0, label %sout.null138
  ]

cond.next.0126:                                   ; preds = %cond.merge13
  %x.load152 = load ptr, ptr %x, align 8
  %rcc.val153 = alloca %RCCValue, align 8
  %tag.ptr154 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val153, i32 0, i32 0
  store i64 1, ptr %tag.ptr154, align 4
  %payload.ptr155 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val153, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr155, align 8
  %payload.ptr156 = getelementptr inbounds nuw %RCCValue, ptr %x.load152, i32 0, i32 1
  %payload157 = load ptr, ptr %payload.ptr156, align 8
  %payload.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val153, i32 0, i32 1
  %payload159 = load ptr, ptr %payload.ptr158, align 8
  %left.int160 = ptrtoint ptr %payload157 to i64
  %right.int161 = ptrtoint ptr %payload159 to i64
  %cmp.gt162 = icmp sgt i64 %left.int160, %right.int161
  %cmp.ext163 = zext i1 %cmp.gt162 to i64
  %int.ptr164 = inttoptr i64 %cmp.ext163 to ptr
  %rcc.val165 = alloca %RCCValue, align 8
  %tag.ptr166 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val165, i32 0, i32 0
  store i64 3, ptr %tag.ptr166, align 4
  %payload.ptr167 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val165, i32 0, i32 1
  store ptr %int.ptr164, ptr %payload.ptr167, align 8
  %tag.ptr168 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val165, i32 0, i32 0
  %tag169 = load i64, ptr %tag.ptr168, align 4
  %payload.ptr170 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val165, i32 0, i32 1
  %payload171 = load ptr, ptr %payload.ptr170, align 8
  %is_null172 = icmp eq i64 %tag169, 0
  %is_int173 = icmp eq i64 %tag169, 1
  %is_bool174 = icmp eq i64 %tag169, 3
  %is_bool_or_int175 = or i1 %is_bool174, %is_int173
  %payload.int176 = ptrtoint ptr %payload171 to i64
  %payload_nz177 = icmp ne i64 %payload.int176, 0
  %not_null178 = xor i1 %is_null172, true
  %bool_int_result179 = select i1 %is_bool_or_int175, i1 %payload_nz177, i1 true
  %coerced_bool180 = select i1 %is_null172, i1 false, i1 %bool_int_result179
  br i1 %coerced_bool180, label %cond.then.1181, label %cond.next.1182

sout.int134:                                      ; preds = %cond.then.0125
  %int.val141 = ptrtoint ptr %payload133 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.34, i64 %int.val141)
  br label %sout.merge140

sout.float135:                                    ; preds = %cond.then.0125
  %float.bits142 = ptrtoint ptr %payload133 to i64
  %float.val143 = bitcast i64 %float.bits142 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.35, double %float.val143)
  br label %sout.merge140

sout.bool136:                                     ; preds = %cond.then.0125
  %bool.val144 = ptrtoint ptr %payload133 to i64
  %is.true145 = icmp ne i64 %bool.val144, 0
  br i1 %is.true145, label %sout.true146, label %sout.false147

sout.str137:                                      ; preds = %cond.then.0125
  %28 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload133)
  br label %sout.merge140

sout.null138:                                     ; preds = %cond.then.0125
  %29 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr @.str.42)
  br label %sout.merge140

sout.default139:                                  ; preds = %cond.then.0125
  %30 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr %payload133)
  br label %sout.merge140

sout.merge140:                                    ; preds = %sout.default139, %sout.null138, %sout.str137, %sout.bool.merge148, %sout.float135, %sout.int134
  %31 = call i32 @putchar(i32 10)
  %rcc.val149 = alloca %RCCValue, align 8
  %tag.ptr150 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val149, i32 0, i32 0
  store i64 0, ptr %tag.ptr150, align 4
  %payload.ptr151 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val149, i32 0, i32 1
  store ptr null, ptr %payload.ptr151, align 8
  br label %cond.merge101

sout.true146:                                     ; preds = %sout.bool136
  %32 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.bool.merge148

sout.false147:                                    ; preds = %sout.bool136
  %33 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr @.str.39)
  br label %sout.bool.merge148

sout.bool.merge148:                               ; preds = %sout.false147, %sout.true146
  br label %sout.merge140

cond.then.1181:                                   ; preds = %cond.next.0126
  %rcc.val183 = alloca %RCCValue, align 8
  %tag.ptr184 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val183, i32 0, i32 0
  store i64 4, ptr %tag.ptr184, align 4
  %payload.ptr185 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val183, i32 0, i32 1
  store ptr @.str.44, ptr %payload.ptr185, align 8
  %tag.ptr186 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val183, i32 0, i32 0
  %tag187 = load i64, ptr %tag.ptr186, align 4
  %payload.ptr188 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val183, i32 0, i32 1
  %payload189 = load ptr, ptr %payload.ptr188, align 8
  switch i64 %tag187, label %sout.default195 [
    i64 1, label %sout.int190
    i64 2, label %sout.float191
    i64 3, label %sout.bool192
    i64 4, label %sout.str193
    i64 0, label %sout.null194
  ]

cond.next.1182:                                   ; preds = %cond.next.0126
  %rcc.val208 = alloca %RCCValue, align 8
  %tag.ptr209 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val208, i32 0, i32 0
  store i64 3, ptr %tag.ptr209, align 4
  %payload.ptr210 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val208, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr210, align 8
  %tag.ptr211 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val208, i32 0, i32 0
  %tag212 = load i64, ptr %tag.ptr211, align 4
  %payload.ptr213 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val208, i32 0, i32 1
  %payload214 = load ptr, ptr %payload.ptr213, align 8
  %is_null215 = icmp eq i64 %tag212, 0
  %is_int216 = icmp eq i64 %tag212, 1
  %is_bool217 = icmp eq i64 %tag212, 3
  %is_bool_or_int218 = or i1 %is_bool217, %is_int216
  %payload.int219 = ptrtoint ptr %payload214 to i64
  %payload_nz220 = icmp ne i64 %payload.int219, 0
  %not_null221 = xor i1 %is_null215, true
  %bool_int_result222 = select i1 %is_bool_or_int218, i1 %payload_nz220, i1 true
  %coerced_bool223 = select i1 %is_null215, i1 false, i1 %bool_int_result222
  br i1 %coerced_bool223, label %cond.then.2, label %cond.next.2

sout.int190:                                      ; preds = %cond.then.1181
  %int.val197 = ptrtoint ptr %payload189 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.45, i64 %int.val197)
  br label %sout.merge196

sout.float191:                                    ; preds = %cond.then.1181
  %float.bits198 = ptrtoint ptr %payload189 to i64
  %float.val199 = bitcast i64 %float.bits198 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.46, double %float.val199)
  br label %sout.merge196

sout.bool192:                                     ; preds = %cond.then.1181
  %bool.val200 = ptrtoint ptr %payload189 to i64
  %is.true201 = icmp ne i64 %bool.val200, 0
  br i1 %is.true201, label %sout.true202, label %sout.false203

sout.str193:                                      ; preds = %cond.then.1181
  %36 = call i32 (ptr, ...) @printf(ptr @.str.51, ptr %payload189)
  br label %sout.merge196

sout.null194:                                     ; preds = %cond.then.1181
  %37 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.merge196

sout.default195:                                  ; preds = %cond.then.1181
  %38 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr %payload189)
  br label %sout.merge196

sout.merge196:                                    ; preds = %sout.default195, %sout.null194, %sout.str193, %sout.bool.merge204, %sout.float191, %sout.int190
  %39 = call i32 @putchar(i32 10)
  %rcc.val205 = alloca %RCCValue, align 8
  %tag.ptr206 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val205, i32 0, i32 0
  store i64 0, ptr %tag.ptr206, align 4
  %payload.ptr207 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val205, i32 0, i32 1
  store ptr null, ptr %payload.ptr207, align 8
  br label %cond.merge101

sout.true202:                                     ; preds = %sout.bool192
  %40 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.bool.merge204

sout.false203:                                    ; preds = %sout.bool192
  %41 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr @.str.50)
  br label %sout.bool.merge204

sout.bool.merge204:                               ; preds = %sout.false203, %sout.true202
  br label %sout.merge196

cond.then.2:                                      ; preds = %cond.next.1182
  %rcc.val224 = alloca %RCCValue, align 8
  %tag.ptr225 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val224, i32 0, i32 0
  store i64 4, ptr %tag.ptr225, align 4
  %payload.ptr226 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val224, i32 0, i32 1
  store ptr @.str.55, ptr %payload.ptr226, align 8
  %tag.ptr227 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val224, i32 0, i32 0
  %tag228 = load i64, ptr %tag.ptr227, align 4
  %payload.ptr229 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val224, i32 0, i32 1
  %payload230 = load ptr, ptr %payload.ptr229, align 8
  switch i64 %tag228, label %sout.default236 [
    i64 1, label %sout.int231
    i64 2, label %sout.float232
    i64 3, label %sout.bool233
    i64 4, label %sout.str234
    i64 0, label %sout.null235
  ]

cond.next.2:                                      ; preds = %cond.next.1182
  br label %cond.merge101

sout.int231:                                      ; preds = %cond.then.2
  %int.val238 = ptrtoint ptr %payload230 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.56, i64 %int.val238)
  br label %sout.merge237

sout.float232:                                    ; preds = %cond.then.2
  %float.bits239 = ptrtoint ptr %payload230 to i64
  %float.val240 = bitcast i64 %float.bits239 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.57, double %float.val240)
  br label %sout.merge237

sout.bool233:                                     ; preds = %cond.then.2
  %bool.val241 = ptrtoint ptr %payload230 to i64
  %is.true242 = icmp ne i64 %bool.val241, 0
  br i1 %is.true242, label %sout.true243, label %sout.false244

sout.str234:                                      ; preds = %cond.then.2
  %44 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr %payload230)
  br label %sout.merge237

sout.null235:                                     ; preds = %cond.then.2
  %45 = call i32 (ptr, ...) @printf(ptr @.str.63, ptr @.str.64)
  br label %sout.merge237

sout.default236:                                  ; preds = %cond.then.2
  %46 = call i32 (ptr, ...) @printf(ptr @.str.65, ptr %payload230)
  br label %sout.merge237

sout.merge237:                                    ; preds = %sout.default236, %sout.null235, %sout.str234, %sout.bool.merge245, %sout.float232, %sout.int231
  %47 = call i32 @putchar(i32 10)
  %rcc.val246 = alloca %RCCValue, align 8
  %tag.ptr247 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val246, i32 0, i32 0
  store i64 0, ptr %tag.ptr247, align 4
  %payload.ptr248 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val246, i32 0, i32 1
  store ptr null, ptr %payload.ptr248, align 8
  br label %cond.merge101

sout.true243:                                     ; preds = %sout.bool233
  %48 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.bool.merge245

sout.false244:                                    ; preds = %sout.bool233
  %49 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr @.str.61)
  br label %sout.bool.merge245

sout.bool.merge245:                               ; preds = %sout.false244, %sout.true243
  br label %sout.merge237
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
