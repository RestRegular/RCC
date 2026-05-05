; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [12 x i8] c"hello world\00"
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
@.str.11 = private constant [5 x i8] c"%lld\00"
@.str.12 = private constant [3 x i8] c"%g\00"
@.str.13 = private constant [3 x i8] c"%s\00"
@.str.14 = private constant [5 x i8] c"true\00"
@.str.15 = private constant [3 x i8] c"%s\00"
@.str.16 = private constant [6 x i8] c"false\00"
@.str.17 = private constant [3 x i8] c"%s\00"
@.str.18 = private constant [3 x i8] c"%s\00"
@.str.19 = private constant [5 x i8] c"null\00"
@.str.20 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.21 = private constant [5 x i8] c"%lld\00"
@.str.22 = private constant [3 x i8] c"%g\00"
@.str.23 = private constant [3 x i8] c"%s\00"
@.str.24 = private constant [5 x i8] c"true\00"
@.str.25 = private constant [3 x i8] c"%s\00"
@.str.26 = private constant [6 x i8] c"false\00"
@.str.27 = private constant [3 x i8] c"%s\00"
@.str.28 = private constant [3 x i8] c"%s\00"
@.str.29 = private constant [5 x i8] c"null\00"
@.str.30 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.31 = private constant [5 x i8] c"%lld\00"
@.str.32 = private constant [3 x i8] c"%g\00"
@.str.33 = private constant [3 x i8] c"%s\00"
@.str.34 = private constant [5 x i8] c"true\00"
@.str.35 = private constant [3 x i8] c"%s\00"
@.str.36 = private constant [6 x i8] c"false\00"
@.str.37 = private constant [3 x i8] c"%s\00"
@.str.38 = private constant [3 x i8] c"%s\00"
@.str.39 = private constant [5 x i8] c"null\00"
@.str.40 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.41 = private constant [5 x i8] c"%lld\00"
@.str.42 = private constant [3 x i8] c"%g\00"
@.str.43 = private constant [3 x i8] c"%s\00"
@.str.44 = private constant [5 x i8] c"true\00"
@.str.45 = private constant [3 x i8] c"%s\00"
@.str.46 = private constant [6 x i8] c"false\00"
@.str.47 = private constant [3 x i8] c"%s\00"
@.str.48 = private constant [3 x i8] c"%s\00"
@.str.49 = private constant [5 x i8] c"null\00"
@.str.50 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.51 = private constant [5 x i8] c"%lld\00"
@.str.52 = private constant [3 x i8] c"%g\00"
@.str.53 = private constant [3 x i8] c"%s\00"
@.str.54 = private constant [5 x i8] c"true\00"
@.str.55 = private constant [3 x i8] c"%s\00"
@.str.56 = private constant [6 x i8] c"false\00"
@.str.57 = private constant [3 x i8] c"%s\00"
@.str.58 = private constant [3 x i8] c"%s\00"
@.str.59 = private constant [5 x i8] c"null\00"
@.str.60 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
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

sout.int:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str.1, ptr %payload)
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
  %rcc.val3 = alloca %RCCValue, align 8
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 0
  store i64 0, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val3, i32 0, i32 1
  store ptr null, ptr %payload.ptr5, align 8
  %rcc.val6 = alloca %RCCValue, align 8
  %tag.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 0
  store i64 1, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 1
  store ptr inttoptr (i64 123 to ptr), ptr %payload.ptr8, align 8
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

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.3, ptr @.str.4)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.5, ptr @.str.6)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int13:                                       ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.11, ptr %payload12)
  br label %sout.merge19

sout.float14:                                     ; preds = %sout.merge
  %float.bits20 = ptrtoint ptr %payload12 to i64
  %float.val21 = bitcast i64 %float.bits20 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.12, double %float.val21)
  br label %sout.merge19

sout.bool15:                                      ; preds = %sout.merge
  %bool.val22 = ptrtoint ptr %payload12 to i64
  %is.true23 = icmp ne i64 %bool.val22, 0
  br i1 %is.true23, label %sout.true24, label %sout.false25

sout.str16:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr %payload12)
  br label %sout.merge19

sout.null17:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr @.str.19)
  br label %sout.merge19

sout.default18:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.20, ptr %payload12)
  br label %sout.merge19

sout.merge19:                                     ; preds = %sout.default18, %sout.null17, %sout.str16, %sout.bool.merge26, %sout.float14, %sout.int13
  %15 = call i32 @putchar(i32 10)
  %rcc.val27 = alloca %RCCValue, align 8
  %tag.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 0
  store i64 0, ptr %tag.ptr28, align 4
  %payload.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  store ptr null, ptr %payload.ptr29, align 8
  %rcc.val30 = alloca %RCCValue, align 8
  %tag.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 0
  store i64 3, ptr %tag.ptr31, align 4
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr32, align 8
  %tag.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 0
  %tag34 = load i64, ptr %tag.ptr33, align 4
  %payload.ptr35 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val30, i32 0, i32 1
  %payload36 = load ptr, ptr %payload.ptr35, align 8
  switch i64 %tag34, label %sout.default42 [
    i64 1, label %sout.int37
    i64 2, label %sout.float38
    i64 3, label %sout.bool39
    i64 4, label %sout.str40
    i64 0, label %sout.null41
  ]

sout.true24:                                      ; preds = %sout.bool15
  %16 = call i32 (ptr, ...) @printf(ptr @.str.13, ptr @.str.14)
  br label %sout.bool.merge26

sout.false25:                                     ; preds = %sout.bool15
  %17 = call i32 (ptr, ...) @printf(ptr @.str.15, ptr @.str.16)
  br label %sout.bool.merge26

sout.bool.merge26:                                ; preds = %sout.false25, %sout.true24
  br label %sout.merge19

sout.int37:                                       ; preds = %sout.merge19
  %18 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload36)
  br label %sout.merge43

sout.float38:                                     ; preds = %sout.merge19
  %float.bits44 = ptrtoint ptr %payload36 to i64
  %float.val45 = bitcast i64 %float.bits44 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.22, double %float.val45)
  br label %sout.merge43

sout.bool39:                                      ; preds = %sout.merge19
  %bool.val46 = ptrtoint ptr %payload36 to i64
  %is.true47 = icmp ne i64 %bool.val46, 0
  br i1 %is.true47, label %sout.true48, label %sout.false49

sout.str40:                                       ; preds = %sout.merge19
  %20 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr %payload36)
  br label %sout.merge43

sout.null41:                                      ; preds = %sout.merge19
  %21 = call i32 (ptr, ...) @printf(ptr @.str.28, ptr @.str.29)
  br label %sout.merge43

sout.default42:                                   ; preds = %sout.merge19
  %22 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr %payload36)
  br label %sout.merge43

sout.merge43:                                     ; preds = %sout.default42, %sout.null41, %sout.str40, %sout.bool.merge50, %sout.float38, %sout.int37
  %23 = call i32 @putchar(i32 10)
  %rcc.val51 = alloca %RCCValue, align 8
  %tag.ptr52 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val51, i32 0, i32 0
  store i64 0, ptr %tag.ptr52, align 4
  %payload.ptr53 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val51, i32 0, i32 1
  store ptr null, ptr %payload.ptr53, align 8
  %rcc.val54 = alloca %RCCValue, align 8
  %tag.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 0
  store i64 3, ptr %tag.ptr55, align 4
  %payload.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 1
  store ptr null, ptr %payload.ptr56, align 8
  %tag.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 0
  %tag58 = load i64, ptr %tag.ptr57, align 4
  %payload.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 1
  %payload60 = load ptr, ptr %payload.ptr59, align 8
  switch i64 %tag58, label %sout.default66 [
    i64 1, label %sout.int61
    i64 2, label %sout.float62
    i64 3, label %sout.bool63
    i64 4, label %sout.str64
    i64 0, label %sout.null65
  ]

sout.true48:                                      ; preds = %sout.bool39
  %24 = call i32 (ptr, ...) @printf(ptr @.str.23, ptr @.str.24)
  br label %sout.bool.merge50

sout.false49:                                     ; preds = %sout.bool39
  %25 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge50

sout.bool.merge50:                                ; preds = %sout.false49, %sout.true48
  br label %sout.merge43

sout.int61:                                       ; preds = %sout.merge43
  %26 = call i32 (ptr, ...) @printf(ptr @.str.31, ptr %payload60)
  br label %sout.merge67

sout.float62:                                     ; preds = %sout.merge43
  %float.bits68 = ptrtoint ptr %payload60 to i64
  %float.val69 = bitcast i64 %float.bits68 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.32, double %float.val69)
  br label %sout.merge67

sout.bool63:                                      ; preds = %sout.merge43
  %bool.val70 = ptrtoint ptr %payload60 to i64
  %is.true71 = icmp ne i64 %bool.val70, 0
  br i1 %is.true71, label %sout.true72, label %sout.false73

sout.str64:                                       ; preds = %sout.merge43
  %28 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr %payload60)
  br label %sout.merge67

sout.null65:                                      ; preds = %sout.merge43
  %29 = call i32 (ptr, ...) @printf(ptr @.str.38, ptr @.str.39)
  br label %sout.merge67

sout.default66:                                   ; preds = %sout.merge43
  %30 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload60)
  br label %sout.merge67

sout.merge67:                                     ; preds = %sout.default66, %sout.null65, %sout.str64, %sout.bool.merge74, %sout.float62, %sout.int61
  %31 = call i32 @putchar(i32 10)
  %rcc.val75 = alloca %RCCValue, align 8
  %tag.ptr76 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val75, i32 0, i32 0
  store i64 0, ptr %tag.ptr76, align 4
  %payload.ptr77 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val75, i32 0, i32 1
  store ptr null, ptr %payload.ptr77, align 8
  %rcc.val78 = alloca %RCCValue, align 8
  %tag.ptr79 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 0
  store i64 0, ptr %tag.ptr79, align 4
  %payload.ptr80 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 1
  store ptr null, ptr %payload.ptr80, align 8
  %tag.ptr81 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 0
  %tag82 = load i64, ptr %tag.ptr81, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 1
  %payload84 = load ptr, ptr %payload.ptr83, align 8
  switch i64 %tag82, label %sout.default90 [
    i64 1, label %sout.int85
    i64 2, label %sout.float86
    i64 3, label %sout.bool87
    i64 4, label %sout.str88
    i64 0, label %sout.null89
  ]

sout.true72:                                      ; preds = %sout.bool63
  %32 = call i32 (ptr, ...) @printf(ptr @.str.33, ptr @.str.34)
  br label %sout.bool.merge74

sout.false73:                                     ; preds = %sout.bool63
  %33 = call i32 (ptr, ...) @printf(ptr @.str.35, ptr @.str.36)
  br label %sout.bool.merge74

sout.bool.merge74:                                ; preds = %sout.false73, %sout.true72
  br label %sout.merge67

sout.int85:                                       ; preds = %sout.merge67
  %34 = call i32 (ptr, ...) @printf(ptr @.str.41, ptr %payload84)
  br label %sout.merge91

sout.float86:                                     ; preds = %sout.merge67
  %float.bits92 = ptrtoint ptr %payload84 to i64
  %float.val93 = bitcast i64 %float.bits92 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.42, double %float.val93)
  br label %sout.merge91

sout.bool87:                                      ; preds = %sout.merge67
  %bool.val94 = ptrtoint ptr %payload84 to i64
  %is.true95 = icmp ne i64 %bool.val94, 0
  br i1 %is.true95, label %sout.true96, label %sout.false97

sout.str88:                                       ; preds = %sout.merge67
  %36 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr %payload84)
  br label %sout.merge91

sout.null89:                                      ; preds = %sout.merge67
  %37 = call i32 (ptr, ...) @printf(ptr @.str.48, ptr @.str.49)
  br label %sout.merge91

sout.default90:                                   ; preds = %sout.merge67
  %38 = call i32 (ptr, ...) @printf(ptr @.str.50, ptr %payload84)
  br label %sout.merge91

sout.merge91:                                     ; preds = %sout.default90, %sout.null89, %sout.str88, %sout.bool.merge98, %sout.float86, %sout.int85
  %39 = call i32 @putchar(i32 10)
  %rcc.val99 = alloca %RCCValue, align 8
  %tag.ptr100 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val99, i32 0, i32 0
  store i64 0, ptr %tag.ptr100, align 4
  %payload.ptr101 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val99, i32 0, i32 1
  store ptr null, ptr %payload.ptr101, align 8
  %rcc.val102 = alloca %RCCValue, align 8
  %tag.ptr103 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 0
  store i64 2, ptr %tag.ptr103, align 4
  %payload.ptr104 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 1
  store ptr inttoptr (i64 4614253070214989087 to ptr), ptr %payload.ptr104, align 8
  %tag.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 0
  %tag106 = load i64, ptr %tag.ptr105, align 4
  %payload.ptr107 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val102, i32 0, i32 1
  %payload108 = load ptr, ptr %payload.ptr107, align 8
  switch i64 %tag106, label %sout.default114 [
    i64 1, label %sout.int109
    i64 2, label %sout.float110
    i64 3, label %sout.bool111
    i64 4, label %sout.str112
    i64 0, label %sout.null113
  ]

sout.true96:                                      ; preds = %sout.bool87
  %40 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr @.str.44)
  br label %sout.bool.merge98

sout.false97:                                     ; preds = %sout.bool87
  %41 = call i32 (ptr, ...) @printf(ptr @.str.45, ptr @.str.46)
  br label %sout.bool.merge98

sout.bool.merge98:                                ; preds = %sout.false97, %sout.true96
  br label %sout.merge91

sout.int109:                                      ; preds = %sout.merge91
  %42 = call i32 (ptr, ...) @printf(ptr @.str.51, ptr %payload108)
  br label %sout.merge115

sout.float110:                                    ; preds = %sout.merge91
  %float.bits116 = ptrtoint ptr %payload108 to i64
  %float.val117 = bitcast i64 %float.bits116 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.52, double %float.val117)
  br label %sout.merge115

sout.bool111:                                     ; preds = %sout.merge91
  %bool.val118 = ptrtoint ptr %payload108 to i64
  %is.true119 = icmp ne i64 %bool.val118, 0
  br i1 %is.true119, label %sout.true120, label %sout.false121

sout.str112:                                      ; preds = %sout.merge91
  %44 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr %payload108)
  br label %sout.merge115

sout.null113:                                     ; preds = %sout.merge91
  %45 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.merge115

sout.default114:                                  ; preds = %sout.merge91
  %46 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload108)
  br label %sout.merge115

sout.merge115:                                    ; preds = %sout.default114, %sout.null113, %sout.str112, %sout.bool.merge122, %sout.float110, %sout.int109
  %47 = call i32 @putchar(i32 10)
  %rcc.val123 = alloca %RCCValue, align 8
  %tag.ptr124 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val123, i32 0, i32 0
  store i64 0, ptr %tag.ptr124, align 4
  %payload.ptr125 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val123, i32 0, i32 1
  store ptr null, ptr %payload.ptr125, align 8
  ret i32 0

sout.true120:                                     ; preds = %sout.bool111
  %48 = call i32 (ptr, ...) @printf(ptr @.str.53, ptr @.str.54)
  br label %sout.bool.merge122

sout.false121:                                    ; preds = %sout.bool111
  %49 = call i32 (ptr, ...) @printf(ptr @.str.55, ptr @.str.56)
  br label %sout.bool.merge122

sout.bool.merge122:                               ; preds = %sout.false121, %sout.true120
  br label %sout.merge115
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
