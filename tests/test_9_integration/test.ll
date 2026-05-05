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
@.str.30 = private constant [5 x i8] c"%lld\00"
@.str.31 = private constant [3 x i8] c"%g\00"
@.str.32 = private constant [3 x i8] c"%s\00"
@.str.33 = private constant [5 x i8] c"true\00"
@.str.34 = private constant [3 x i8] c"%s\00"
@.str.35 = private constant [6 x i8] c"false\00"
@.str.36 = private constant [3 x i8] c"%s\00"
@.str.37 = private constant [3 x i8] c"%s\00"
@.str.38 = private constant [5 x i8] c"null\00"
@.str.39 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.40 = private constant [5 x i8] c"%lld\00"
@.str.41 = private constant [3 x i8] c"%g\00"
@.str.42 = private constant [3 x i8] c"%s\00"
@.str.43 = private constant [5 x i8] c"true\00"
@.str.44 = private constant [3 x i8] c"%s\00"
@.str.45 = private constant [6 x i8] c"false\00"
@.str.46 = private constant [3 x i8] c"%s\00"
@.str.47 = private constant [3 x i8] c"%s\00"
@.str.48 = private constant [5 x i8] c"null\00"
@.str.49 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.50 = private constant [5 x i8] c"%lld\00"
@.str.51 = private constant [3 x i8] c"%g\00"
@.str.52 = private constant [3 x i8] c"%s\00"
@.str.53 = private constant [5 x i8] c"true\00"
@.str.54 = private constant [3 x i8] c"%s\00"
@.str.55 = private constant [6 x i8] c"false\00"
@.str.56 = private constant [3 x i8] c"%s\00"
@.str.57 = private constant [3 x i8] c"%s\00"
@.str.58 = private constant [5 x i8] c"null\00"
@.str.59 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.60 = private constant [5 x i8] c"%lld\00"
@.str.61 = private constant [3 x i8] c"%g\00"
@.str.62 = private constant [3 x i8] c"%s\00"
@.str.63 = private constant [5 x i8] c"true\00"
@.str.64 = private constant [3 x i8] c"%s\00"
@.str.65 = private constant [6 x i8] c"false\00"
@.str.66 = private constant [3 x i8] c"%s\00"
@.str.67 = private constant [3 x i8] c"%s\00"
@.str.68 = private constant [5 x i8] c"null\00"
@.str.69 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.70 = private constant [5 x i8] c"%lld\00"
@.str.71 = private constant [3 x i8] c"%g\00"
@.str.72 = private constant [3 x i8] c"%s\00"
@.str.73 = private constant [5 x i8] c"true\00"
@.str.74 = private constant [3 x i8] c"%s\00"
@.str.75 = private constant [6 x i8] c"false\00"
@.str.76 = private constant [3 x i8] c"%s\00"
@.str.77 = private constant [3 x i8] c"%s\00"
@.str.78 = private constant [5 x i8] c"null\00"
@.str.79 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.80 = private constant [5 x i8] c"%lld\00"
@.str.81 = private constant [3 x i8] c"%g\00"
@.str.82 = private constant [3 x i8] c"%s\00"
@.str.83 = private constant [5 x i8] c"true\00"
@.str.84 = private constant [3 x i8] c"%s\00"
@.str.85 = private constant [6 x i8] c"false\00"
@.str.86 = private constant [3 x i8] c"%s\00"
@.str.87 = private constant [3 x i8] c"%s\00"
@.str.88 = private constant [5 x i8] c"null\00"
@.str.89 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.90 = private constant [5 x i8] c"%lld\00"
@.str.91 = private constant [3 x i8] c"%g\00"
@.str.92 = private constant [3 x i8] c"%s\00"
@.str.93 = private constant [5 x i8] c"true\00"
@.str.94 = private constant [3 x i8] c"%s\00"
@.str.95 = private constant [6 x i8] c"false\00"
@.str.96 = private constant [3 x i8] c"%s\00"
@.str.97 = private constant [3 x i8] c"%s\00"
@.str.98 = private constant [5 x i8] c"null\00"
@.str.99 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.100 = private constant [5 x i8] c"%lld\00"
@.str.101 = private constant [3 x i8] c"%g\00"
@.str.102 = private constant [3 x i8] c"%s\00"
@.str.103 = private constant [5 x i8] c"true\00"
@.str.104 = private constant [3 x i8] c"%s\00"
@.str.105 = private constant [6 x i8] c"false\00"
@.str.106 = private constant [3 x i8] c"%s\00"
@.str.107 = private constant [3 x i8] c"%s\00"
@.str.108 = private constant [5 x i8] c"null\00"
@.str.109 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr null, ptr %payload.ptr, align 8
  %call.fib = call ptr @fib(ptr %rcc.val)
  %tag.ptr1 = getelementptr inbounds nuw %RCCValue, ptr %call.fib, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr1, align 4
  %payload.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %call.fib, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr2, align 8
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
  %rcc.val3 = alloca %RCCValue, align 8
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 0
  store i64 0, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 1
  store ptr null, ptr %payload.ptr5, align 8
  %rcc.val6 = alloca %RCCValue, align 8
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 0
  store i64 1, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr8, align 8
  %call.fib9 = call ptr @fib(ptr %rcc.val6)
  %tag.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %call.fib9, i32 0, i32 0
  %tag11 = load i64, ptr %tag.ptr10, align 4
  %payload.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %call.fib9, i32 0, i32 1
  %payload13 = load ptr, ptr %payload.ptr12, align 8
  switch i64 %tag11, label %sout.default19 [
    i64 1, label %sout.int14
    i64 2, label %sout.float15
    i64 3, label %sout.bool16
    i64 4, label %sout.str17
    i64 0, label %sout.null18
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int14:                                       ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload13)
  br label %sout.merge20

sout.float15:                                     ; preds = %sout.merge
  %float.bits21 = ptrtoint ptr %payload13 to i64
  %float.val22 = bitcast i64 %float.bits21 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val22)
  br label %sout.merge20

sout.bool16:                                      ; preds = %sout.merge
  %bool.val23 = ptrtoint ptr %payload13 to i64
  %is.true24 = icmp ne i64 %bool.val23, 0
  br i1 %is.true24, label %sout.true25, label %sout.false26

sout.str17:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload13)
  br label %sout.merge20

sout.null18:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge20

sout.default19:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload13)
  br label %sout.merge20

sout.merge20:                                     ; preds = %sout.default19, %sout.null18, %sout.str17, %sout.bool.merge27, %sout.float15, %sout.int14
  %15 = call i32 @putchar(i32 10)
  %rcc.val28 = alloca %RCCValue, align 8
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val28, i32 0, i32 0
  store i64 0, ptr %tag.ptr29, align 4
  %payload.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val28, i32 0, i32 1
  store ptr null, ptr %payload.ptr30, align 8
  %rcc.val31 = alloca %RCCValue, align 8
  %tag.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 0
  store i64 1, ptr %tag.ptr32, align 4
  %payload.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr33, align 8
  %call.fib34 = call ptr @fib(ptr %rcc.val31)
  %tag.ptr35 = getelementptr inbounds nuw %RCCValue, ptr %call.fib34, i32 0, i32 0
  %tag36 = load i64, ptr %tag.ptr35, align 4
  %payload.ptr37 = getelementptr inbounds nuw %RCCValue, ptr %call.fib34, i32 0, i32 1
  %payload38 = load ptr, ptr %payload.ptr37, align 8
  switch i64 %tag36, label %sout.default44 [
    i64 1, label %sout.int39
    i64 2, label %sout.float40
    i64 3, label %sout.bool41
    i64 4, label %sout.str42
    i64 0, label %sout.null43
  ]

sout.true25:                                      ; preds = %sout.bool16
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge27

sout.false26:                                     ; preds = %sout.bool16
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge27

sout.bool.merge27:                                ; preds = %sout.false26, %sout.true25
  br label %sout.merge20

sout.int39:                                       ; preds = %sout.merge20
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, ptr %payload38)
  br label %sout.merge45

sout.float40:                                     ; preds = %sout.merge20
  %float.bits46 = ptrtoint ptr %payload38 to i64
  %float.val47 = bitcast i64 %float.bits46 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val47)
  br label %sout.merge45

sout.bool41:                                      ; preds = %sout.merge20
  %bool.val48 = ptrtoint ptr %payload38 to i64
  %is.true49 = icmp ne i64 %bool.val48, 0
  br i1 %is.true49, label %sout.true50, label %sout.false51

sout.str42:                                       ; preds = %sout.merge20
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload38)
  br label %sout.merge45

sout.null43:                                      ; preds = %sout.merge20
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge45

sout.default44:                                   ; preds = %sout.merge20
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload38)
  br label %sout.merge45

sout.merge45:                                     ; preds = %sout.default44, %sout.null43, %sout.str42, %sout.bool.merge52, %sout.float40, %sout.int39
  %23 = call i32 @putchar(i32 10)
  %rcc.val53 = alloca %RCCValue, align 8
  %tag.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val53, i32 0, i32 0
  store i64 0, ptr %tag.ptr54, align 4
  %payload.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val53, i32 0, i32 1
  store ptr null, ptr %payload.ptr55, align 8
  %rcc.val56 = alloca %RCCValue, align 8
  %tag.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 0
  store i64 1, ptr %tag.ptr57, align 4
  %payload.ptr58 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr58, align 8
  %call.fib59 = call ptr @fib(ptr %rcc.val56)
  %tag.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %call.fib59, i32 0, i32 0
  %tag61 = load i64, ptr %tag.ptr60, align 4
  %payload.ptr62 = getelementptr inbounds nuw %RCCValue, ptr %call.fib59, i32 0, i32 1
  %payload63 = load ptr, ptr %payload.ptr62, align 8
  switch i64 %tag61, label %sout.default69 [
    i64 1, label %sout.int64
    i64 2, label %sout.float65
    i64 3, label %sout.bool66
    i64 4, label %sout.str67
    i64 0, label %sout.null68
  ]

sout.true50:                                      ; preds = %sout.bool41
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge52

sout.false51:                                     ; preds = %sout.bool41
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge52

sout.bool.merge52:                                ; preds = %sout.false51, %sout.true50
  br label %sout.merge45

sout.int64:                                       ; preds = %sout.merge45
  %26 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr %payload63)
  br label %sout.merge70

sout.float65:                                     ; preds = %sout.merge45
  %float.bits71 = ptrtoint ptr %payload63 to i64
  %float.val72 = bitcast i64 %float.bits71 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.31, double %float.val72)
  br label %sout.merge70

sout.bool66:                                      ; preds = %sout.merge45
  %bool.val73 = ptrtoint ptr %payload63 to i64
  %is.true74 = icmp ne i64 %bool.val73, 0
  br i1 %is.true74, label %sout.true75, label %sout.false76

sout.str67:                                       ; preds = %sout.merge45
  %28 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr %payload63)
  br label %sout.merge70

sout.null68:                                      ; preds = %sout.merge45
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.merge70

sout.default69:                                   ; preds = %sout.merge45
  %30 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload63)
  br label %sout.merge70

sout.merge70:                                     ; preds = %sout.default69, %sout.null68, %sout.str67, %sout.bool.merge77, %sout.float65, %sout.int64
  %31 = call i32 @putchar(i32 10)
  %rcc.val78 = alloca %RCCValue, align 8
  %tag.ptr79 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 0
  store i64 0, ptr %tag.ptr79, align 4
  %payload.ptr80 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 1
  store ptr null, ptr %payload.ptr80, align 8
  %rcc.val81 = alloca %RCCValue, align 8
  %tag.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 0
  store i64 1, ptr %tag.ptr82, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 1
  store ptr inttoptr (i64 4 to ptr), ptr %payload.ptr83, align 8
  %call.fib84 = call ptr @fib(ptr %rcc.val81)
  %tag.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %call.fib84, i32 0, i32 0
  %tag86 = load i64, ptr %tag.ptr85, align 4
  %payload.ptr87 = getelementptr inbounds nuw %RCCValue, ptr %call.fib84, i32 0, i32 1
  %payload88 = load ptr, ptr %payload.ptr87, align 8
  switch i64 %tag86, label %sout.default94 [
    i64 1, label %sout.int89
    i64 2, label %sout.float90
    i64 3, label %sout.bool91
    i64 4, label %sout.str92
    i64 0, label %sout.null93
  ]

sout.true75:                                      ; preds = %sout.bool66
  %32 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr @.str.33)
  br label %sout.bool.merge77

sout.false76:                                     ; preds = %sout.bool66
  %33 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr @.str.35)
  br label %sout.bool.merge77

sout.bool.merge77:                                ; preds = %sout.false76, %sout.true75
  br label %sout.merge70

sout.int89:                                       ; preds = %sout.merge70
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload88)
  br label %sout.merge95

sout.float90:                                     ; preds = %sout.merge70
  %float.bits96 = ptrtoint ptr %payload88 to i64
  %float.val97 = bitcast i64 %float.bits96 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val97)
  br label %sout.merge95

sout.bool91:                                      ; preds = %sout.merge70
  %bool.val98 = ptrtoint ptr %payload88 to i64
  %is.true99 = icmp ne i64 %bool.val98, 0
  br i1 %is.true99, label %sout.true100, label %sout.false101

sout.str92:                                       ; preds = %sout.merge70
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload88)
  br label %sout.merge95

sout.null93:                                      ; preds = %sout.merge70
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge95

sout.default94:                                   ; preds = %sout.merge70
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload88)
  br label %sout.merge95

sout.merge95:                                     ; preds = %sout.default94, %sout.null93, %sout.str92, %sout.bool.merge102, %sout.float90, %sout.int89
  %39 = call i32 @putchar(i32 10)
  %rcc.val103 = alloca %RCCValue, align 8
  %tag.ptr104 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val103, i32 0, i32 0
  store i64 0, ptr %tag.ptr104, align 4
  %payload.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val103, i32 0, i32 1
  store ptr null, ptr %payload.ptr105, align 8
  %rcc.val106 = alloca %RCCValue, align 8
  %tag.ptr107 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val106, i32 0, i32 0
  store i64 1, ptr %tag.ptr107, align 4
  %payload.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val106, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr108, align 8
  %call.fib109 = call ptr @fib(ptr %rcc.val106)
  %tag.ptr110 = getelementptr inbounds nuw %RCCValue, ptr %call.fib109, i32 0, i32 0
  %tag111 = load i64, ptr %tag.ptr110, align 4
  %payload.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %call.fib109, i32 0, i32 1
  %payload113 = load ptr, ptr %payload.ptr112, align 8
  switch i64 %tag111, label %sout.default119 [
    i64 1, label %sout.int114
    i64 2, label %sout.float115
    i64 3, label %sout.bool116
    i64 4, label %sout.str117
    i64 0, label %sout.null118
  ]

sout.true100:                                     ; preds = %sout.bool91
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge102

sout.false101:                                    ; preds = %sout.bool91
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge102

sout.bool.merge102:                               ; preds = %sout.false101, %sout.true100
  br label %sout.merge95

sout.int114:                                      ; preds = %sout.merge95
  %42 = call i32 (ptr, ...) @printf(ptr @.str.50, ptr %payload113)
  br label %sout.merge120

sout.float115:                                    ; preds = %sout.merge95
  %float.bits121 = ptrtoint ptr %payload113 to i64
  %float.val122 = bitcast i64 %float.bits121 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.51, double %float.val122)
  br label %sout.merge120

sout.bool116:                                     ; preds = %sout.merge95
  %bool.val123 = ptrtoint ptr %payload113 to i64
  %is.true124 = icmp ne i64 %bool.val123, 0
  br i1 %is.true124, label %sout.true125, label %sout.false126

sout.str117:                                      ; preds = %sout.merge95
  %44 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr %payload113)
  br label %sout.merge120

sout.null118:                                     ; preds = %sout.merge95
  %45 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr @.str.58)
  br label %sout.merge120

sout.default119:                                  ; preds = %sout.merge95
  %46 = call i32 (ptr, ...) @printf(ptr @.str.59, ptr %payload113)
  br label %sout.merge120

sout.merge120:                                    ; preds = %sout.default119, %sout.null118, %sout.str117, %sout.bool.merge127, %sout.float115, %sout.int114
  %47 = call i32 @putchar(i32 10)
  %rcc.val128 = alloca %RCCValue, align 8
  %tag.ptr129 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val128, i32 0, i32 0
  store i64 0, ptr %tag.ptr129, align 4
  %payload.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val128, i32 0, i32 1
  store ptr null, ptr %payload.ptr130, align 8
  %rcc.val131 = alloca %RCCValue, align 8
  %tag.ptr132 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val131, i32 0, i32 0
  store i64 1, ptr %tag.ptr132, align 4
  %payload.ptr133 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val131, i32 0, i32 1
  store ptr inttoptr (i64 6 to ptr), ptr %payload.ptr133, align 8
  %call.fib134 = call ptr @fib(ptr %rcc.val131)
  %tag.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %call.fib134, i32 0, i32 0
  %tag136 = load i64, ptr %tag.ptr135, align 4
  %payload.ptr137 = getelementptr inbounds nuw %RCCValue, ptr %call.fib134, i32 0, i32 1
  %payload138 = load ptr, ptr %payload.ptr137, align 8
  switch i64 %tag136, label %sout.default144 [
    i64 1, label %sout.int139
    i64 2, label %sout.float140
    i64 3, label %sout.bool141
    i64 4, label %sout.str142
    i64 0, label %sout.null143
  ]

sout.true125:                                     ; preds = %sout.bool116
  %48 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.bool.merge127

sout.false126:                                    ; preds = %sout.bool116
  %49 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr @.str.55)
  br label %sout.bool.merge127

sout.bool.merge127:                               ; preds = %sout.false126, %sout.true125
  br label %sout.merge120

sout.int139:                                      ; preds = %sout.merge120
  %50 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload138)
  br label %sout.merge145

sout.float140:                                    ; preds = %sout.merge120
  %float.bits146 = ptrtoint ptr %payload138 to i64
  %float.val147 = bitcast i64 %float.bits146 to double
  %51 = call i32 (ptr, ...) @printf(ptr @.str.61, double %float.val147)
  br label %sout.merge145

sout.bool141:                                     ; preds = %sout.merge120
  %bool.val148 = ptrtoint ptr %payload138 to i64
  %is.true149 = icmp ne i64 %bool.val148, 0
  br i1 %is.true149, label %sout.true150, label %sout.false151

sout.str142:                                      ; preds = %sout.merge120
  %52 = call i32 (ptr, ...) @printf(ptr @.str.66, ptr %payload138)
  br label %sout.merge145

sout.null143:                                     ; preds = %sout.merge120
  %53 = call i32 (ptr, ...) @printf(ptr @.str.67, ptr @.str.68)
  br label %sout.merge145

sout.default144:                                  ; preds = %sout.merge120
  %54 = call i32 (ptr, ...) @printf(ptr @.str.69, ptr %payload138)
  br label %sout.merge145

sout.merge145:                                    ; preds = %sout.default144, %sout.null143, %sout.str142, %sout.bool.merge152, %sout.float140, %sout.int139
  %55 = call i32 @putchar(i32 10)
  %rcc.val153 = alloca %RCCValue, align 8
  %tag.ptr154 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val153, i32 0, i32 0
  store i64 0, ptr %tag.ptr154, align 4
  %payload.ptr155 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val153, i32 0, i32 1
  store ptr null, ptr %payload.ptr155, align 8
  %rcc.val156 = alloca %RCCValue, align 8
  %tag.ptr157 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val156, i32 0, i32 0
  store i64 1, ptr %tag.ptr157, align 4
  %payload.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val156, i32 0, i32 1
  store ptr inttoptr (i64 7 to ptr), ptr %payload.ptr158, align 8
  %call.fib159 = call ptr @fib(ptr %rcc.val156)
  %tag.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %call.fib159, i32 0, i32 0
  %tag161 = load i64, ptr %tag.ptr160, align 4
  %payload.ptr162 = getelementptr inbounds nuw %RCCValue, ptr %call.fib159, i32 0, i32 1
  %payload163 = load ptr, ptr %payload.ptr162, align 8
  switch i64 %tag161, label %sout.default169 [
    i64 1, label %sout.int164
    i64 2, label %sout.float165
    i64 3, label %sout.bool166
    i64 4, label %sout.str167
    i64 0, label %sout.null168
  ]

sout.true150:                                     ; preds = %sout.bool141
  %56 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr @.str.63)
  br label %sout.bool.merge152

sout.false151:                                    ; preds = %sout.bool141
  %57 = call i32 (ptr, ...) @printf(ptr @.str.64, ptr @.str.65)
  br label %sout.bool.merge152

sout.bool.merge152:                               ; preds = %sout.false151, %sout.true150
  br label %sout.merge145

sout.int164:                                      ; preds = %sout.merge145
  %58 = call i32 (ptr, ...) @printf(ptr @.str.70, ptr %payload163)
  br label %sout.merge170

sout.float165:                                    ; preds = %sout.merge145
  %float.bits171 = ptrtoint ptr %payload163 to i64
  %float.val172 = bitcast i64 %float.bits171 to double
  %59 = call i32 (ptr, ...) @printf(ptr @.str.71, double %float.val172)
  br label %sout.merge170

sout.bool166:                                     ; preds = %sout.merge145
  %bool.val173 = ptrtoint ptr %payload163 to i64
  %is.true174 = icmp ne i64 %bool.val173, 0
  br i1 %is.true174, label %sout.true175, label %sout.false176

sout.str167:                                      ; preds = %sout.merge145
  %60 = call i32 (ptr, ...) @printf(ptr @.str.76, ptr %payload163)
  br label %sout.merge170

sout.null168:                                     ; preds = %sout.merge145
  %61 = call i32 (ptr, ...) @printf(ptr @.str.77, ptr @.str.78)
  br label %sout.merge170

sout.default169:                                  ; preds = %sout.merge145
  %62 = call i32 (ptr, ...) @printf(ptr @.str.79, ptr %payload163)
  br label %sout.merge170

sout.merge170:                                    ; preds = %sout.default169, %sout.null168, %sout.str167, %sout.bool.merge177, %sout.float165, %sout.int164
  %63 = call i32 @putchar(i32 10)
  %rcc.val178 = alloca %RCCValue, align 8
  %tag.ptr179 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val178, i32 0, i32 0
  store i64 0, ptr %tag.ptr179, align 4
  %payload.ptr180 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val178, i32 0, i32 1
  store ptr null, ptr %payload.ptr180, align 8
  %rcc.val181 = alloca %RCCValue, align 8
  %tag.ptr182 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val181, i32 0, i32 0
  store i64 1, ptr %tag.ptr182, align 4
  %payload.ptr183 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val181, i32 0, i32 1
  store ptr inttoptr (i64 8 to ptr), ptr %payload.ptr183, align 8
  %call.fib184 = call ptr @fib(ptr %rcc.val181)
  %tag.ptr185 = getelementptr inbounds nuw %RCCValue, ptr %call.fib184, i32 0, i32 0
  %tag186 = load i64, ptr %tag.ptr185, align 4
  %payload.ptr187 = getelementptr inbounds nuw %RCCValue, ptr %call.fib184, i32 0, i32 1
  %payload188 = load ptr, ptr %payload.ptr187, align 8
  switch i64 %tag186, label %sout.default194 [
    i64 1, label %sout.int189
    i64 2, label %sout.float190
    i64 3, label %sout.bool191
    i64 4, label %sout.str192
    i64 0, label %sout.null193
  ]

sout.true175:                                     ; preds = %sout.bool166
  %64 = call i32 (ptr, ...) @printf(ptr @.str.72, ptr @.str.73)
  br label %sout.bool.merge177

sout.false176:                                    ; preds = %sout.bool166
  %65 = call i32 (ptr, ...) @printf(ptr @.str.74, ptr @.str.75)
  br label %sout.bool.merge177

sout.bool.merge177:                               ; preds = %sout.false176, %sout.true175
  br label %sout.merge170

sout.int189:                                      ; preds = %sout.merge170
  %66 = call i32 (ptr, ...) @printf(ptr @.str.80, ptr %payload188)
  br label %sout.merge195

sout.float190:                                    ; preds = %sout.merge170
  %float.bits196 = ptrtoint ptr %payload188 to i64
  %float.val197 = bitcast i64 %float.bits196 to double
  %67 = call i32 (ptr, ...) @printf(ptr @.str.81, double %float.val197)
  br label %sout.merge195

sout.bool191:                                     ; preds = %sout.merge170
  %bool.val198 = ptrtoint ptr %payload188 to i64
  %is.true199 = icmp ne i64 %bool.val198, 0
  br i1 %is.true199, label %sout.true200, label %sout.false201

sout.str192:                                      ; preds = %sout.merge170
  %68 = call i32 (ptr, ...) @printf(ptr @.str.86, ptr %payload188)
  br label %sout.merge195

sout.null193:                                     ; preds = %sout.merge170
  %69 = call i32 (ptr, ...) @printf(ptr @.str.87, ptr @.str.88)
  br label %sout.merge195

sout.default194:                                  ; preds = %sout.merge170
  %70 = call i32 (ptr, ...) @printf(ptr @.str.89, ptr %payload188)
  br label %sout.merge195

sout.merge195:                                    ; preds = %sout.default194, %sout.null193, %sout.str192, %sout.bool.merge202, %sout.float190, %sout.int189
  %71 = call i32 @putchar(i32 10)
  %rcc.val203 = alloca %RCCValue, align 8
  %tag.ptr204 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val203, i32 0, i32 0
  store i64 0, ptr %tag.ptr204, align 4
  %payload.ptr205 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val203, i32 0, i32 1
  store ptr null, ptr %payload.ptr205, align 8
  %rcc.val206 = alloca %RCCValue, align 8
  %tag.ptr207 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val206, i32 0, i32 0
  store i64 1, ptr %tag.ptr207, align 4
  %payload.ptr208 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val206, i32 0, i32 1
  store ptr inttoptr (i64 9 to ptr), ptr %payload.ptr208, align 8
  %call.fib209 = call ptr @fib(ptr %rcc.val206)
  %tag.ptr210 = getelementptr inbounds nuw %RCCValue, ptr %call.fib209, i32 0, i32 0
  %tag211 = load i64, ptr %tag.ptr210, align 4
  %payload.ptr212 = getelementptr inbounds nuw %RCCValue, ptr %call.fib209, i32 0, i32 1
  %payload213 = load ptr, ptr %payload.ptr212, align 8
  switch i64 %tag211, label %sout.default219 [
    i64 1, label %sout.int214
    i64 2, label %sout.float215
    i64 3, label %sout.bool216
    i64 4, label %sout.str217
    i64 0, label %sout.null218
  ]

sout.true200:                                     ; preds = %sout.bool191
  %72 = call i32 (ptr, ...) @printf(ptr @.str.82, ptr @.str.83)
  br label %sout.bool.merge202

sout.false201:                                    ; preds = %sout.bool191
  %73 = call i32 (ptr, ...) @printf(ptr @.str.84, ptr @.str.85)
  br label %sout.bool.merge202

sout.bool.merge202:                               ; preds = %sout.false201, %sout.true200
  br label %sout.merge195

sout.int214:                                      ; preds = %sout.merge195
  %74 = call i32 (ptr, ...) @printf(ptr @.str.90, ptr %payload213)
  br label %sout.merge220

sout.float215:                                    ; preds = %sout.merge195
  %float.bits221 = ptrtoint ptr %payload213 to i64
  %float.val222 = bitcast i64 %float.bits221 to double
  %75 = call i32 (ptr, ...) @printf(ptr @.str.91, double %float.val222)
  br label %sout.merge220

sout.bool216:                                     ; preds = %sout.merge195
  %bool.val223 = ptrtoint ptr %payload213 to i64
  %is.true224 = icmp ne i64 %bool.val223, 0
  br i1 %is.true224, label %sout.true225, label %sout.false226

sout.str217:                                      ; preds = %sout.merge195
  %76 = call i32 (ptr, ...) @printf(ptr @.str.96, ptr %payload213)
  br label %sout.merge220

sout.null218:                                     ; preds = %sout.merge195
  %77 = call i32 (ptr, ...) @printf(ptr @.str.97, ptr @.str.98)
  br label %sout.merge220

sout.default219:                                  ; preds = %sout.merge195
  %78 = call i32 (ptr, ...) @printf(ptr @.str.99, ptr %payload213)
  br label %sout.merge220

sout.merge220:                                    ; preds = %sout.default219, %sout.null218, %sout.str217, %sout.bool.merge227, %sout.float215, %sout.int214
  %79 = call i32 @putchar(i32 10)
  %rcc.val228 = alloca %RCCValue, align 8
  %tag.ptr229 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val228, i32 0, i32 0
  store i64 0, ptr %tag.ptr229, align 4
  %payload.ptr230 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val228, i32 0, i32 1
  store ptr null, ptr %payload.ptr230, align 8
  %rcc.val231 = alloca %RCCValue, align 8
  %tag.ptr232 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val231, i32 0, i32 0
  store i64 1, ptr %tag.ptr232, align 4
  %payload.ptr233 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val231, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr233, align 8
  %call.fib234 = call ptr @fib(ptr %rcc.val231)
  %tag.ptr235 = getelementptr inbounds nuw %RCCValue, ptr %call.fib234, i32 0, i32 0
  %tag236 = load i64, ptr %tag.ptr235, align 4
  %payload.ptr237 = getelementptr inbounds nuw %RCCValue, ptr %call.fib234, i32 0, i32 1
  %payload238 = load ptr, ptr %payload.ptr237, align 8
  switch i64 %tag236, label %sout.default244 [
    i64 1, label %sout.int239
    i64 2, label %sout.float240
    i64 3, label %sout.bool241
    i64 4, label %sout.str242
    i64 0, label %sout.null243
  ]

sout.true225:                                     ; preds = %sout.bool216
  %80 = call i32 (ptr, ...) @printf(ptr @.str.92, ptr @.str.93)
  br label %sout.bool.merge227

sout.false226:                                    ; preds = %sout.bool216
  %81 = call i32 (ptr, ...) @printf(ptr @.str.94, ptr @.str.95)
  br label %sout.bool.merge227

sout.bool.merge227:                               ; preds = %sout.false226, %sout.true225
  br label %sout.merge220

sout.int239:                                      ; preds = %sout.merge220
  %82 = call i32 (ptr, ...) @printf(ptr @.str.100, ptr %payload238)
  br label %sout.merge245

sout.float240:                                    ; preds = %sout.merge220
  %float.bits246 = ptrtoint ptr %payload238 to i64
  %float.val247 = bitcast i64 %float.bits246 to double
  %83 = call i32 (ptr, ...) @printf(ptr @.str.101, double %float.val247)
  br label %sout.merge245

sout.bool241:                                     ; preds = %sout.merge220
  %bool.val248 = ptrtoint ptr %payload238 to i64
  %is.true249 = icmp ne i64 %bool.val248, 0
  br i1 %is.true249, label %sout.true250, label %sout.false251

sout.str242:                                      ; preds = %sout.merge220
  %84 = call i32 (ptr, ...) @printf(ptr @.str.106, ptr %payload238)
  br label %sout.merge245

sout.null243:                                     ; preds = %sout.merge220
  %85 = call i32 (ptr, ...) @printf(ptr @.str.107, ptr @.str.108)
  br label %sout.merge245

sout.default244:                                  ; preds = %sout.merge220
  %86 = call i32 (ptr, ...) @printf(ptr @.str.109, ptr %payload238)
  br label %sout.merge245

sout.merge245:                                    ; preds = %sout.default244, %sout.null243, %sout.str242, %sout.bool.merge252, %sout.float240, %sout.int239
  %87 = call i32 @putchar(i32 10)
  %rcc.val253 = alloca %RCCValue, align 8
  %tag.ptr254 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val253, i32 0, i32 0
  store i64 0, ptr %tag.ptr254, align 4
  %payload.ptr255 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val253, i32 0, i32 1
  store ptr null, ptr %payload.ptr255, align 8
  ret i32 0

sout.true250:                                     ; preds = %sout.bool241
  %88 = call i32 (ptr, ...) @printf(ptr @.str.102, ptr @.str.103)
  br label %sout.bool.merge252

sout.false251:                                    ; preds = %sout.bool241
  %89 = call i32 (ptr, ...) @printf(ptr @.str.104, ptr @.str.105)
  br label %sout.bool.merge252

sout.bool.merge252:                               ; preds = %sout.false251, %sout.true250
  br label %sout.merge245
}

define private ptr @fib(ptr %n) {
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
  %n.load12 = load ptr, ptr %n1, align 8
  %rcc.val13 = alloca %RCCValue, align 8
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 0
  store i64 1, ptr %tag.ptr14, align 4
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr15, align 8
  %payload.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %n.load12, i32 0, i32 1
  %payload17 = load ptr, ptr %payload.ptr16, align 8
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 1
  %payload19 = load ptr, ptr %payload.ptr18, align 8
  %left.int20 = ptrtoint ptr %payload17 to i64
  %right.int21 = ptrtoint ptr %payload19 to i64
  %sub = sub i64 %left.int20, %right.int21
  %int.ptr22 = inttoptr i64 %sub to ptr
  %rcc.val23 = alloca %RCCValue, align 8
  %tag.ptr24 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val23, i32 0, i32 0
  store i64 1, ptr %tag.ptr24, align 4
  %payload.ptr25 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val23, i32 0, i32 1
  store ptr %int.ptr22, ptr %payload.ptr25, align 8
  %call.fib = call ptr @fib(ptr %rcc.val23)
  %n.load26 = load ptr, ptr %n1, align 8
  %rcc.val27 = alloca %RCCValue, align 8
  %tag.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 0
  store i64 1, ptr %tag.ptr28, align 4
  %payload.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr29, align 8
  %payload.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %n.load26, i32 0, i32 1
  %payload31 = load ptr, ptr %payload.ptr30, align 8
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  %payload33 = load ptr, ptr %payload.ptr32, align 8
  %left.int34 = ptrtoint ptr %payload31 to i64
  %right.int35 = ptrtoint ptr %payload33 to i64
  %sub36 = sub i64 %left.int34, %right.int35
  %int.ptr37 = inttoptr i64 %sub36 to ptr
  %rcc.val38 = alloca %RCCValue, align 8
  %tag.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 0
  store i64 1, ptr %tag.ptr39, align 4
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 1
  store ptr %int.ptr37, ptr %payload.ptr40, align 8
  %call.fib41 = call ptr @fib(ptr %rcc.val38)
  %payload.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %call.fib, i32 0, i32 1
  %payload43 = load ptr, ptr %payload.ptr42, align 8
  %payload.ptr44 = getelementptr inbounds nuw %RCCValue, ptr %call.fib41, i32 0, i32 1
  %payload45 = load ptr, ptr %payload.ptr44, align 8
  %left.int46 = ptrtoint ptr %payload43 to i64
  %right.int47 = ptrtoint ptr %payload45 to i64
  %add = add i64 %left.int46, %right.int47
  %int.ptr48 = inttoptr i64 %add to ptr
  %rcc.val49 = alloca %RCCValue, align 8
  %tag.ptr50 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val49, i32 0, i32 0
  store i64 1, ptr %tag.ptr50, align 4
  %payload.ptr51 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val49, i32 0, i32 1
  store ptr %int.ptr48, ptr %payload.ptr51, align 8
  ret ptr %rcc.val49

cond.then.0:                                      ; preds = %entry
  %n.load11 = load ptr, ptr %n1, align 8
  ret ptr %n.load11

cond.next.0:                                      ; preds = %entry
  br label %cond.merge
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
