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
  %arith.result = alloca ptr, align 8
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
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr4, align 4
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %y.load, i32 0, i32 0
  %tag6 = load i64, ptr %tag.ptr5, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag6, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr7, align 8
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %y.load, i32 0, i32 1
  %payload9 = load ptr, ptr %payload.ptr8, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload9 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %fadd = fadd double %left.double, %right.double
  %result.bits = bitcast double %fadd to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val10 = alloca %RCCValue, align 8
  %tag.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val10, i32 0, i32 0
  store i64 2, ptr %tag.ptr11, align 4
  %payload.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val10, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr12, align 8
  store ptr %rcc.val10, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %x.load, i32 0, i32 1
  %payload14 = load ptr, ptr %payload.ptr13, align 8
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %y.load, i32 0, i32 1
  %payload16 = load ptr, ptr %payload.ptr15, align 8
  %left.int = ptrtoint ptr %payload14 to i64
  %right.int = ptrtoint ptr %payload16 to i64
  %add = add i64 %left.int, %right.int
  %int.ptr = inttoptr i64 %add to ptr
  %rcc.val17 = alloca %RCCValue, align 8
  %tag.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val17, i32 0, i32 0
  store i64 1, ptr %tag.ptr18, align 4
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val17, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr19, align 8
  store ptr %rcc.val17, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  store ptr %arith.result.load, ptr %z, align 8
  %z.load = load ptr, ptr %z, align 8
  %tag.ptr20 = getelementptr inbounds nuw %RCCValue, ptr %z.load, i32 0, i32 0
  %tag21 = load i64, ptr %tag.ptr20, align 4
  %payload.ptr22 = getelementptr inbounds nuw %RCCValue, ptr %z.load, i32 0, i32 1
  %payload23 = load ptr, ptr %payload.ptr22, align 8
  switch i64 %tag21, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %arith.merge
  %int.val = ptrtoint ptr %payload23 to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %arith.merge
  %float.bits = ptrtoint ptr %payload23 to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %arith.merge
  %bool.val = ptrtoint ptr %payload23 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %arith.merge
  %4 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload23)
  br label %sout.merge

sout.null:                                        ; preds = %arith.merge
  %5 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %arith.merge
  %6 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload23)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val24 = alloca %RCCValue, align 8
  %tag.ptr25 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val24, i32 0, i32 0
  store i64 0, ptr %tag.ptr25, align 4
  %payload.ptr26 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val24, i32 0, i32 1
  store ptr null, ptr %payload.ptr26, align 8
  store ptr null, ptr %name, align 8
  %rcc.val27 = alloca %RCCValue, align 8
  %tag.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 0
  store i64 4, ptr %tag.ptr28, align 4
  %payload.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val27, i32 0, i32 1
  store ptr @.str.10, ptr %payload.ptr29, align 8
  store ptr %rcc.val27, ptr %name, align 8
  %name.load = load ptr, ptr %name, align 8
  %tag.ptr30 = getelementptr inbounds nuw %RCCValue, ptr %name.load, i32 0, i32 0
  %tag31 = load i64, ptr %tag.ptr30, align 4
  %payload.ptr32 = getelementptr inbounds nuw %RCCValue, ptr %name.load, i32 0, i32 1
  %payload33 = load ptr, ptr %payload.ptr32, align 8
  switch i64 %tag31, label %sout.default39 [
    i64 1, label %sout.int34
    i64 2, label %sout.float35
    i64 3, label %sout.bool36
    i64 4, label %sout.str37
    i64 0, label %sout.null38
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int34:                                       ; preds = %sout.merge
  %int.val41 = ptrtoint ptr %payload33 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.11, i64 %int.val41)
  br label %sout.merge40

sout.float35:                                     ; preds = %sout.merge
  %float.bits42 = ptrtoint ptr %payload33 to i64
  %float.val43 = bitcast i64 %float.bits42 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.12, double %float.val43)
  br label %sout.merge40

sout.bool36:                                      ; preds = %sout.merge
  %bool.val44 = ptrtoint ptr %payload33 to i64
  %is.true45 = icmp ne i64 %bool.val44, 0
  br i1 %is.true45, label %sout.true46, label %sout.false47

sout.str37:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr %payload33)
  br label %sout.merge40

sout.null38:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr @.str.19)
  br label %sout.merge40

sout.default39:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.20, ptr %payload33)
  br label %sout.merge40

sout.merge40:                                     ; preds = %sout.default39, %sout.null38, %sout.str37, %sout.bool.merge48, %sout.float35, %sout.int34
  %15 = call i32 @putchar(i32 10)
  %rcc.val49 = alloca %RCCValue, align 8
  %tag.ptr50 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val49, i32 0, i32 0
  store i64 0, ptr %tag.ptr50, align 4
  %payload.ptr51 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val49, i32 0, i32 1
  store ptr null, ptr %payload.ptr51, align 8
  store ptr null, ptr %flag, align 8
  %rcc.val52 = alloca %RCCValue, align 8
  %tag.ptr53 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val52, i32 0, i32 0
  store i64 3, ptr %tag.ptr53, align 4
  %payload.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val52, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr54, align 8
  store ptr %rcc.val52, ptr %flag, align 8
  %flag.load = load ptr, ptr %flag, align 8
  %tag.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %flag.load, i32 0, i32 0
  %tag56 = load i64, ptr %tag.ptr55, align 4
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %flag.load, i32 0, i32 1
  %payload58 = load ptr, ptr %payload.ptr57, align 8
  switch i64 %tag56, label %sout.default64 [
    i64 1, label %sout.int59
    i64 2, label %sout.float60
    i64 3, label %sout.bool61
    i64 4, label %sout.str62
    i64 0, label %sout.null63
  ]

sout.true46:                                      ; preds = %sout.bool36
  %16 = call i32 (ptr, ...) @printf(ptr @.str.13, ptr @.str.14)
  br label %sout.bool.merge48

sout.false47:                                     ; preds = %sout.bool36
  %17 = call i32 (ptr, ...) @printf(ptr @.str.15, ptr @.str.16)
  br label %sout.bool.merge48

sout.bool.merge48:                                ; preds = %sout.false47, %sout.true46
  br label %sout.merge40

sout.int59:                                       ; preds = %sout.merge40
  %int.val66 = ptrtoint ptr %payload58 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.21, i64 %int.val66)
  br label %sout.merge65

sout.float60:                                     ; preds = %sout.merge40
  %float.bits67 = ptrtoint ptr %payload58 to i64
  %float.val68 = bitcast i64 %float.bits67 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.22, double %float.val68)
  br label %sout.merge65

sout.bool61:                                      ; preds = %sout.merge40
  %bool.val69 = ptrtoint ptr %payload58 to i64
  %is.true70 = icmp ne i64 %bool.val69, 0
  br i1 %is.true70, label %sout.true71, label %sout.false72

sout.str62:                                       ; preds = %sout.merge40
  %20 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr %payload58)
  br label %sout.merge65

sout.null63:                                      ; preds = %sout.merge40
  %21 = call i32 (ptr, ...) @printf(ptr @.str.28, ptr @.str.29)
  br label %sout.merge65

sout.default64:                                   ; preds = %sout.merge40
  %22 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr %payload58)
  br label %sout.merge65

sout.merge65:                                     ; preds = %sout.default64, %sout.null63, %sout.str62, %sout.bool.merge73, %sout.float60, %sout.int59
  %23 = call i32 @putchar(i32 10)
  %rcc.val74 = alloca %RCCValue, align 8
  %tag.ptr75 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val74, i32 0, i32 0
  store i64 0, ptr %tag.ptr75, align 4
  %payload.ptr76 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val74, i32 0, i32 1
  store ptr null, ptr %payload.ptr76, align 8
  ret i32 0

sout.true71:                                      ; preds = %sout.bool61
  %24 = call i32 (ptr, ...) @printf(ptr @.str.23, ptr @.str.24)
  br label %sout.bool.merge73

sout.false72:                                     ; preds = %sout.bool61
  %25 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge73

sout.bool.merge73:                                ; preds = %sout.false72, %sout.true71
  br label %sout.merge65
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
