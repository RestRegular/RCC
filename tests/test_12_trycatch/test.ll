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
  %rcc.val31 = alloca %RCCValue, align 8
  %tag.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 0
  store i64 0, ptr %tag.ptr32, align 4
  %payload.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 1
  store ptr null, ptr %payload.ptr33, align 8
  br label %try.body34

sout.int:                                         ; preds = %try.body
  %int.val = ptrtoint ptr %payload to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str.1, i64 %int.val)
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
  %int.val20 = ptrtoint ptr %payload12 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.12, i64 %int.val20)
  br label %sout.merge19

sout.float14:                                     ; preds = %try.finally
  %float.bits21 = ptrtoint ptr %payload12 to i64
  %float.val22 = bitcast i64 %float.bits21 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.13, double %float.val22)
  br label %sout.merge19

sout.bool15:                                      ; preds = %try.finally
  %bool.val23 = ptrtoint ptr %payload12 to i64
  %is.true24 = icmp ne i64 %bool.val23, 0
  br i1 %is.true24, label %sout.true25, label %sout.false26

sout.str16:                                       ; preds = %try.finally
  %12 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr %payload12)
  br label %sout.merge19

sout.null17:                                      ; preds = %try.finally
  %13 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.merge19

sout.default18:                                   ; preds = %try.finally
  %14 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload12)
  br label %sout.merge19

sout.merge19:                                     ; preds = %sout.default18, %sout.null17, %sout.str16, %sout.bool.merge27, %sout.float14, %sout.int13
  %15 = call i32 @putchar(i32 10)
  %rcc.val28 = alloca %RCCValue, align 8
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val28, i32 0, i32 0
  store i64 0, ptr %tag.ptr29, align 4
  %payload.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val28, i32 0, i32 1
  store ptr null, ptr %payload.ptr30, align 8
  br label %try.end

sout.true25:                                      ; preds = %sout.bool15
  %16 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge27

sout.false26:                                     ; preds = %sout.bool15
  %17 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.17)
  br label %sout.bool.merge27

sout.bool.merge27:                                ; preds = %sout.false26, %sout.true25
  br label %sout.merge19

try.body34:                                       ; preds = %try.end
  %rcc.val37 = alloca %RCCValue, align 8
  %tag.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val37, i32 0, i32 0
  store i64 4, ptr %tag.ptr38, align 4
  %payload.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val37, i32 0, i32 1
  store ptr @.str.22, ptr %payload.ptr39, align 8
  %tag.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val37, i32 0, i32 0
  %tag41 = load i64, ptr %tag.ptr40, align 4
  %payload.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val37, i32 0, i32 1
  %payload43 = load ptr, ptr %payload.ptr42, align 8
  switch i64 %tag41, label %sout.default49 [
    i64 1, label %sout.int44
    i64 2, label %sout.float45
    i64 3, label %sout.bool46
    i64 4, label %sout.str47
    i64 0, label %sout.null48
  ]

try.finally35:                                    ; preds = %try.catch, %sout.merge50
  %rcc.val62 = alloca %RCCValue, align 8
  %tag.ptr63 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val62, i32 0, i32 0
  store i64 4, ptr %tag.ptr63, align 4
  %payload.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val62, i32 0, i32 1
  store ptr @.str.33, ptr %payload.ptr64, align 8
  %tag.ptr65 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val62, i32 0, i32 0
  %tag66 = load i64, ptr %tag.ptr65, align 4
  %payload.ptr67 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val62, i32 0, i32 1
  %payload68 = load ptr, ptr %payload.ptr67, align 8
  switch i64 %tag66, label %sout.default74 [
    i64 1, label %sout.int69
    i64 2, label %sout.float70
    i64 3, label %sout.bool71
    i64 4, label %sout.str72
    i64 0, label %sout.null73
  ]

try.end36:                                        ; preds = %sout.merge75
  %rcc.val87 = alloca %RCCValue, align 8
  %tag.ptr88 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val87, i32 0, i32 0
  store i64 0, ptr %tag.ptr88, align 4
  %payload.ptr89 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val87, i32 0, i32 1
  store ptr null, ptr %payload.ptr89, align 8
  ret i32 0

sout.int44:                                       ; preds = %try.body34
  %int.val51 = ptrtoint ptr %payload43 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.23, i64 %int.val51)
  br label %sout.merge50

sout.float45:                                     ; preds = %try.body34
  %float.bits52 = ptrtoint ptr %payload43 to i64
  %float.val53 = bitcast i64 %float.bits52 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.24, double %float.val53)
  br label %sout.merge50

sout.bool46:                                      ; preds = %try.body34
  %bool.val54 = ptrtoint ptr %payload43 to i64
  %is.true55 = icmp ne i64 %bool.val54, 0
  br i1 %is.true55, label %sout.true56, label %sout.false57

sout.str47:                                       ; preds = %try.body34
  %20 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload43)
  br label %sout.merge50

sout.null48:                                      ; preds = %try.body34
  %21 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr @.str.31)
  br label %sout.merge50

sout.default49:                                   ; preds = %try.body34
  %22 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr %payload43)
  br label %sout.merge50

sout.merge50:                                     ; preds = %sout.default49, %sout.null48, %sout.str47, %sout.bool.merge58, %sout.float45, %sout.int44
  %23 = call i32 @putchar(i32 10)
  %rcc.val59 = alloca %RCCValue, align 8
  %tag.ptr60 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val59, i32 0, i32 0
  store i64 0, ptr %tag.ptr60, align 4
  %payload.ptr61 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val59, i32 0, i32 1
  store ptr null, ptr %payload.ptr61, align 8
  br label %try.finally35

sout.true56:                                      ; preds = %sout.bool46
  %24 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge58

sout.false57:                                     ; preds = %sout.bool46
  %25 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.bool.merge58

sout.bool.merge58:                                ; preds = %sout.false57, %sout.true56
  br label %sout.merge50

try.catch:                                        ; No predecessors!
  br label %try.finally35

sout.int69:                                       ; preds = %try.finally35
  %int.val76 = ptrtoint ptr %payload68 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.34, i64 %int.val76)
  br label %sout.merge75

sout.float70:                                     ; preds = %try.finally35
  %float.bits77 = ptrtoint ptr %payload68 to i64
  %float.val78 = bitcast i64 %float.bits77 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.35, double %float.val78)
  br label %sout.merge75

sout.bool71:                                      ; preds = %try.finally35
  %bool.val79 = ptrtoint ptr %payload68 to i64
  %is.true80 = icmp ne i64 %bool.val79, 0
  br i1 %is.true80, label %sout.true81, label %sout.false82

sout.str72:                                       ; preds = %try.finally35
  %28 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload68)
  br label %sout.merge75

sout.null73:                                      ; preds = %try.finally35
  %29 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr @.str.42)
  br label %sout.merge75

sout.default74:                                   ; preds = %try.finally35
  %30 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr %payload68)
  br label %sout.merge75

sout.merge75:                                     ; preds = %sout.default74, %sout.null73, %sout.str72, %sout.bool.merge83, %sout.float70, %sout.int69
  %31 = call i32 @putchar(i32 10)
  %rcc.val84 = alloca %RCCValue, align 8
  %tag.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val84, i32 0, i32 0
  store i64 0, ptr %tag.ptr85, align 4
  %payload.ptr86 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val84, i32 0, i32 1
  store ptr null, ptr %payload.ptr86, align 8
  br label %try.end36

sout.true81:                                      ; preds = %sout.bool71
  %32 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr @.str.37)
  br label %sout.bool.merge83

sout.false82:                                     ; preds = %sout.bool71
  %33 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr @.str.39)
  br label %sout.bool.merge83

sout.bool.merge83:                                ; preds = %sout.false82, %sout.true81
  br label %sout.merge75
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
