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
  %2 = call i32 (ptr, ...) @printf(ptr @.str.1, ptr %payload9)
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

cond.merge13:                                     ; preds = %cond.next.1, %sout.merge85, %sout.merge45
  store ptr null, ptr %x, align 8
  %rcc.val96 = alloca %RCCValue, align 8
  %tag.ptr97 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val96, i32 0, i32 0
  store i64 1, ptr %tag.ptr97, align 4
  %payload.ptr98 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val96, i32 0, i32 1
  store ptr inttoptr (i64 15 to ptr), ptr %payload.ptr98, align 8
  store ptr %rcc.val96, ptr %x, align 8
  %x.load = load ptr, ptr %x, align 8
  %rcc.val100 = alloca %RCCValue, align 8
  %tag.ptr101 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val100, i32 0, i32 0
  store i64 1, ptr %tag.ptr101, align 4
  %payload.ptr102 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val100, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr102, align 8
  %payload.ptr103 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload104 = load ptr, ptr %payload.ptr103, align 8
  %payload.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val100, i32 0, i32 1
  %payload106 = load ptr, ptr %payload.ptr105, align 8
  %left.int = ptrtoint ptr %payload104 to i64
  %right.int = ptrtoint ptr %payload106 to i64
  %cmp.gt = icmp sgt i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.gt to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val107 = alloca %RCCValue, align 8
  %tag.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val107, i32 0, i32 0
  store i64 1, ptr %tag.ptr108, align 4
  %payload.ptr109 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val107, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr109, align 8
  %tag.ptr110 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val107, i32 0, i32 0
  %tag111 = load i64, ptr %tag.ptr110, align 4
  %payload.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val107, i32 0, i32 1
  %payload113 = load ptr, ptr %payload.ptr112, align 8
  %is_null114 = icmp eq i64 %tag111, 0
  %is_int115 = icmp eq i64 %tag111, 1
  %is_bool116 = icmp eq i64 %tag111, 3
  %is_bool_or_int117 = or i1 %is_bool116, %is_int115
  %payload.int118 = ptrtoint ptr %payload113 to i64
  %payload_nz119 = icmp ne i64 %payload.int118, 0
  %not_null120 = xor i1 %is_null114, true
  %bool_int_result121 = select i1 %is_bool_or_int117, i1 %payload_nz119, i1 true
  %coerced_bool122 = select i1 %is_null114, i1 false, i1 %bool_int_result121
  br i1 %coerced_bool122, label %cond.then.0123, label %cond.next.0124

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
  %rcc.val56 = alloca %RCCValue, align 8
  %tag.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 0
  store i64 3, ptr %tag.ptr57, align 4
  %payload.ptr58 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr58, align 8
  %tag.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 0
  %tag60 = load i64, ptr %tag.ptr59, align 4
  %payload.ptr61 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 1
  %payload62 = load ptr, ptr %payload.ptr61, align 8
  %is_null63 = icmp eq i64 %tag60, 0
  %is_int64 = icmp eq i64 %tag60, 1
  %is_bool65 = icmp eq i64 %tag60, 3
  %is_bool_or_int66 = or i1 %is_bool65, %is_int64
  %payload.int67 = ptrtoint ptr %payload62 to i64
  %payload_nz68 = icmp ne i64 %payload.int67, 0
  %not_null69 = xor i1 %is_null63, true
  %bool_int_result70 = select i1 %is_bool_or_int66, i1 %payload_nz68, i1 true
  %coerced_bool71 = select i1 %is_null63, i1 false, i1 %bool_int_result70
  br i1 %coerced_bool71, label %cond.then.1, label %cond.next.1

sout.int39:                                       ; preds = %cond.then.030
  %10 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr %payload38)
  br label %sout.merge45

sout.float40:                                     ; preds = %cond.then.030
  %float.bits46 = ptrtoint ptr %payload38 to i64
  %float.val47 = bitcast i64 %float.bits46 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.13, double %float.val47)
  br label %sout.merge45

sout.bool41:                                      ; preds = %cond.then.030
  %bool.val48 = ptrtoint ptr %payload38 to i64
  %is.true49 = icmp ne i64 %bool.val48, 0
  br i1 %is.true49, label %sout.true50, label %sout.false51

sout.str42:                                       ; preds = %cond.then.030
  %12 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr %payload38)
  br label %sout.merge45

sout.null43:                                      ; preds = %cond.then.030
  %13 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.merge45

sout.default44:                                   ; preds = %cond.then.030
  %14 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload38)
  br label %sout.merge45

sout.merge45:                                     ; preds = %sout.default44, %sout.null43, %sout.str42, %sout.bool.merge52, %sout.float40, %sout.int39
  %15 = call i32 @putchar(i32 10)
  %rcc.val53 = alloca %RCCValue, align 8
  %tag.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val53, i32 0, i32 0
  store i64 0, ptr %tag.ptr54, align 4
  %payload.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val53, i32 0, i32 1
  store ptr null, ptr %payload.ptr55, align 8
  br label %cond.merge13

sout.true50:                                      ; preds = %sout.bool41
  %16 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge52

sout.false51:                                     ; preds = %sout.bool41
  %17 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.17)
  br label %sout.bool.merge52

sout.bool.merge52:                                ; preds = %sout.false51, %sout.true50
  br label %sout.merge45

cond.then.1:                                      ; preds = %cond.next.031
  %rcc.val72 = alloca %RCCValue, align 8
  %tag.ptr73 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val72, i32 0, i32 0
  store i64 4, ptr %tag.ptr73, align 4
  %payload.ptr74 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val72, i32 0, i32 1
  store ptr @.str.22, ptr %payload.ptr74, align 8
  %tag.ptr75 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val72, i32 0, i32 0
  %tag76 = load i64, ptr %tag.ptr75, align 4
  %payload.ptr77 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val72, i32 0, i32 1
  %payload78 = load ptr, ptr %payload.ptr77, align 8
  switch i64 %tag76, label %sout.default84 [
    i64 1, label %sout.int79
    i64 2, label %sout.float80
    i64 3, label %sout.bool81
    i64 4, label %sout.str82
    i64 0, label %sout.null83
  ]

cond.next.1:                                      ; preds = %cond.next.031
  br label %cond.merge13

sout.int79:                                       ; preds = %cond.then.1
  %18 = call i32 (ptr, ...) @printf(ptr @.str.23, ptr %payload78)
  br label %sout.merge85

sout.float80:                                     ; preds = %cond.then.1
  %float.bits86 = ptrtoint ptr %payload78 to i64
  %float.val87 = bitcast i64 %float.bits86 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.24, double %float.val87)
  br label %sout.merge85

sout.bool81:                                      ; preds = %cond.then.1
  %bool.val88 = ptrtoint ptr %payload78 to i64
  %is.true89 = icmp ne i64 %bool.val88, 0
  br i1 %is.true89, label %sout.true90, label %sout.false91

sout.str82:                                       ; preds = %cond.then.1
  %20 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload78)
  br label %sout.merge85

sout.null83:                                      ; preds = %cond.then.1
  %21 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr @.str.31)
  br label %sout.merge85

sout.default84:                                   ; preds = %cond.then.1
  %22 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr %payload78)
  br label %sout.merge85

sout.merge85:                                     ; preds = %sout.default84, %sout.null83, %sout.str82, %sout.bool.merge92, %sout.float80, %sout.int79
  %23 = call i32 @putchar(i32 10)
  %rcc.val93 = alloca %RCCValue, align 8
  %tag.ptr94 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val93, i32 0, i32 0
  store i64 0, ptr %tag.ptr94, align 4
  %payload.ptr95 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val93, i32 0, i32 1
  store ptr null, ptr %payload.ptr95, align 8
  br label %cond.merge13

sout.true90:                                      ; preds = %sout.bool81
  %24 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge92

sout.false91:                                     ; preds = %sout.bool81
  %25 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.bool.merge92

sout.bool.merge92:                                ; preds = %sout.false91, %sout.true90
  br label %sout.merge85

cond.merge99:                                     ; preds = %cond.next.2, %sout.merge233, %sout.merge193, %sout.merge138
  ret i32 0

cond.then.0123:                                   ; preds = %cond.merge13
  %rcc.val125 = alloca %RCCValue, align 8
  %tag.ptr126 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val125, i32 0, i32 0
  store i64 4, ptr %tag.ptr126, align 4
  %payload.ptr127 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val125, i32 0, i32 1
  store ptr @.str.33, ptr %payload.ptr127, align 8
  %tag.ptr128 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val125, i32 0, i32 0
  %tag129 = load i64, ptr %tag.ptr128, align 4
  %payload.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val125, i32 0, i32 1
  %payload131 = load ptr, ptr %payload.ptr130, align 8
  switch i64 %tag129, label %sout.default137 [
    i64 1, label %sout.int132
    i64 2, label %sout.float133
    i64 3, label %sout.bool134
    i64 4, label %sout.str135
    i64 0, label %sout.null136
  ]

cond.next.0124:                                   ; preds = %cond.merge13
  %x.load149 = load ptr, ptr %x, align 8
  %rcc.val150 = alloca %RCCValue, align 8
  %tag.ptr151 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val150, i32 0, i32 0
  store i64 1, ptr %tag.ptr151, align 4
  %payload.ptr152 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val150, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr152, align 8
  %payload.ptr153 = getelementptr inbounds nuw %RCCValue, ptr %x.load149, i32 0, i32 1
  %payload154 = load ptr, ptr %payload.ptr153, align 8
  %payload.ptr155 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val150, i32 0, i32 1
  %payload156 = load ptr, ptr %payload.ptr155, align 8
  %left.int157 = ptrtoint ptr %payload154 to i64
  %right.int158 = ptrtoint ptr %payload156 to i64
  %cmp.gt159 = icmp sgt i64 %left.int157, %right.int158
  %cmp.ext160 = zext i1 %cmp.gt159 to i64
  %int.ptr161 = inttoptr i64 %cmp.ext160 to ptr
  %rcc.val162 = alloca %RCCValue, align 8
  %tag.ptr163 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val162, i32 0, i32 0
  store i64 1, ptr %tag.ptr163, align 4
  %payload.ptr164 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val162, i32 0, i32 1
  store ptr %int.ptr161, ptr %payload.ptr164, align 8
  %tag.ptr165 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val162, i32 0, i32 0
  %tag166 = load i64, ptr %tag.ptr165, align 4
  %payload.ptr167 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val162, i32 0, i32 1
  %payload168 = load ptr, ptr %payload.ptr167, align 8
  %is_null169 = icmp eq i64 %tag166, 0
  %is_int170 = icmp eq i64 %tag166, 1
  %is_bool171 = icmp eq i64 %tag166, 3
  %is_bool_or_int172 = or i1 %is_bool171, %is_int170
  %payload.int173 = ptrtoint ptr %payload168 to i64
  %payload_nz174 = icmp ne i64 %payload.int173, 0
  %not_null175 = xor i1 %is_null169, true
  %bool_int_result176 = select i1 %is_bool_or_int172, i1 %payload_nz174, i1 true
  %coerced_bool177 = select i1 %is_null169, i1 false, i1 %bool_int_result176
  br i1 %coerced_bool177, label %cond.then.1178, label %cond.next.1179

sout.int132:                                      ; preds = %cond.then.0123
  %26 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr %payload131)
  br label %sout.merge138

sout.float133:                                    ; preds = %cond.then.0123
  %float.bits139 = ptrtoint ptr %payload131 to i64
  %float.val140 = bitcast i64 %float.bits139 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.35, double %float.val140)
  br label %sout.merge138

sout.bool134:                                     ; preds = %cond.then.0123
  %bool.val141 = ptrtoint ptr %payload131 to i64
  %is.true142 = icmp ne i64 %bool.val141, 0
  br i1 %is.true142, label %sout.true143, label %sout.false144

sout.str135:                                      ; preds = %cond.then.0123
  %28 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload131)
  br label %sout.merge138

sout.null136:                                     ; preds = %cond.then.0123
  %29 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr @.str.42)
  br label %sout.merge138

sout.default137:                                  ; preds = %cond.then.0123
  %30 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr %payload131)
  br label %sout.merge138

sout.merge138:                                    ; preds = %sout.default137, %sout.null136, %sout.str135, %sout.bool.merge145, %sout.float133, %sout.int132
  %31 = call i32 @putchar(i32 10)
  %rcc.val146 = alloca %RCCValue, align 8
  %tag.ptr147 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val146, i32 0, i32 0
  store i64 0, ptr %tag.ptr147, align 4
  %payload.ptr148 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val146, i32 0, i32 1
  store ptr null, ptr %payload.ptr148, align 8
  br label %cond.merge99

sout.true143:                                     ; preds = %sout.bool134
  %32 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.bool.merge145

sout.false144:                                    ; preds = %sout.bool134
  %33 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr @.str.39)
  br label %sout.bool.merge145

sout.bool.merge145:                               ; preds = %sout.false144, %sout.true143
  br label %sout.merge138

cond.then.1178:                                   ; preds = %cond.next.0124
  %rcc.val180 = alloca %RCCValue, align 8
  %tag.ptr181 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val180, i32 0, i32 0
  store i64 4, ptr %tag.ptr181, align 4
  %payload.ptr182 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val180, i32 0, i32 1
  store ptr @.str.44, ptr %payload.ptr182, align 8
  %tag.ptr183 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val180, i32 0, i32 0
  %tag184 = load i64, ptr %tag.ptr183, align 4
  %payload.ptr185 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val180, i32 0, i32 1
  %payload186 = load ptr, ptr %payload.ptr185, align 8
  switch i64 %tag184, label %sout.default192 [
    i64 1, label %sout.int187
    i64 2, label %sout.float188
    i64 3, label %sout.bool189
    i64 4, label %sout.str190
    i64 0, label %sout.null191
  ]

cond.next.1179:                                   ; preds = %cond.next.0124
  %rcc.val204 = alloca %RCCValue, align 8
  %tag.ptr205 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val204, i32 0, i32 0
  store i64 3, ptr %tag.ptr205, align 4
  %payload.ptr206 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val204, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr206, align 8
  %tag.ptr207 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val204, i32 0, i32 0
  %tag208 = load i64, ptr %tag.ptr207, align 4
  %payload.ptr209 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val204, i32 0, i32 1
  %payload210 = load ptr, ptr %payload.ptr209, align 8
  %is_null211 = icmp eq i64 %tag208, 0
  %is_int212 = icmp eq i64 %tag208, 1
  %is_bool213 = icmp eq i64 %tag208, 3
  %is_bool_or_int214 = or i1 %is_bool213, %is_int212
  %payload.int215 = ptrtoint ptr %payload210 to i64
  %payload_nz216 = icmp ne i64 %payload.int215, 0
  %not_null217 = xor i1 %is_null211, true
  %bool_int_result218 = select i1 %is_bool_or_int214, i1 %payload_nz216, i1 true
  %coerced_bool219 = select i1 %is_null211, i1 false, i1 %bool_int_result218
  br i1 %coerced_bool219, label %cond.then.2, label %cond.next.2

sout.int187:                                      ; preds = %cond.then.1178
  %34 = call i32 (ptr, ...) @printf(ptr @.str.45, ptr %payload186)
  br label %sout.merge193

sout.float188:                                    ; preds = %cond.then.1178
  %float.bits194 = ptrtoint ptr %payload186 to i64
  %float.val195 = bitcast i64 %float.bits194 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.46, double %float.val195)
  br label %sout.merge193

sout.bool189:                                     ; preds = %cond.then.1178
  %bool.val196 = ptrtoint ptr %payload186 to i64
  %is.true197 = icmp ne i64 %bool.val196, 0
  br i1 %is.true197, label %sout.true198, label %sout.false199

sout.str190:                                      ; preds = %cond.then.1178
  %36 = call i32 (ptr, ...) @printf(ptr @.str.51, ptr %payload186)
  br label %sout.merge193

sout.null191:                                     ; preds = %cond.then.1178
  %37 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.merge193

sout.default192:                                  ; preds = %cond.then.1178
  %38 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr %payload186)
  br label %sout.merge193

sout.merge193:                                    ; preds = %sout.default192, %sout.null191, %sout.str190, %sout.bool.merge200, %sout.float188, %sout.int187
  %39 = call i32 @putchar(i32 10)
  %rcc.val201 = alloca %RCCValue, align 8
  %tag.ptr202 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val201, i32 0, i32 0
  store i64 0, ptr %tag.ptr202, align 4
  %payload.ptr203 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val201, i32 0, i32 1
  store ptr null, ptr %payload.ptr203, align 8
  br label %cond.merge99

sout.true198:                                     ; preds = %sout.bool189
  %40 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.bool.merge200

sout.false199:                                    ; preds = %sout.bool189
  %41 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr @.str.50)
  br label %sout.bool.merge200

sout.bool.merge200:                               ; preds = %sout.false199, %sout.true198
  br label %sout.merge193

cond.then.2:                                      ; preds = %cond.next.1179
  %rcc.val220 = alloca %RCCValue, align 8
  %tag.ptr221 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 0
  store i64 4, ptr %tag.ptr221, align 4
  %payload.ptr222 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 1
  store ptr @.str.55, ptr %payload.ptr222, align 8
  %tag.ptr223 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 0
  %tag224 = load i64, ptr %tag.ptr223, align 4
  %payload.ptr225 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 1
  %payload226 = load ptr, ptr %payload.ptr225, align 8
  switch i64 %tag224, label %sout.default232 [
    i64 1, label %sout.int227
    i64 2, label %sout.float228
    i64 3, label %sout.bool229
    i64 4, label %sout.str230
    i64 0, label %sout.null231
  ]

cond.next.2:                                      ; preds = %cond.next.1179
  br label %cond.merge99

sout.int227:                                      ; preds = %cond.then.2
  %42 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr %payload226)
  br label %sout.merge233

sout.float228:                                    ; preds = %cond.then.2
  %float.bits234 = ptrtoint ptr %payload226 to i64
  %float.val235 = bitcast i64 %float.bits234 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.57, double %float.val235)
  br label %sout.merge233

sout.bool229:                                     ; preds = %cond.then.2
  %bool.val236 = ptrtoint ptr %payload226 to i64
  %is.true237 = icmp ne i64 %bool.val236, 0
  br i1 %is.true237, label %sout.true238, label %sout.false239

sout.str230:                                      ; preds = %cond.then.2
  %44 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr %payload226)
  br label %sout.merge233

sout.null231:                                     ; preds = %cond.then.2
  %45 = call i32 (ptr, ...) @printf(ptr @.str.63, ptr @.str.64)
  br label %sout.merge233

sout.default232:                                  ; preds = %cond.then.2
  %46 = call i32 (ptr, ...) @printf(ptr @.str.65, ptr %payload226)
  br label %sout.merge233

sout.merge233:                                    ; preds = %sout.default232, %sout.null231, %sout.str230, %sout.bool.merge240, %sout.float228, %sout.int227
  %47 = call i32 @putchar(i32 10)
  %rcc.val241 = alloca %RCCValue, align 8
  %tag.ptr242 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val241, i32 0, i32 0
  store i64 0, ptr %tag.ptr242, align 4
  %payload.ptr243 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val241, i32 0, i32 1
  store ptr null, ptr %payload.ptr243, align 8
  br label %cond.merge99

sout.true238:                                     ; preds = %sout.bool229
  %48 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.bool.merge240

sout.false239:                                    ; preds = %sout.bool229
  %49 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr @.str.61)
  br label %sout.bool.merge240

sout.bool.merge240:                               ; preds = %sout.false239, %sout.true238
  br label %sout.merge233
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
