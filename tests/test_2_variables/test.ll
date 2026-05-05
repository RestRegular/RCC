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
@.str.10 = private constant [4 x i8] c"RCC\00"
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

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %flag = alloca ptr, align 8
  %name = alloca ptr, align 8
  %z = alloca ptr, align 8
  %y = alloca ptr, align 8
  %x = alloca ptr, align 8
  store ptr null, ptr %x, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 100 to ptr), ptr %payload.ptr, align 8
  store ptr %rcc.val, ptr %x, align 8
  store ptr null, ptr %y, align 8
  %rcc.val1 = alloca %RCCValue, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr3, align 8
  store ptr %rcc.val1, ptr %y, align 8
  store ptr null, ptr %z, align 8
  %x.load = load ptr, ptr %x, align 8
  %y.load = load ptr, ptr %y, align 8
  %payload.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr4, align 8
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %y.load, i32 0, i32 1
  %payload6 = load ptr, ptr %payload.ptr5, align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload6 to i64
  %add = add i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %add to ptr
  %rcc.val7 = alloca %RCCValue, align 8
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val7, i32 0, i32 0
  store i64 1, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val7, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr9, align 8
  store ptr %rcc.val7, ptr %z, align 8
  %z.load = load ptr, ptr %z, align 8
  %tag.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %z.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr10, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %z.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %int.val = ptrtoint ptr %payload12 to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload12 to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload12 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload12)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %5 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %6 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload12)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val13 = alloca %RCCValue, align 8
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 0
  store i64 0, ptr %tag.ptr14, align 4
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val13, i32 0, i32 1
  store ptr null, ptr %payload.ptr15, align 8
  store ptr null, ptr %name, align 8
  %rcc.val16 = alloca %RCCValue, align 8
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 0
  store i64 4, ptr %tag.ptr17, align 4
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val16, i32 0, i32 1
  store ptr @.str.10, ptr %payload.ptr18, align 8
  store ptr %rcc.val16, ptr %name, align 8
  %name.load = load ptr, ptr %name, align 8
  %tag.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %name.load, i32 0, i32 0
  %tag20 = load i64, ptr %tag.ptr19, align 4
  %payload.ptr21 = getelementptr inbounds nuw %RCCValue, ptr %name.load, i32 0, i32 1
  %payload22 = load ptr, ptr %payload.ptr21, align 8
  switch i64 %tag20, label %sout.default28 [
    i64 1, label %sout.int23
    i64 2, label %sout.float24
    i64 3, label %sout.bool25
    i64 4, label %sout.str26
    i64 0, label %sout.null27
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int23:                                       ; preds = %sout.merge
  %int.val30 = ptrtoint ptr %payload22 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.11, i64 %int.val30)
  br label %sout.merge29

sout.float24:                                     ; preds = %sout.merge
  %float.bits31 = ptrtoint ptr %payload22 to i64
  %float.val32 = bitcast i64 %float.bits31 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.12, double %float.val32)
  br label %sout.merge29

sout.bool25:                                      ; preds = %sout.merge
  %bool.val33 = ptrtoint ptr %payload22 to i64
  %is.true34 = icmp ne i64 %bool.val33, 0
  br i1 %is.true34, label %sout.true35, label %sout.false36

sout.str26:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr %payload22)
  br label %sout.merge29

sout.null27:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr @.str.19)
  br label %sout.merge29

sout.default28:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.20, ptr %payload22)
  br label %sout.merge29

sout.merge29:                                     ; preds = %sout.default28, %sout.null27, %sout.str26, %sout.bool.merge37, %sout.float24, %sout.int23
  %15 = call i32 @putchar(i32 10)
  %rcc.val38 = alloca %RCCValue, align 8
  %tag.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 0
  store i64 0, ptr %tag.ptr39, align 4
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val38, i32 0, i32 1
  store ptr null, ptr %payload.ptr40, align 8
  store ptr null, ptr %flag, align 8
  %rcc.val41 = alloca %RCCValue, align 8
  %tag.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val41, i32 0, i32 0
  store i64 3, ptr %tag.ptr42, align 4
  %payload.ptr43 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val41, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr43, align 8
  store ptr %rcc.val41, ptr %flag, align 8
  %flag.load = load ptr, ptr %flag, align 8
  %tag.ptr44 = getelementptr inbounds nuw %RCCValue, ptr %flag.load, i32 0, i32 0
  %tag45 = load i64, ptr %tag.ptr44, align 4
  %payload.ptr46 = getelementptr inbounds nuw %RCCValue, ptr %flag.load, i32 0, i32 1
  %payload47 = load ptr, ptr %payload.ptr46, align 8
  switch i64 %tag45, label %sout.default53 [
    i64 1, label %sout.int48
    i64 2, label %sout.float49
    i64 3, label %sout.bool50
    i64 4, label %sout.str51
    i64 0, label %sout.null52
  ]

sout.true35:                                      ; preds = %sout.bool25
  %16 = call i32 (ptr, ...) @printf(ptr @.str.13, ptr @.str.14)
  br label %sout.bool.merge37

sout.false36:                                     ; preds = %sout.bool25
  %17 = call i32 (ptr, ...) @printf(ptr @.str.15, ptr @.str.16)
  br label %sout.bool.merge37

sout.bool.merge37:                                ; preds = %sout.false36, %sout.true35
  br label %sout.merge29

sout.int48:                                       ; preds = %sout.merge29
  %int.val55 = ptrtoint ptr %payload47 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.21, i64 %int.val55)
  br label %sout.merge54

sout.float49:                                     ; preds = %sout.merge29
  %float.bits56 = ptrtoint ptr %payload47 to i64
  %float.val57 = bitcast i64 %float.bits56 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.22, double %float.val57)
  br label %sout.merge54

sout.bool50:                                      ; preds = %sout.merge29
  %bool.val58 = ptrtoint ptr %payload47 to i64
  %is.true59 = icmp ne i64 %bool.val58, 0
  br i1 %is.true59, label %sout.true60, label %sout.false61

sout.str51:                                       ; preds = %sout.merge29
  %20 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr %payload47)
  br label %sout.merge54

sout.null52:                                      ; preds = %sout.merge29
  %21 = call i32 (ptr, ...) @printf(ptr @.str.28, ptr @.str.29)
  br label %sout.merge54

sout.default53:                                   ; preds = %sout.merge29
  %22 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr %payload47)
  br label %sout.merge54

sout.merge54:                                     ; preds = %sout.default53, %sout.null52, %sout.str51, %sout.bool.merge62, %sout.float49, %sout.int48
  %23 = call i32 @putchar(i32 10)
  %rcc.val63 = alloca %RCCValue, align 8
  %tag.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val63, i32 0, i32 0
  store i64 0, ptr %tag.ptr64, align 4
  %payload.ptr65 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val63, i32 0, i32 1
  store ptr null, ptr %payload.ptr65, align 8
  ret i32 0

sout.true60:                                      ; preds = %sout.bool50
  %24 = call i32 (ptr, ...) @printf(ptr @.str.23, ptr @.str.24)
  br label %sout.bool.merge62

sout.false61:                                     ; preds = %sout.bool50
  %25 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge62

sout.bool.merge62:                                ; preds = %sout.false61, %sout.true60
  br label %sout.merge54
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
