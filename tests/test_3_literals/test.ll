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
@.str.40 = private constant [6 x i8] c"hello\00"
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
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr, align 8
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
  store i64 2, ptr %tag.ptr7, align 4
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val6, i32 0, i32 1
  store ptr inttoptr (i64 4614253070214989087 to ptr), ptr %payload.ptr8, align 8
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
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int13:                                       ; preds = %sout.merge
  %int.val20 = ptrtoint ptr %payload12 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val20)
  br label %sout.merge19

sout.float14:                                     ; preds = %sout.merge
  %float.bits21 = ptrtoint ptr %payload12 to i64
  %float.val22 = bitcast i64 %float.bits21 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val22)
  br label %sout.merge19

sout.bool15:                                      ; preds = %sout.merge
  %bool.val23 = ptrtoint ptr %payload12 to i64
  %is.true24 = icmp ne i64 %bool.val23, 0
  br i1 %is.true24, label %sout.true25, label %sout.false26

sout.str16:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload12)
  br label %sout.merge19

sout.null17:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge19

sout.default18:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload12)
  br label %sout.merge19

sout.merge19:                                     ; preds = %sout.default18, %sout.null17, %sout.str16, %sout.bool.merge27, %sout.float14, %sout.int13
  %15 = call i32 @putchar(i32 10)
  %rcc.val28 = alloca %RCCValue, align 8
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val28, i32 0, i32 0
  store i64 0, ptr %tag.ptr29, align 4
  %payload.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val28, i32 0, i32 1
  store ptr null, ptr %payload.ptr30, align 8
  %rcc.val31 = alloca %RCCValue, align 8
  %tag.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 0
  store i64 3, ptr %tag.ptr32, align 4
  %payload.ptr33 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr33, align 8
  %tag.ptr34 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 0
  %tag35 = load i64, ptr %tag.ptr34, align 4
  %payload.ptr36 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val31, i32 0, i32 1
  %payload37 = load ptr, ptr %payload.ptr36, align 8
  switch i64 %tag35, label %sout.default43 [
    i64 1, label %sout.int38
    i64 2, label %sout.float39
    i64 3, label %sout.bool40
    i64 4, label %sout.str41
    i64 0, label %sout.null42
  ]

sout.true25:                                      ; preds = %sout.bool15
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge27

sout.false26:                                     ; preds = %sout.bool15
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge27

sout.bool.merge27:                                ; preds = %sout.false26, %sout.true25
  br label %sout.merge19

sout.int38:                                       ; preds = %sout.merge19
  %int.val45 = ptrtoint ptr %payload37 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val45)
  br label %sout.merge44

sout.float39:                                     ; preds = %sout.merge19
  %float.bits46 = ptrtoint ptr %payload37 to i64
  %float.val47 = bitcast i64 %float.bits46 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val47)
  br label %sout.merge44

sout.bool40:                                      ; preds = %sout.merge19
  %bool.val48 = ptrtoint ptr %payload37 to i64
  %is.true49 = icmp ne i64 %bool.val48, 0
  br i1 %is.true49, label %sout.true50, label %sout.false51

sout.str41:                                       ; preds = %sout.merge19
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload37)
  br label %sout.merge44

sout.null42:                                      ; preds = %sout.merge19
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge44

sout.default43:                                   ; preds = %sout.merge19
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload37)
  br label %sout.merge44

sout.merge44:                                     ; preds = %sout.default43, %sout.null42, %sout.str41, %sout.bool.merge52, %sout.float39, %sout.int38
  %23 = call i32 @putchar(i32 10)
  %rcc.val53 = alloca %RCCValue, align 8
  %tag.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val53, i32 0, i32 0
  store i64 0, ptr %tag.ptr54, align 4
  %payload.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val53, i32 0, i32 1
  store ptr null, ptr %payload.ptr55, align 8
  %rcc.val56 = alloca %RCCValue, align 8
  %tag.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 0
  store i64 3, ptr %tag.ptr57, align 4
  %payload.ptr58 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 1
  store ptr null, ptr %payload.ptr58, align 8
  %tag.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 0
  %tag60 = load i64, ptr %tag.ptr59, align 4
  %payload.ptr61 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val56, i32 0, i32 1
  %payload62 = load ptr, ptr %payload.ptr61, align 8
  switch i64 %tag60, label %sout.default68 [
    i64 1, label %sout.int63
    i64 2, label %sout.float64
    i64 3, label %sout.bool65
    i64 4, label %sout.str66
    i64 0, label %sout.null67
  ]

sout.true50:                                      ; preds = %sout.bool40
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge52

sout.false51:                                     ; preds = %sout.bool40
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge52

sout.bool.merge52:                                ; preds = %sout.false51, %sout.true50
  br label %sout.merge44

sout.int63:                                       ; preds = %sout.merge44
  %int.val70 = ptrtoint ptr %payload62 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.30, i64 %int.val70)
  br label %sout.merge69

sout.float64:                                     ; preds = %sout.merge44
  %float.bits71 = ptrtoint ptr %payload62 to i64
  %float.val72 = bitcast i64 %float.bits71 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.31, double %float.val72)
  br label %sout.merge69

sout.bool65:                                      ; preds = %sout.merge44
  %bool.val73 = ptrtoint ptr %payload62 to i64
  %is.true74 = icmp ne i64 %bool.val73, 0
  br i1 %is.true74, label %sout.true75, label %sout.false76

sout.str66:                                       ; preds = %sout.merge44
  %28 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr %payload62)
  br label %sout.merge69

sout.null67:                                      ; preds = %sout.merge44
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.merge69

sout.default68:                                   ; preds = %sout.merge44
  %30 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload62)
  br label %sout.merge69

sout.merge69:                                     ; preds = %sout.default68, %sout.null67, %sout.str66, %sout.bool.merge77, %sout.float64, %sout.int63
  %31 = call i32 @putchar(i32 10)
  %rcc.val78 = alloca %RCCValue, align 8
  %tag.ptr79 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 0
  store i64 0, ptr %tag.ptr79, align 4
  %payload.ptr80 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val78, i32 0, i32 1
  store ptr null, ptr %payload.ptr80, align 8
  %rcc.val81 = alloca %RCCValue, align 8
  %tag.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 0
  store i64 4, ptr %tag.ptr82, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 1
  store ptr @.str.40, ptr %payload.ptr83, align 8
  %tag.ptr84 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 0
  %tag85 = load i64, ptr %tag.ptr84, align 4
  %payload.ptr86 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 1
  %payload87 = load ptr, ptr %payload.ptr86, align 8
  switch i64 %tag85, label %sout.default93 [
    i64 1, label %sout.int88
    i64 2, label %sout.float89
    i64 3, label %sout.bool90
    i64 4, label %sout.str91
    i64 0, label %sout.null92
  ]

sout.true75:                                      ; preds = %sout.bool65
  %32 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr @.str.33)
  br label %sout.bool.merge77

sout.false76:                                     ; preds = %sout.bool65
  %33 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr @.str.35)
  br label %sout.bool.merge77

sout.bool.merge77:                                ; preds = %sout.false76, %sout.true75
  br label %sout.merge69

sout.int88:                                       ; preds = %sout.merge69
  %int.val95 = ptrtoint ptr %payload87 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.41, i64 %int.val95)
  br label %sout.merge94

sout.float89:                                     ; preds = %sout.merge69
  %float.bits96 = ptrtoint ptr %payload87 to i64
  %float.val97 = bitcast i64 %float.bits96 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.42, double %float.val97)
  br label %sout.merge94

sout.bool90:                                      ; preds = %sout.merge69
  %bool.val98 = ptrtoint ptr %payload87 to i64
  %is.true99 = icmp ne i64 %bool.val98, 0
  br i1 %is.true99, label %sout.true100, label %sout.false101

sout.str91:                                       ; preds = %sout.merge69
  %36 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr %payload87)
  br label %sout.merge94

sout.null92:                                      ; preds = %sout.merge69
  %37 = call i32 (ptr, ...) @printf(ptr @.str.48, ptr @.str.49)
  br label %sout.merge94

sout.default93:                                   ; preds = %sout.merge69
  %38 = call i32 (ptr, ...) @printf(ptr @.str.50, ptr %payload87)
  br label %sout.merge94

sout.merge94:                                     ; preds = %sout.default93, %sout.null92, %sout.str91, %sout.bool.merge102, %sout.float89, %sout.int88
  %39 = call i32 @putchar(i32 10)
  %rcc.val103 = alloca %RCCValue, align 8
  %tag.ptr104 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val103, i32 0, i32 0
  store i64 0, ptr %tag.ptr104, align 4
  %payload.ptr105 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val103, i32 0, i32 1
  store ptr null, ptr %payload.ptr105, align 8
  %rcc.val106 = alloca %RCCValue, align 8
  %tag.ptr107 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val106, i32 0, i32 0
  store i64 0, ptr %tag.ptr107, align 4
  %payload.ptr108 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val106, i32 0, i32 1
  store ptr null, ptr %payload.ptr108, align 8
  %tag.ptr109 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val106, i32 0, i32 0
  %tag110 = load i64, ptr %tag.ptr109, align 4
  %payload.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val106, i32 0, i32 1
  %payload112 = load ptr, ptr %payload.ptr111, align 8
  switch i64 %tag110, label %sout.default118 [
    i64 1, label %sout.int113
    i64 2, label %sout.float114
    i64 3, label %sout.bool115
    i64 4, label %sout.str116
    i64 0, label %sout.null117
  ]

sout.true100:                                     ; preds = %sout.bool90
  %40 = call i32 (ptr, ...) @printf(ptr @.str.43, ptr @.str.44)
  br label %sout.bool.merge102

sout.false101:                                    ; preds = %sout.bool90
  %41 = call i32 (ptr, ...) @printf(ptr @.str.45, ptr @.str.46)
  br label %sout.bool.merge102

sout.bool.merge102:                               ; preds = %sout.false101, %sout.true100
  br label %sout.merge94

sout.int113:                                      ; preds = %sout.merge94
  %int.val120 = ptrtoint ptr %payload112 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.51, i64 %int.val120)
  br label %sout.merge119

sout.float114:                                    ; preds = %sout.merge94
  %float.bits121 = ptrtoint ptr %payload112 to i64
  %float.val122 = bitcast i64 %float.bits121 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.52, double %float.val122)
  br label %sout.merge119

sout.bool115:                                     ; preds = %sout.merge94
  %bool.val123 = ptrtoint ptr %payload112 to i64
  %is.true124 = icmp ne i64 %bool.val123, 0
  br i1 %is.true124, label %sout.true125, label %sout.false126

sout.str116:                                      ; preds = %sout.merge94
  %44 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr %payload112)
  br label %sout.merge119

sout.null117:                                     ; preds = %sout.merge94
  %45 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.merge119

sout.default118:                                  ; preds = %sout.merge94
  %46 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload112)
  br label %sout.merge119

sout.merge119:                                    ; preds = %sout.default118, %sout.null117, %sout.str116, %sout.bool.merge127, %sout.float114, %sout.int113
  %47 = call i32 @putchar(i32 10)
  %rcc.val128 = alloca %RCCValue, align 8
  %tag.ptr129 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val128, i32 0, i32 0
  store i64 0, ptr %tag.ptr129, align 4
  %payload.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val128, i32 0, i32 1
  store ptr null, ptr %payload.ptr130, align 8
  ret i32 0

sout.true125:                                     ; preds = %sout.bool115
  %48 = call i32 (ptr, ...) @printf(ptr @.str.53, ptr @.str.54)
  br label %sout.bool.merge127

sout.false126:                                    ; preds = %sout.bool115
  %49 = call i32 (ptr, ...) @printf(ptr @.str.55, ptr @.str.56)
  br label %sout.bool.merge127

sout.bool.merge127:                               ; preds = %sout.false126, %sout.true125
  br label %sout.merge119
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
