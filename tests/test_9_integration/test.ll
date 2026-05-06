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
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr null, ptr %payload.ptr, align 8
  %call.fib = call ptr @fib(ptr %rcc.val.heap)
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
  %rcc.val.heap3 = call ptr @malloc(i64 16)
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap3, i32 0, i32 0
  store i64 0, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap3, i32 0, i32 1
  store ptr null, ptr %payload.ptr5, align 8
  %rcc.val.heap6 = call ptr @malloc(i64 16)
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap6, i32 0, i32 0
  store i64 1, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap6, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr8, align 8
  %call.fib9 = call ptr @fib(ptr %rcc.val.heap6)
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
  %int.val21 = ptrtoint ptr %payload13 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val21)
  br label %sout.merge20

sout.float15:                                     ; preds = %sout.merge
  %float.bits22 = ptrtoint ptr %payload13 to i64
  %float.val23 = bitcast i64 %float.bits22 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val23)
  br label %sout.merge20

sout.bool16:                                      ; preds = %sout.merge
  %bool.val24 = ptrtoint ptr %payload13 to i64
  %is.true25 = icmp ne i64 %bool.val24, 0
  br i1 %is.true25, label %sout.true26, label %sout.false27

sout.str17:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload13)
  br label %sout.merge20

sout.null18:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge20

sout.default19:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload13)
  br label %sout.merge20

sout.merge20:                                     ; preds = %sout.default19, %sout.null18, %sout.str17, %sout.bool.merge28, %sout.float15, %sout.int14
  %15 = call i32 @putchar(i32 10)
  %rcc.val.heap29 = call ptr @malloc(i64 16)
  %tag.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap29, i32 0, i32 0
  store i64 0, ptr %tag.ptr30, align 4
  %payload.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap29, i32 0, i32 1
  store ptr null, ptr %payload.ptr31, align 8
  %rcc.val.heap32 = call ptr @malloc(i64 16)
  %tag.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap32, i32 0, i32 0
  store i64 1, ptr %tag.ptr33, align 4
  %payload.ptr34 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap32, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr34, align 8
  %call.fib35 = call ptr @fib(ptr %rcc.val.heap32)
  %tag.ptr36 = getelementptr inbounds nuw %RCCValue, ptr %call.fib35, i32 0, i32 0
  %tag37 = load i64, ptr %tag.ptr36, align 4
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %call.fib35, i32 0, i32 1
  %payload39 = load ptr, ptr %payload.ptr38, align 8
  switch i64 %tag37, label %sout.default45 [
    i64 1, label %sout.int40
    i64 2, label %sout.float41
    i64 3, label %sout.bool42
    i64 4, label %sout.str43
    i64 0, label %sout.null44
  ]

sout.true26:                                      ; preds = %sout.bool16
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge28

sout.false27:                                     ; preds = %sout.bool16
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge28

sout.bool.merge28:                                ; preds = %sout.false27, %sout.true26
  br label %sout.merge20

sout.int40:                                       ; preds = %sout.merge20
  %int.val47 = ptrtoint ptr %payload39 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val47)
  br label %sout.merge46

sout.float41:                                     ; preds = %sout.merge20
  %float.bits48 = ptrtoint ptr %payload39 to i64
  %float.val49 = bitcast i64 %float.bits48 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val49)
  br label %sout.merge46

sout.bool42:                                      ; preds = %sout.merge20
  %bool.val50 = ptrtoint ptr %payload39 to i64
  %is.true51 = icmp ne i64 %bool.val50, 0
  br i1 %is.true51, label %sout.true52, label %sout.false53

sout.str43:                                       ; preds = %sout.merge20
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload39)
  br label %sout.merge46

sout.null44:                                      ; preds = %sout.merge20
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge46

sout.default45:                                   ; preds = %sout.merge20
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload39)
  br label %sout.merge46

sout.merge46:                                     ; preds = %sout.default45, %sout.null44, %sout.str43, %sout.bool.merge54, %sout.float41, %sout.int40
  %23 = call i32 @putchar(i32 10)
  %rcc.val.heap55 = call ptr @malloc(i64 16)
  %tag.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap55, i32 0, i32 0
  store i64 0, ptr %tag.ptr56, align 4
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap55, i32 0, i32 1
  store ptr null, ptr %payload.ptr57, align 8
  %rcc.val.heap58 = call ptr @malloc(i64 16)
  %tag.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap58, i32 0, i32 0
  store i64 1, ptr %tag.ptr59, align 4
  %payload.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap58, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr60, align 8
  %call.fib61 = call ptr @fib(ptr %rcc.val.heap58)
  %tag.ptr62 = getelementptr inbounds nuw %RCCValue, ptr %call.fib61, i32 0, i32 0
  %tag63 = load i64, ptr %tag.ptr62, align 4
  %payload.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %call.fib61, i32 0, i32 1
  %payload65 = load ptr, ptr %payload.ptr64, align 8
  switch i64 %tag63, label %sout.default71 [
    i64 1, label %sout.int66
    i64 2, label %sout.float67
    i64 3, label %sout.bool68
    i64 4, label %sout.str69
    i64 0, label %sout.null70
  ]

sout.true52:                                      ; preds = %sout.bool42
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge54

sout.false53:                                     ; preds = %sout.bool42
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge54

sout.bool.merge54:                                ; preds = %sout.false53, %sout.true52
  br label %sout.merge46

sout.int66:                                       ; preds = %sout.merge46
  %int.val73 = ptrtoint ptr %payload65 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.30, i64 %int.val73)
  br label %sout.merge72

sout.float67:                                     ; preds = %sout.merge46
  %float.bits74 = ptrtoint ptr %payload65 to i64
  %float.val75 = bitcast i64 %float.bits74 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.31, double %float.val75)
  br label %sout.merge72

sout.bool68:                                      ; preds = %sout.merge46
  %bool.val76 = ptrtoint ptr %payload65 to i64
  %is.true77 = icmp ne i64 %bool.val76, 0
  br i1 %is.true77, label %sout.true78, label %sout.false79

sout.str69:                                       ; preds = %sout.merge46
  %28 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr %payload65)
  br label %sout.merge72

sout.null70:                                      ; preds = %sout.merge46
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.merge72

sout.default71:                                   ; preds = %sout.merge46
  %30 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload65)
  br label %sout.merge72

sout.merge72:                                     ; preds = %sout.default71, %sout.null70, %sout.str69, %sout.bool.merge80, %sout.float67, %sout.int66
  %31 = call i32 @putchar(i32 10)
  %rcc.val.heap81 = call ptr @malloc(i64 16)
  %tag.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap81, i32 0, i32 0
  store i64 0, ptr %tag.ptr82, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap81, i32 0, i32 1
  store ptr null, ptr %payload.ptr83, align 8
  %rcc.val.heap84 = call ptr @malloc(i64 16)
  %tag.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap84, i32 0, i32 0
  store i64 1, ptr %tag.ptr85, align 4
  %payload.ptr86 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap84, i32 0, i32 1
  store ptr inttoptr (i64 4 to ptr), ptr %payload.ptr86, align 8
  %call.fib87 = call ptr @fib(ptr %rcc.val.heap84)
  %tag.ptr88 = getelementptr inbounds nuw %RCCValue, ptr %call.fib87, i32 0, i32 0
  %tag89 = load i64, ptr %tag.ptr88, align 4
  %payload.ptr90 = getelementptr inbounds nuw %RCCValue, ptr %call.fib87, i32 0, i32 1
  %payload91 = load ptr, ptr %payload.ptr90, align 8
  switch i64 %tag89, label %sout.default97 [
    i64 1, label %sout.int92
    i64 2, label %sout.float93
    i64 3, label %sout.bool94
    i64 4, label %sout.str95
    i64 0, label %sout.null96
  ]

sout.true78:                                      ; preds = %sout.bool68
  %32 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr @.str.33)
  br label %sout.bool.merge80

sout.false79:                                     ; preds = %sout.bool68
  %33 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr @.str.35)
  br label %sout.bool.merge80

sout.bool.merge80:                                ; preds = %sout.false79, %sout.true78
  br label %sout.merge72

sout.int92:                                       ; preds = %sout.merge72
  %int.val99 = ptrtoint ptr %payload91 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, i64 %int.val99)
  br label %sout.merge98

sout.float93:                                     ; preds = %sout.merge72
  %float.bits100 = ptrtoint ptr %payload91 to i64
  %float.val101 = bitcast i64 %float.bits100 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val101)
  br label %sout.merge98

sout.bool94:                                      ; preds = %sout.merge72
  %bool.val102 = ptrtoint ptr %payload91 to i64
  %is.true103 = icmp ne i64 %bool.val102, 0
  br i1 %is.true103, label %sout.true104, label %sout.false105

sout.str95:                                       ; preds = %sout.merge72
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload91)
  br label %sout.merge98

sout.null96:                                      ; preds = %sout.merge72
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge98

sout.default97:                                   ; preds = %sout.merge72
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload91)
  br label %sout.merge98

sout.merge98:                                     ; preds = %sout.default97, %sout.null96, %sout.str95, %sout.bool.merge106, %sout.float93, %sout.int92
  %39 = call i32 @putchar(i32 10)
  %rcc.val.heap107 = call ptr @malloc(i64 16)
  %tag.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap107, i32 0, i32 0
  store i64 0, ptr %tag.ptr108, align 4
  %payload.ptr109 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap107, i32 0, i32 1
  store ptr null, ptr %payload.ptr109, align 8
  %rcc.val.heap110 = call ptr @malloc(i64 16)
  %tag.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap110, i32 0, i32 0
  store i64 1, ptr %tag.ptr111, align 4
  %payload.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap110, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr112, align 8
  %call.fib113 = call ptr @fib(ptr %rcc.val.heap110)
  %tag.ptr114 = getelementptr inbounds nuw %RCCValue, ptr %call.fib113, i32 0, i32 0
  %tag115 = load i64, ptr %tag.ptr114, align 4
  %payload.ptr116 = getelementptr inbounds nuw %RCCValue, ptr %call.fib113, i32 0, i32 1
  %payload117 = load ptr, ptr %payload.ptr116, align 8
  switch i64 %tag115, label %sout.default123 [
    i64 1, label %sout.int118
    i64 2, label %sout.float119
    i64 3, label %sout.bool120
    i64 4, label %sout.str121
    i64 0, label %sout.null122
  ]

sout.true104:                                     ; preds = %sout.bool94
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge106

sout.false105:                                    ; preds = %sout.bool94
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge106

sout.bool.merge106:                               ; preds = %sout.false105, %sout.true104
  br label %sout.merge98

sout.int118:                                      ; preds = %sout.merge98
  %int.val125 = ptrtoint ptr %payload117 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.50, i64 %int.val125)
  br label %sout.merge124

sout.float119:                                    ; preds = %sout.merge98
  %float.bits126 = ptrtoint ptr %payload117 to i64
  %float.val127 = bitcast i64 %float.bits126 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.51, double %float.val127)
  br label %sout.merge124

sout.bool120:                                     ; preds = %sout.merge98
  %bool.val128 = ptrtoint ptr %payload117 to i64
  %is.true129 = icmp ne i64 %bool.val128, 0
  br i1 %is.true129, label %sout.true130, label %sout.false131

sout.str121:                                      ; preds = %sout.merge98
  %44 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr %payload117)
  br label %sout.merge124

sout.null122:                                     ; preds = %sout.merge98
  %45 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr @.str.58)
  br label %sout.merge124

sout.default123:                                  ; preds = %sout.merge98
  %46 = call i32 (ptr, ...) @printf(ptr @.str.59, ptr %payload117)
  br label %sout.merge124

sout.merge124:                                    ; preds = %sout.default123, %sout.null122, %sout.str121, %sout.bool.merge132, %sout.float119, %sout.int118
  %47 = call i32 @putchar(i32 10)
  %rcc.val.heap133 = call ptr @malloc(i64 16)
  %tag.ptr134 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap133, i32 0, i32 0
  store i64 0, ptr %tag.ptr134, align 4
  %payload.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap133, i32 0, i32 1
  store ptr null, ptr %payload.ptr135, align 8
  %rcc.val.heap136 = call ptr @malloc(i64 16)
  %tag.ptr137 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap136, i32 0, i32 0
  store i64 1, ptr %tag.ptr137, align 4
  %payload.ptr138 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap136, i32 0, i32 1
  store ptr inttoptr (i64 6 to ptr), ptr %payload.ptr138, align 8
  %call.fib139 = call ptr @fib(ptr %rcc.val.heap136)
  %tag.ptr140 = getelementptr inbounds nuw %RCCValue, ptr %call.fib139, i32 0, i32 0
  %tag141 = load i64, ptr %tag.ptr140, align 4
  %payload.ptr142 = getelementptr inbounds nuw %RCCValue, ptr %call.fib139, i32 0, i32 1
  %payload143 = load ptr, ptr %payload.ptr142, align 8
  switch i64 %tag141, label %sout.default149 [
    i64 1, label %sout.int144
    i64 2, label %sout.float145
    i64 3, label %sout.bool146
    i64 4, label %sout.str147
    i64 0, label %sout.null148
  ]

sout.true130:                                     ; preds = %sout.bool120
  %48 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.bool.merge132

sout.false131:                                    ; preds = %sout.bool120
  %49 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr @.str.55)
  br label %sout.bool.merge132

sout.bool.merge132:                               ; preds = %sout.false131, %sout.true130
  br label %sout.merge124

sout.int144:                                      ; preds = %sout.merge124
  %int.val151 = ptrtoint ptr %payload143 to i64
  %50 = call i32 (ptr, ...) @printf(ptr @.str.60, i64 %int.val151)
  br label %sout.merge150

sout.float145:                                    ; preds = %sout.merge124
  %float.bits152 = ptrtoint ptr %payload143 to i64
  %float.val153 = bitcast i64 %float.bits152 to double
  %51 = call i32 (ptr, ...) @printf(ptr @.str.61, double %float.val153)
  br label %sout.merge150

sout.bool146:                                     ; preds = %sout.merge124
  %bool.val154 = ptrtoint ptr %payload143 to i64
  %is.true155 = icmp ne i64 %bool.val154, 0
  br i1 %is.true155, label %sout.true156, label %sout.false157

sout.str147:                                      ; preds = %sout.merge124
  %52 = call i32 (ptr, ...) @printf(ptr @.str.66, ptr %payload143)
  br label %sout.merge150

sout.null148:                                     ; preds = %sout.merge124
  %53 = call i32 (ptr, ...) @printf(ptr @.str.67, ptr @.str.68)
  br label %sout.merge150

sout.default149:                                  ; preds = %sout.merge124
  %54 = call i32 (ptr, ...) @printf(ptr @.str.69, ptr %payload143)
  br label %sout.merge150

sout.merge150:                                    ; preds = %sout.default149, %sout.null148, %sout.str147, %sout.bool.merge158, %sout.float145, %sout.int144
  %55 = call i32 @putchar(i32 10)
  %rcc.val.heap159 = call ptr @malloc(i64 16)
  %tag.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap159, i32 0, i32 0
  store i64 0, ptr %tag.ptr160, align 4
  %payload.ptr161 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap159, i32 0, i32 1
  store ptr null, ptr %payload.ptr161, align 8
  %rcc.val.heap162 = call ptr @malloc(i64 16)
  %tag.ptr163 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap162, i32 0, i32 0
  store i64 1, ptr %tag.ptr163, align 4
  %payload.ptr164 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap162, i32 0, i32 1
  store ptr inttoptr (i64 7 to ptr), ptr %payload.ptr164, align 8
  %call.fib165 = call ptr @fib(ptr %rcc.val.heap162)
  %tag.ptr166 = getelementptr inbounds nuw %RCCValue, ptr %call.fib165, i32 0, i32 0
  %tag167 = load i64, ptr %tag.ptr166, align 4
  %payload.ptr168 = getelementptr inbounds nuw %RCCValue, ptr %call.fib165, i32 0, i32 1
  %payload169 = load ptr, ptr %payload.ptr168, align 8
  switch i64 %tag167, label %sout.default175 [
    i64 1, label %sout.int170
    i64 2, label %sout.float171
    i64 3, label %sout.bool172
    i64 4, label %sout.str173
    i64 0, label %sout.null174
  ]

sout.true156:                                     ; preds = %sout.bool146
  %56 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr @.str.63)
  br label %sout.bool.merge158

sout.false157:                                    ; preds = %sout.bool146
  %57 = call i32 (ptr, ...) @printf(ptr @.str.64, ptr @.str.65)
  br label %sout.bool.merge158

sout.bool.merge158:                               ; preds = %sout.false157, %sout.true156
  br label %sout.merge150

sout.int170:                                      ; preds = %sout.merge150
  %int.val177 = ptrtoint ptr %payload169 to i64
  %58 = call i32 (ptr, ...) @printf(ptr @.str.70, i64 %int.val177)
  br label %sout.merge176

sout.float171:                                    ; preds = %sout.merge150
  %float.bits178 = ptrtoint ptr %payload169 to i64
  %float.val179 = bitcast i64 %float.bits178 to double
  %59 = call i32 (ptr, ...) @printf(ptr @.str.71, double %float.val179)
  br label %sout.merge176

sout.bool172:                                     ; preds = %sout.merge150
  %bool.val180 = ptrtoint ptr %payload169 to i64
  %is.true181 = icmp ne i64 %bool.val180, 0
  br i1 %is.true181, label %sout.true182, label %sout.false183

sout.str173:                                      ; preds = %sout.merge150
  %60 = call i32 (ptr, ...) @printf(ptr @.str.76, ptr %payload169)
  br label %sout.merge176

sout.null174:                                     ; preds = %sout.merge150
  %61 = call i32 (ptr, ...) @printf(ptr @.str.77, ptr @.str.78)
  br label %sout.merge176

sout.default175:                                  ; preds = %sout.merge150
  %62 = call i32 (ptr, ...) @printf(ptr @.str.79, ptr %payload169)
  br label %sout.merge176

sout.merge176:                                    ; preds = %sout.default175, %sout.null174, %sout.str173, %sout.bool.merge184, %sout.float171, %sout.int170
  %63 = call i32 @putchar(i32 10)
  %rcc.val.heap185 = call ptr @malloc(i64 16)
  %tag.ptr186 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap185, i32 0, i32 0
  store i64 0, ptr %tag.ptr186, align 4
  %payload.ptr187 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap185, i32 0, i32 1
  store ptr null, ptr %payload.ptr187, align 8
  %rcc.val.heap188 = call ptr @malloc(i64 16)
  %tag.ptr189 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap188, i32 0, i32 0
  store i64 1, ptr %tag.ptr189, align 4
  %payload.ptr190 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap188, i32 0, i32 1
  store ptr inttoptr (i64 8 to ptr), ptr %payload.ptr190, align 8
  %call.fib191 = call ptr @fib(ptr %rcc.val.heap188)
  %tag.ptr192 = getelementptr inbounds nuw %RCCValue, ptr %call.fib191, i32 0, i32 0
  %tag193 = load i64, ptr %tag.ptr192, align 4
  %payload.ptr194 = getelementptr inbounds nuw %RCCValue, ptr %call.fib191, i32 0, i32 1
  %payload195 = load ptr, ptr %payload.ptr194, align 8
  switch i64 %tag193, label %sout.default201 [
    i64 1, label %sout.int196
    i64 2, label %sout.float197
    i64 3, label %sout.bool198
    i64 4, label %sout.str199
    i64 0, label %sout.null200
  ]

sout.true182:                                     ; preds = %sout.bool172
  %64 = call i32 (ptr, ...) @printf(ptr @.str.72, ptr @.str.73)
  br label %sout.bool.merge184

sout.false183:                                    ; preds = %sout.bool172
  %65 = call i32 (ptr, ...) @printf(ptr @.str.74, ptr @.str.75)
  br label %sout.bool.merge184

sout.bool.merge184:                               ; preds = %sout.false183, %sout.true182
  br label %sout.merge176

sout.int196:                                      ; preds = %sout.merge176
  %int.val203 = ptrtoint ptr %payload195 to i64
  %66 = call i32 (ptr, ...) @printf(ptr @.str.80, i64 %int.val203)
  br label %sout.merge202

sout.float197:                                    ; preds = %sout.merge176
  %float.bits204 = ptrtoint ptr %payload195 to i64
  %float.val205 = bitcast i64 %float.bits204 to double
  %67 = call i32 (ptr, ...) @printf(ptr @.str.81, double %float.val205)
  br label %sout.merge202

sout.bool198:                                     ; preds = %sout.merge176
  %bool.val206 = ptrtoint ptr %payload195 to i64
  %is.true207 = icmp ne i64 %bool.val206, 0
  br i1 %is.true207, label %sout.true208, label %sout.false209

sout.str199:                                      ; preds = %sout.merge176
  %68 = call i32 (ptr, ...) @printf(ptr @.str.86, ptr %payload195)
  br label %sout.merge202

sout.null200:                                     ; preds = %sout.merge176
  %69 = call i32 (ptr, ...) @printf(ptr @.str.87, ptr @.str.88)
  br label %sout.merge202

sout.default201:                                  ; preds = %sout.merge176
  %70 = call i32 (ptr, ...) @printf(ptr @.str.89, ptr %payload195)
  br label %sout.merge202

sout.merge202:                                    ; preds = %sout.default201, %sout.null200, %sout.str199, %sout.bool.merge210, %sout.float197, %sout.int196
  %71 = call i32 @putchar(i32 10)
  %rcc.val.heap211 = call ptr @malloc(i64 16)
  %tag.ptr212 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap211, i32 0, i32 0
  store i64 0, ptr %tag.ptr212, align 4
  %payload.ptr213 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap211, i32 0, i32 1
  store ptr null, ptr %payload.ptr213, align 8
  %rcc.val.heap214 = call ptr @malloc(i64 16)
  %tag.ptr215 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap214, i32 0, i32 0
  store i64 1, ptr %tag.ptr215, align 4
  %payload.ptr216 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap214, i32 0, i32 1
  store ptr inttoptr (i64 9 to ptr), ptr %payload.ptr216, align 8
  %call.fib217 = call ptr @fib(ptr %rcc.val.heap214)
  %tag.ptr218 = getelementptr inbounds nuw %RCCValue, ptr %call.fib217, i32 0, i32 0
  %tag219 = load i64, ptr %tag.ptr218, align 4
  %payload.ptr220 = getelementptr inbounds nuw %RCCValue, ptr %call.fib217, i32 0, i32 1
  %payload221 = load ptr, ptr %payload.ptr220, align 8
  switch i64 %tag219, label %sout.default227 [
    i64 1, label %sout.int222
    i64 2, label %sout.float223
    i64 3, label %sout.bool224
    i64 4, label %sout.str225
    i64 0, label %sout.null226
  ]

sout.true208:                                     ; preds = %sout.bool198
  %72 = call i32 (ptr, ...) @printf(ptr @.str.82, ptr @.str.83)
  br label %sout.bool.merge210

sout.false209:                                    ; preds = %sout.bool198
  %73 = call i32 (ptr, ...) @printf(ptr @.str.84, ptr @.str.85)
  br label %sout.bool.merge210

sout.bool.merge210:                               ; preds = %sout.false209, %sout.true208
  br label %sout.merge202

sout.int222:                                      ; preds = %sout.merge202
  %int.val229 = ptrtoint ptr %payload221 to i64
  %74 = call i32 (ptr, ...) @printf(ptr @.str.90, i64 %int.val229)
  br label %sout.merge228

sout.float223:                                    ; preds = %sout.merge202
  %float.bits230 = ptrtoint ptr %payload221 to i64
  %float.val231 = bitcast i64 %float.bits230 to double
  %75 = call i32 (ptr, ...) @printf(ptr @.str.91, double %float.val231)
  br label %sout.merge228

sout.bool224:                                     ; preds = %sout.merge202
  %bool.val232 = ptrtoint ptr %payload221 to i64
  %is.true233 = icmp ne i64 %bool.val232, 0
  br i1 %is.true233, label %sout.true234, label %sout.false235

sout.str225:                                      ; preds = %sout.merge202
  %76 = call i32 (ptr, ...) @printf(ptr @.str.96, ptr %payload221)
  br label %sout.merge228

sout.null226:                                     ; preds = %sout.merge202
  %77 = call i32 (ptr, ...) @printf(ptr @.str.97, ptr @.str.98)
  br label %sout.merge228

sout.default227:                                  ; preds = %sout.merge202
  %78 = call i32 (ptr, ...) @printf(ptr @.str.99, ptr %payload221)
  br label %sout.merge228

sout.merge228:                                    ; preds = %sout.default227, %sout.null226, %sout.str225, %sout.bool.merge236, %sout.float223, %sout.int222
  %79 = call i32 @putchar(i32 10)
  %rcc.val.heap237 = call ptr @malloc(i64 16)
  %tag.ptr238 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap237, i32 0, i32 0
  store i64 0, ptr %tag.ptr238, align 4
  %payload.ptr239 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap237, i32 0, i32 1
  store ptr null, ptr %payload.ptr239, align 8
  %rcc.val.heap240 = call ptr @malloc(i64 16)
  %tag.ptr241 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap240, i32 0, i32 0
  store i64 1, ptr %tag.ptr241, align 4
  %payload.ptr242 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap240, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr242, align 8
  %call.fib243 = call ptr @fib(ptr %rcc.val.heap240)
  %tag.ptr244 = getelementptr inbounds nuw %RCCValue, ptr %call.fib243, i32 0, i32 0
  %tag245 = load i64, ptr %tag.ptr244, align 4
  %payload.ptr246 = getelementptr inbounds nuw %RCCValue, ptr %call.fib243, i32 0, i32 1
  %payload247 = load ptr, ptr %payload.ptr246, align 8
  switch i64 %tag245, label %sout.default253 [
    i64 1, label %sout.int248
    i64 2, label %sout.float249
    i64 3, label %sout.bool250
    i64 4, label %sout.str251
    i64 0, label %sout.null252
  ]

sout.true234:                                     ; preds = %sout.bool224
  %80 = call i32 (ptr, ...) @printf(ptr @.str.92, ptr @.str.93)
  br label %sout.bool.merge236

sout.false235:                                    ; preds = %sout.bool224
  %81 = call i32 (ptr, ...) @printf(ptr @.str.94, ptr @.str.95)
  br label %sout.bool.merge236

sout.bool.merge236:                               ; preds = %sout.false235, %sout.true234
  br label %sout.merge228

sout.int248:                                      ; preds = %sout.merge228
  %int.val255 = ptrtoint ptr %payload247 to i64
  %82 = call i32 (ptr, ...) @printf(ptr @.str.100, i64 %int.val255)
  br label %sout.merge254

sout.float249:                                    ; preds = %sout.merge228
  %float.bits256 = ptrtoint ptr %payload247 to i64
  %float.val257 = bitcast i64 %float.bits256 to double
  %83 = call i32 (ptr, ...) @printf(ptr @.str.101, double %float.val257)
  br label %sout.merge254

sout.bool250:                                     ; preds = %sout.merge228
  %bool.val258 = ptrtoint ptr %payload247 to i64
  %is.true259 = icmp ne i64 %bool.val258, 0
  br i1 %is.true259, label %sout.true260, label %sout.false261

sout.str251:                                      ; preds = %sout.merge228
  %84 = call i32 (ptr, ...) @printf(ptr @.str.106, ptr %payload247)
  br label %sout.merge254

sout.null252:                                     ; preds = %sout.merge228
  %85 = call i32 (ptr, ...) @printf(ptr @.str.107, ptr @.str.108)
  br label %sout.merge254

sout.default253:                                  ; preds = %sout.merge228
  %86 = call i32 (ptr, ...) @printf(ptr @.str.109, ptr %payload247)
  br label %sout.merge254

sout.merge254:                                    ; preds = %sout.default253, %sout.null252, %sout.str251, %sout.bool.merge262, %sout.float249, %sout.int248
  %87 = call i32 @putchar(i32 10)
  %rcc.val.heap263 = call ptr @malloc(i64 16)
  %tag.ptr264 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap263, i32 0, i32 0
  store i64 0, ptr %tag.ptr264, align 4
  %payload.ptr265 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap263, i32 0, i32 1
  store ptr null, ptr %payload.ptr265, align 8
  ret i32 0

sout.true260:                                     ; preds = %sout.bool250
  %88 = call i32 (ptr, ...) @printf(ptr @.str.102, ptr @.str.103)
  br label %sout.bool.merge262

sout.false261:                                    ; preds = %sout.bool250
  %89 = call i32 (ptr, ...) @printf(ptr @.str.104, ptr @.str.105)
  br label %sout.bool.merge262

sout.bool.merge262:                               ; preds = %sout.false261, %sout.true260
  br label %sout.merge254
}

define private ptr @fib(ptr %n) {
entry:
  %arith.result122 = alloca ptr, align 8
  %arith.result80 = alloca ptr, align 8
  %arith.result37 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
  %n1 = alloca ptr, align 8
  store ptr %n, ptr %n1, align 8
  %n.load = load ptr, ptr %n1, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr2, align 4
  %tag.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  %tag4 = load i64, ptr %tag.ptr3, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag4, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

cond.merge:                                       ; preds = %cond.next.0
  %n.load23 = load ptr, ptr %n1, align 8
  %rcc.val.heap24 = call ptr @malloc(i64 16)
  %tag.ptr25 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap24, i32 0, i32 0
  store i64 1, ptr %tag.ptr25, align 4
  %payload.ptr26 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap24, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr26, align 8
  %tag.ptr27 = getelementptr inbounds nuw %RCCValue, ptr %n.load23, i32 0, i32 0
  %tag28 = load i64, ptr %tag.ptr27, align 4
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap24, i32 0, i32 0
  %tag30 = load i64, ptr %tag.ptr29, align 4
  %is_float.left31 = icmp eq i64 %tag28, 2
  %is_float.right32 = icmp eq i64 %tag30, 2
  %is_float33 = or i1 %is_float.left31, %is_float.right32
  store ptr null, ptr %arith.result37, align 8
  br i1 %is_float33, label %arith.float34, label %arith.int35

arith.float:                                      ; preds = %entry
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
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
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val.heap8 = call ptr @malloc(i64 16)
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 0
  store i64 2, ptr %tag.ptr9, align 4
  %payload.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr10, align 8
  store ptr %rcc.val.heap8, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %n.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  %payload14 = load ptr, ptr %payload.ptr13, align 8
  %left.int = ptrtoint ptr %payload12 to i64
  %right.int = ptrtoint ptr %payload14 to i64
  %cmp.le = icmp sle i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.le to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val.heap15 = call ptr @malloc(i64 16)
  %tag.ptr16 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 0
  store i64 3, ptr %tag.ptr16, align 4
  %payload.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap15, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr17, align 8
  store ptr %rcc.val.heap15, ptr %arith.result, align 8
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
  %n.load22 = load ptr, ptr %n1, align 8
  ret ptr %n.load22

cond.next.0:                                      ; preds = %arith.merge
  br label %cond.merge

arith.float34:                                    ; preds = %cond.merge
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %n.load23, i32 0, i32 1
  %payload39 = load ptr, ptr %payload.ptr38, align 8
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap24, i32 0, i32 1
  %payload41 = load ptr, ptr %payload.ptr40, align 8
  %left.bits42 = ptrtoint ptr %payload39 to i64
  %right.bits43 = ptrtoint ptr %payload41 to i64
  %left.sitofp44 = sitofp i64 %left.bits42 to double
  %left.bitcast45 = bitcast i64 %left.bits42 to double
  %left.double46 = select i1 %is_float.left31, double %left.bitcast45, double %left.sitofp44
  %right.sitofp47 = sitofp i64 %right.bits43 to double
  %right.bitcast48 = bitcast i64 %right.bits43 to double
  %right.double49 = select i1 %is_float.right32, double %right.bitcast48, double %right.sitofp47
  %fsub = fsub double %left.double46, %right.double49
  %result.bits50 = bitcast double %fsub to i64
  %result.float.ptr51 = inttoptr i64 %result.bits50 to ptr
  %rcc.val.heap52 = call ptr @malloc(i64 16)
  %tag.ptr53 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap52, i32 0, i32 0
  store i64 2, ptr %tag.ptr53, align 4
  %payload.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap52, i32 0, i32 1
  store ptr %result.float.ptr51, ptr %payload.ptr54, align 8
  store ptr %rcc.val.heap52, ptr %arith.result37, align 8
  br label %arith.merge36

arith.int35:                                      ; preds = %cond.merge
  %payload.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %n.load23, i32 0, i32 1
  %payload56 = load ptr, ptr %payload.ptr55, align 8
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap24, i32 0, i32 1
  %payload58 = load ptr, ptr %payload.ptr57, align 8
  %left.int59 = ptrtoint ptr %payload56 to i64
  %right.int60 = ptrtoint ptr %payload58 to i64
  %sub = sub i64 %left.int59, %right.int60
  %int.ptr61 = inttoptr i64 %sub to ptr
  %rcc.val.heap62 = call ptr @malloc(i64 16)
  %tag.ptr63 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap62, i32 0, i32 0
  store i64 1, ptr %tag.ptr63, align 4
  %payload.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap62, i32 0, i32 1
  store ptr %int.ptr61, ptr %payload.ptr64, align 8
  store ptr %rcc.val.heap62, ptr %arith.result37, align 8
  br label %arith.merge36

arith.merge36:                                    ; preds = %arith.int35, %arith.float34
  %arith.result.load65 = load ptr, ptr %arith.result37, align 8
  %call.fib = call ptr @fib(ptr %arith.result.load65)
  %n.load66 = load ptr, ptr %n1, align 8
  %rcc.val.heap67 = call ptr @malloc(i64 16)
  %tag.ptr68 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap67, i32 0, i32 0
  store i64 1, ptr %tag.ptr68, align 4
  %payload.ptr69 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap67, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr69, align 8
  %tag.ptr70 = getelementptr inbounds nuw %RCCValue, ptr %n.load66, i32 0, i32 0
  %tag71 = load i64, ptr %tag.ptr70, align 4
  %tag.ptr72 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap67, i32 0, i32 0
  %tag73 = load i64, ptr %tag.ptr72, align 4
  %is_float.left74 = icmp eq i64 %tag71, 2
  %is_float.right75 = icmp eq i64 %tag73, 2
  %is_float76 = or i1 %is_float.left74, %is_float.right75
  store ptr null, ptr %arith.result80, align 8
  br i1 %is_float76, label %arith.float77, label %arith.int78

arith.float77:                                    ; preds = %arith.merge36
  %payload.ptr81 = getelementptr inbounds nuw %RCCValue, ptr %n.load66, i32 0, i32 1
  %payload82 = load ptr, ptr %payload.ptr81, align 8
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap67, i32 0, i32 1
  %payload84 = load ptr, ptr %payload.ptr83, align 8
  %left.bits85 = ptrtoint ptr %payload82 to i64
  %right.bits86 = ptrtoint ptr %payload84 to i64
  %left.sitofp87 = sitofp i64 %left.bits85 to double
  %left.bitcast88 = bitcast i64 %left.bits85 to double
  %left.double89 = select i1 %is_float.left74, double %left.bitcast88, double %left.sitofp87
  %right.sitofp90 = sitofp i64 %right.bits86 to double
  %right.bitcast91 = bitcast i64 %right.bits86 to double
  %right.double92 = select i1 %is_float.right75, double %right.bitcast91, double %right.sitofp90
  %fsub93 = fsub double %left.double89, %right.double92
  %result.bits94 = bitcast double %fsub93 to i64
  %result.float.ptr95 = inttoptr i64 %result.bits94 to ptr
  %rcc.val.heap96 = call ptr @malloc(i64 16)
  %tag.ptr97 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap96, i32 0, i32 0
  store i64 2, ptr %tag.ptr97, align 4
  %payload.ptr98 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap96, i32 0, i32 1
  store ptr %result.float.ptr95, ptr %payload.ptr98, align 8
  store ptr %rcc.val.heap96, ptr %arith.result80, align 8
  br label %arith.merge79

arith.int78:                                      ; preds = %arith.merge36
  %payload.ptr99 = getelementptr inbounds nuw %RCCValue, ptr %n.load66, i32 0, i32 1
  %payload100 = load ptr, ptr %payload.ptr99, align 8
  %payload.ptr101 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap67, i32 0, i32 1
  %payload102 = load ptr, ptr %payload.ptr101, align 8
  %left.int103 = ptrtoint ptr %payload100 to i64
  %right.int104 = ptrtoint ptr %payload102 to i64
  %sub105 = sub i64 %left.int103, %right.int104
  %int.ptr106 = inttoptr i64 %sub105 to ptr
  %rcc.val.heap107 = call ptr @malloc(i64 16)
  %tag.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap107, i32 0, i32 0
  store i64 1, ptr %tag.ptr108, align 4
  %payload.ptr109 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap107, i32 0, i32 1
  store ptr %int.ptr106, ptr %payload.ptr109, align 8
  store ptr %rcc.val.heap107, ptr %arith.result80, align 8
  br label %arith.merge79

arith.merge79:                                    ; preds = %arith.int78, %arith.float77
  %arith.result.load110 = load ptr, ptr %arith.result80, align 8
  %call.fib111 = call ptr @fib(ptr %arith.result.load110)
  %tag.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %call.fib, i32 0, i32 0
  %tag113 = load i64, ptr %tag.ptr112, align 4
  %tag.ptr114 = getelementptr inbounds nuw %RCCValue, ptr %call.fib111, i32 0, i32 0
  %tag115 = load i64, ptr %tag.ptr114, align 4
  %is_float.left116 = icmp eq i64 %tag113, 2
  %is_float.right117 = icmp eq i64 %tag115, 2
  %is_float118 = or i1 %is_float.left116, %is_float.right117
  store ptr null, ptr %arith.result122, align 8
  br i1 %is_float118, label %arith.float119, label %arith.int120

arith.float119:                                   ; preds = %arith.merge79
  %payload.ptr123 = getelementptr inbounds nuw %RCCValue, ptr %call.fib, i32 0, i32 1
  %payload124 = load ptr, ptr %payload.ptr123, align 8
  %payload.ptr125 = getelementptr inbounds nuw %RCCValue, ptr %call.fib111, i32 0, i32 1
  %payload126 = load ptr, ptr %payload.ptr125, align 8
  %left.bits127 = ptrtoint ptr %payload124 to i64
  %right.bits128 = ptrtoint ptr %payload126 to i64
  %left.sitofp129 = sitofp i64 %left.bits127 to double
  %left.bitcast130 = bitcast i64 %left.bits127 to double
  %left.double131 = select i1 %is_float.left116, double %left.bitcast130, double %left.sitofp129
  %right.sitofp132 = sitofp i64 %right.bits128 to double
  %right.bitcast133 = bitcast i64 %right.bits128 to double
  %right.double134 = select i1 %is_float.right117, double %right.bitcast133, double %right.sitofp132
  %fadd = fadd double %left.double131, %right.double134
  %result.bits135 = bitcast double %fadd to i64
  %result.float.ptr136 = inttoptr i64 %result.bits135 to ptr
  %rcc.val.heap137 = call ptr @malloc(i64 16)
  %tag.ptr138 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap137, i32 0, i32 0
  store i64 2, ptr %tag.ptr138, align 4
  %payload.ptr139 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap137, i32 0, i32 1
  store ptr %result.float.ptr136, ptr %payload.ptr139, align 8
  store ptr %rcc.val.heap137, ptr %arith.result122, align 8
  br label %arith.merge121

arith.int120:                                     ; preds = %arith.merge79
  %payload.ptr140 = getelementptr inbounds nuw %RCCValue, ptr %call.fib, i32 0, i32 1
  %payload141 = load ptr, ptr %payload.ptr140, align 8
  %payload.ptr142 = getelementptr inbounds nuw %RCCValue, ptr %call.fib111, i32 0, i32 1
  %payload143 = load ptr, ptr %payload.ptr142, align 8
  %left.int144 = ptrtoint ptr %payload141 to i64
  %right.int145 = ptrtoint ptr %payload143 to i64
  %add = add i64 %left.int144, %right.int145
  %int.ptr146 = inttoptr i64 %add to ptr
  %rcc.val.heap147 = call ptr @malloc(i64 16)
  %tag.ptr148 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap147, i32 0, i32 0
  store i64 1, ptr %tag.ptr148, align 4
  %payload.ptr149 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap147, i32 0, i32 1
  store ptr %int.ptr146, ptr %payload.ptr149, align 8
  store ptr %rcc.val.heap147, ptr %arith.result122, align 8
  br label %arith.merge121

arith.merge121:                                   ; preds = %arith.int120, %arith.float119
  %arith.result.load150 = load ptr, ptr %arith.result122, align 8
  ret ptr %arith.result.load150
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
