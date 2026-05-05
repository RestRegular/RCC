; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [10 x i8] c"try: pass\00"
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
@.str.11 = private constant [14 x i8] c"finally: pass\00"
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
@.str.22 = private constant [11 x i8] c"try2: pass\00"
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
@.str.33 = private constant [15 x i8] c"finally2: pass\00"
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

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  br label %try.body

try.body:                                         ; preds = %entry
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 4, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr, align 8
  %tag.ptr1 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr1, align 4
  %payload.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr2, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

try.finally:                                      ; preds = %sout.merge
  %rcc.val6 = alloca %RCCValue, align 8
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 0
  store i64 4, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 1
  store ptr @.str.11, ptr %payload.ptr8, align 8
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 0
  %tag10 = load i64, ptr %tag.ptr9, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  switch i64 %tag10, label %sout.default18 [
    i64 1, label %sout.int13
    i64 2, label %sout.float14
    i64 3, label %sout.bool15
    i64 4, label %sout.str16
    i64 0, label %sout.null17
  ]

try.end:                                          ; preds = %sout.merge19
  %rcc.val30 = alloca %RCCValue, align 8
  %tag.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 0
  store i64 0, ptr %tag.ptr31, align 4
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 1
  store ptr null, ptr %payload.ptr32, align 8
  br label %try.body33

sout.int:                                         ; preds = %try.body
  %2 = call i32 (ptr, ...) @printf(ptr @.str.1, ptr %payload)
  br label %sout.merge

sout.float:                                       ; preds = %try.body
  %float.bits = ptrtoint ptr %payload to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.2, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %try.body
  %bool.val = ptrtoint ptr %payload to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %try.body
  %4 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr %payload)
  br label %sout.merge

sout.null:                                        ; preds = %try.body
  %5 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr @.str.9)
  br label %sout.merge

sout.default:                                     ; preds = %try.body
  %6 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val3 = alloca %RCCValue, align 8
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 0
  store i64 0, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 1
  store ptr null, ptr %payload.ptr5, align 8
  br label %try.finally

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.3, ptr @.str.4)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.5, ptr @.str.6)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int13:                                       ; preds = %try.finally
  %10 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr %payload12)
  br label %sout.merge19

sout.float14:                                     ; preds = %try.finally
  %float.bits20 = ptrtoint ptr %payload12 to i64
  %float.val21 = bitcast i64 %float.bits20 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.13, double %float.val21)
  br label %sout.merge19

sout.bool15:                                      ; preds = %try.finally
  %bool.val22 = ptrtoint ptr %payload12 to i64
  %is.true23 = icmp ne i64 %bool.val22, 0
  br i1 %is.true23, label %sout.true24, label %sout.false25

sout.str16:                                       ; preds = %try.finally
  %12 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr %payload12)
  br label %sout.merge19

sout.null17:                                      ; preds = %try.finally
  %13 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.merge19

sout.default18:                                   ; preds = %try.finally
  %14 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload12)
  br label %sout.merge19

sout.merge19:                                     ; preds = %sout.default18, %sout.null17, %sout.str16, %sout.bool.merge26, %sout.float14, %sout.int13
  %15 = call i32 @putchar(i32 10)
  %rcc.val27 = alloca %RCCValue, align 8
  %tag.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 0
  store i64 0, ptr %tag.ptr28, align 4
  %payload.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  store ptr null, ptr %payload.ptr29, align 8
  br label %try.end

sout.true24:                                      ; preds = %sout.bool15
  %16 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge26

sout.false25:                                     ; preds = %sout.bool15
  %17 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.17)
  br label %sout.bool.merge26

sout.bool.merge26:                                ; preds = %sout.false25, %sout.true24
  br label %sout.merge19

try.body33:                                       ; preds = %try.end
  %rcc.val36 = alloca %RCCValue, align 8
  %tag.ptr37 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val36, i32 0, i32 0
  store i64 4, ptr %tag.ptr37, align 4
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val36, i32 0, i32 1
  store ptr @.str.22, ptr %payload.ptr38, align 8
  %tag.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val36, i32 0, i32 0
  %tag40 = load i64, ptr %tag.ptr39, align 4
  %payload.ptr41 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val36, i32 0, i32 1
  %payload42 = load ptr, ptr %payload.ptr41, align 8
  switch i64 %tag40, label %sout.default48 [
    i64 1, label %sout.int43
    i64 2, label %sout.float44
    i64 3, label %sout.bool45
    i64 4, label %sout.str46
    i64 0, label %sout.null47
  ]

try.finally34:                                    ; preds = %try.catch, %sout.merge49
  %rcc.val60 = alloca %RCCValue, align 8
  %tag.ptr61 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val60, i32 0, i32 0
  store i64 4, ptr %tag.ptr61, align 4
  %payload.ptr62 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val60, i32 0, i32 1
  store ptr @.str.33, ptr %payload.ptr62, align 8
  %tag.ptr63 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val60, i32 0, i32 0
  %tag64 = load i64, ptr %tag.ptr63, align 4
  %payload.ptr65 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val60, i32 0, i32 1
  %payload66 = load ptr, ptr %payload.ptr65, align 8
  switch i64 %tag64, label %sout.default72 [
    i64 1, label %sout.int67
    i64 2, label %sout.float68
    i64 3, label %sout.bool69
    i64 4, label %sout.str70
    i64 0, label %sout.null71
  ]

try.end35:                                        ; preds = %sout.merge73
  %rcc.val84 = alloca %RCCValue, align 8
  %tag.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val84, i32 0, i32 0
  store i64 0, ptr %tag.ptr85, align 4
  %payload.ptr86 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val84, i32 0, i32 1
  store ptr null, ptr %payload.ptr86, align 8
  ret i32 0

sout.int43:                                       ; preds = %try.body33
  %18 = call i32 (ptr, ...) @printf(ptr @.str.23, ptr %payload42)
  br label %sout.merge49

sout.float44:                                     ; preds = %try.body33
  %float.bits50 = ptrtoint ptr %payload42 to i64
  %float.val51 = bitcast i64 %float.bits50 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.24, double %float.val51)
  br label %sout.merge49

sout.bool45:                                      ; preds = %try.body33
  %bool.val52 = ptrtoint ptr %payload42 to i64
  %is.true53 = icmp ne i64 %bool.val52, 0
  br i1 %is.true53, label %sout.true54, label %sout.false55

sout.str46:                                       ; preds = %try.body33
  %20 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload42)
  br label %sout.merge49

sout.null47:                                      ; preds = %try.body33
  %21 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr @.str.31)
  br label %sout.merge49

sout.default48:                                   ; preds = %try.body33
  %22 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr %payload42)
  br label %sout.merge49

sout.merge49:                                     ; preds = %sout.default48, %sout.null47, %sout.str46, %sout.bool.merge56, %sout.float44, %sout.int43
  %23 = call i32 @putchar(i32 10)
  %rcc.val57 = alloca %RCCValue, align 8
  %tag.ptr58 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val57, i32 0, i32 0
  store i64 0, ptr %tag.ptr58, align 4
  %payload.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val57, i32 0, i32 1
  store ptr null, ptr %payload.ptr59, align 8
  br label %try.finally34

sout.true54:                                      ; preds = %sout.bool45
  %24 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge56

sout.false55:                                     ; preds = %sout.bool45
  %25 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.bool.merge56

sout.bool.merge56:                                ; preds = %sout.false55, %sout.true54
  br label %sout.merge49

try.catch:                                        ; No predecessors!
  br label %try.finally34

sout.int67:                                       ; preds = %try.finally34
  %26 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr %payload66)
  br label %sout.merge73

sout.float68:                                     ; preds = %try.finally34
  %float.bits74 = ptrtoint ptr %payload66 to i64
  %float.val75 = bitcast i64 %float.bits74 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.35, double %float.val75)
  br label %sout.merge73

sout.bool69:                                      ; preds = %try.finally34
  %bool.val76 = ptrtoint ptr %payload66 to i64
  %is.true77 = icmp ne i64 %bool.val76, 0
  br i1 %is.true77, label %sout.true78, label %sout.false79

sout.str70:                                       ; preds = %try.finally34
  %28 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload66)
  br label %sout.merge73

sout.null71:                                      ; preds = %try.finally34
  %29 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr @.str.42)
  br label %sout.merge73

sout.default72:                                   ; preds = %try.finally34
  %30 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr %payload66)
  br label %sout.merge73

sout.merge73:                                     ; preds = %sout.default72, %sout.null71, %sout.str70, %sout.bool.merge80, %sout.float68, %sout.int67
  %31 = call i32 @putchar(i32 10)
  %rcc.val81 = alloca %RCCValue, align 8
  %tag.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 0
  store i64 0, ptr %tag.ptr82, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 1
  store ptr null, ptr %payload.ptr83, align 8
  br label %try.end35

sout.true78:                                      ; preds = %sout.bool69
  %32 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.bool.merge80

sout.false79:                                     ; preds = %sout.bool69
  %33 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr @.str.39)
  br label %sout.bool.merge80

sout.bool.merge80:                                ; preds = %sout.false79, %sout.true78
  br label %sout.merge73
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
