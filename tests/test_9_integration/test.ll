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
  %rcc.val29 = alloca %RCCValue, align 8
  %tag.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val29, i32 0, i32 0
  store i64 0, ptr %tag.ptr30, align 4
  %payload.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val29, i32 0, i32 1
  store ptr null, ptr %payload.ptr31, align 8
  %rcc.val32 = alloca %RCCValue, align 8
  %tag.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val32, i32 0, i32 0
  store i64 1, ptr %tag.ptr33, align 4
  %payload.ptr34 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val32, i32 0, i32 1
  store ptr inttoptr (i64 2 to ptr), ptr %payload.ptr34, align 8
  %call.fib35 = call ptr @fib(ptr %rcc.val32)
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
  %rcc.val55 = alloca %RCCValue, align 8
  %tag.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val55, i32 0, i32 0
  store i64 0, ptr %tag.ptr56, align 4
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val55, i32 0, i32 1
  store ptr null, ptr %payload.ptr57, align 8
  %rcc.val58 = alloca %RCCValue, align 8
  %tag.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val58, i32 0, i32 0
  store i64 1, ptr %tag.ptr59, align 4
  %payload.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val58, i32 0, i32 1
  store ptr inttoptr (i64 3 to ptr), ptr %payload.ptr60, align 8
  %call.fib61 = call ptr @fib(ptr %rcc.val58)
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
  %rcc.val81 = alloca %RCCValue, align 8
  %tag.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 0
  store i64 0, ptr %tag.ptr82, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 1
  store ptr null, ptr %payload.ptr83, align 8
  %rcc.val84 = alloca %RCCValue, align 8
  %tag.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val84, i32 0, i32 0
  store i64 1, ptr %tag.ptr85, align 4
  %payload.ptr86 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val84, i32 0, i32 1
  store ptr inttoptr (i64 4 to ptr), ptr %payload.ptr86, align 8
  %call.fib87 = call ptr @fib(ptr %rcc.val84)
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
  %rcc.val107 = alloca %RCCValue, align 8
  %tag.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val107, i32 0, i32 0
  store i64 0, ptr %tag.ptr108, align 4
  %payload.ptr109 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val107, i32 0, i32 1
  store ptr null, ptr %payload.ptr109, align 8
  %rcc.val110 = alloca %RCCValue, align 8
  %tag.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val110, i32 0, i32 0
  store i64 1, ptr %tag.ptr111, align 4
  %payload.ptr112 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val110, i32 0, i32 1
  store ptr inttoptr (i64 5 to ptr), ptr %payload.ptr112, align 8
  %call.fib113 = call ptr @fib(ptr %rcc.val110)
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
  %rcc.val133 = alloca %RCCValue, align 8
  %tag.ptr134 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val133, i32 0, i32 0
  store i64 0, ptr %tag.ptr134, align 4
  %payload.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val133, i32 0, i32 1
  store ptr null, ptr %payload.ptr135, align 8
  %rcc.val136 = alloca %RCCValue, align 8
  %tag.ptr137 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val136, i32 0, i32 0
  store i64 1, ptr %tag.ptr137, align 4
  %payload.ptr138 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val136, i32 0, i32 1
  store ptr inttoptr (i64 6 to ptr), ptr %payload.ptr138, align 8
  %call.fib139 = call ptr @fib(ptr %rcc.val136)
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
  %rcc.val159 = alloca %RCCValue, align 8
  %tag.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val159, i32 0, i32 0
  store i64 0, ptr %tag.ptr160, align 4
  %payload.ptr161 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val159, i32 0, i32 1
  store ptr null, ptr %payload.ptr161, align 8
  %rcc.val162 = alloca %RCCValue, align 8
  %tag.ptr163 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val162, i32 0, i32 0
  store i64 1, ptr %tag.ptr163, align 4
  %payload.ptr164 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val162, i32 0, i32 1
  store ptr inttoptr (i64 7 to ptr), ptr %payload.ptr164, align 8
  %call.fib165 = call ptr @fib(ptr %rcc.val162)
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
  %rcc.val185 = alloca %RCCValue, align 8
  %tag.ptr186 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val185, i32 0, i32 0
  store i64 0, ptr %tag.ptr186, align 4
  %payload.ptr187 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val185, i32 0, i32 1
  store ptr null, ptr %payload.ptr187, align 8
  %rcc.val188 = alloca %RCCValue, align 8
  %tag.ptr189 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val188, i32 0, i32 0
  store i64 1, ptr %tag.ptr189, align 4
  %payload.ptr190 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val188, i32 0, i32 1
  store ptr inttoptr (i64 8 to ptr), ptr %payload.ptr190, align 8
  %call.fib191 = call ptr @fib(ptr %rcc.val188)
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
  %rcc.val211 = alloca %RCCValue, align 8
  %tag.ptr212 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val211, i32 0, i32 0
  store i64 0, ptr %tag.ptr212, align 4
  %payload.ptr213 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val211, i32 0, i32 1
  store ptr null, ptr %payload.ptr213, align 8
  %rcc.val214 = alloca %RCCValue, align 8
  %tag.ptr215 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val214, i32 0, i32 0
  store i64 1, ptr %tag.ptr215, align 4
  %payload.ptr216 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val214, i32 0, i32 1
  store ptr inttoptr (i64 9 to ptr), ptr %payload.ptr216, align 8
  %call.fib217 = call ptr @fib(ptr %rcc.val214)
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
  %rcc.val237 = alloca %RCCValue, align 8
  %tag.ptr238 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val237, i32 0, i32 0
  store i64 0, ptr %tag.ptr238, align 4
  %payload.ptr239 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val237, i32 0, i32 1
  store ptr null, ptr %payload.ptr239, align 8
  %rcc.val240 = alloca %RCCValue, align 8
  %tag.ptr241 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val240, i32 0, i32 0
  store i64 1, ptr %tag.ptr241, align 4
  %payload.ptr242 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val240, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr242, align 8
  %call.fib243 = call ptr @fib(ptr %rcc.val240)
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
  %rcc.val263 = alloca %RCCValue, align 8
  %tag.ptr264 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val263, i32 0, i32 0
  store i64 0, ptr %tag.ptr264, align 4
  %payload.ptr265 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val263, i32 0, i32 1
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
  store i64 3, ptr %tag.ptr6, align 4
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
