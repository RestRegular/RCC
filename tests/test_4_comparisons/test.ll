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
@.str.110 = private constant [5 x i8] c"%lld\00"
@.str.111 = private constant [3 x i8] c"%g\00"
@.str.112 = private constant [3 x i8] c"%s\00"
@.str.113 = private constant [5 x i8] c"true\00"
@.str.114 = private constant [3 x i8] c"%s\00"
@.str.115 = private constant [6 x i8] c"false\00"
@.str.116 = private constant [3 x i8] c"%s\00"
@.str.117 = private constant [3 x i8] c"%s\00"
@.str.118 = private constant [5 x i8] c"null\00"
@.str.119 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %b = alloca ptr, align 8
  %a = alloca ptr, align 8
  store ptr null, ptr %a, align 8
  %rcc.val = alloca %RCCValue, align 8
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr, align 8
  store ptr %rcc.val, ptr %a, align 8
  store ptr null, ptr %b, align 8
  %rcc.val1 = alloca %RCCValue, align 8
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val1, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr3, align 8
  store ptr %rcc.val1, ptr %b, align 8
  %a.load = load ptr, ptr %a, align 8
  %b.load = load ptr, ptr %b, align 8
  %payload.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr4, align 8
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload6 = load ptr, ptr %payload.ptr5, align 8
  %left.int = ptrtoint ptr %payload to i64
  %right.int = ptrtoint ptr %payload6 to i64
  %cmp.eq = icmp eq i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.eq to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val7 = alloca %RCCValue, align 8
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val7, i32 0, i32 0
  store i64 1, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val7, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr9, align 8
  %tag.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val7, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr10, align 4
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val7, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str, ptr %payload12)
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
  %a.load16 = load ptr, ptr %a, align 8
  %b.load17 = load ptr, ptr %b, align 8
  %payload.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %a.load16, i32 0, i32 1
  %payload19 = load ptr, ptr %payload.ptr18, align 8
  %payload.ptr20 = getelementptr inbounds nuw %RCCValue, ptr %b.load17, i32 0, i32 1
  %payload21 = load ptr, ptr %payload.ptr20, align 8
  %left.int22 = ptrtoint ptr %payload19 to i64
  %right.int23 = ptrtoint ptr %payload21 to i64
  %cmp.ne = icmp ne i64 %left.int22, %right.int23
  %cmp.ext24 = zext i1 %cmp.ne to i64
  %int.ptr25 = inttoptr i64 %cmp.ext24 to ptr
  %rcc.val26 = alloca %RCCValue, align 8
  %tag.ptr27 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val26, i32 0, i32 0
  store i64 1, ptr %tag.ptr27, align 4
  %payload.ptr28 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val26, i32 0, i32 1
  store ptr %int.ptr25, ptr %payload.ptr28, align 8
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val26, i32 0, i32 0
  %tag30 = load i64, ptr %tag.ptr29, align 4
  %payload.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val26, i32 0, i32 1
  %payload32 = load ptr, ptr %payload.ptr31, align 8
  switch i64 %tag30, label %sout.default38 [
    i64 1, label %sout.int33
    i64 2, label %sout.float34
    i64 3, label %sout.bool35
    i64 4, label %sout.str36
    i64 0, label %sout.null37
  ]

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int33:                                       ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, ptr %payload32)
  br label %sout.merge39

sout.float34:                                     ; preds = %sout.merge
  %float.bits40 = ptrtoint ptr %payload32 to i64
  %float.val41 = bitcast i64 %float.bits40 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val41)
  br label %sout.merge39

sout.bool35:                                      ; preds = %sout.merge
  %bool.val42 = ptrtoint ptr %payload32 to i64
  %is.true43 = icmp ne i64 %bool.val42, 0
  br i1 %is.true43, label %sout.true44, label %sout.false45

sout.str36:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload32)
  br label %sout.merge39

sout.null37:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge39

sout.default38:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload32)
  br label %sout.merge39

sout.merge39:                                     ; preds = %sout.default38, %sout.null37, %sout.str36, %sout.bool.merge46, %sout.float34, %sout.int33
  %15 = call i32 @putchar(i32 10)
  %rcc.val47 = alloca %RCCValue, align 8
  %tag.ptr48 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val47, i32 0, i32 0
  store i64 0, ptr %tag.ptr48, align 4
  %payload.ptr49 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val47, i32 0, i32 1
  store ptr null, ptr %payload.ptr49, align 8
  %a.load50 = load ptr, ptr %a, align 8
  %b.load51 = load ptr, ptr %b, align 8
  %payload.ptr52 = getelementptr inbounds nuw %RCCValue, ptr %a.load50, i32 0, i32 1
  %payload53 = load ptr, ptr %payload.ptr52, align 8
  %payload.ptr54 = getelementptr inbounds nuw %RCCValue, ptr %b.load51, i32 0, i32 1
  %payload55 = load ptr, ptr %payload.ptr54, align 8
  %left.int56 = ptrtoint ptr %payload53 to i64
  %right.int57 = ptrtoint ptr %payload55 to i64
  %cmp.lt = icmp slt i64 %left.int56, %right.int57
  %cmp.ext58 = zext i1 %cmp.lt to i64
  %int.ptr59 = inttoptr i64 %cmp.ext58 to ptr
  %rcc.val60 = alloca %RCCValue, align 8
  %tag.ptr61 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val60, i32 0, i32 0
  store i64 1, ptr %tag.ptr61, align 4
  %payload.ptr62 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val60, i32 0, i32 1
  store ptr %int.ptr59, ptr %payload.ptr62, align 8
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

sout.true44:                                      ; preds = %sout.bool35
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge46

sout.false45:                                     ; preds = %sout.bool35
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge46

sout.bool.merge46:                                ; preds = %sout.false45, %sout.true44
  br label %sout.merge39

sout.int67:                                       ; preds = %sout.merge39
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, ptr %payload66)
  br label %sout.merge73

sout.float68:                                     ; preds = %sout.merge39
  %float.bits74 = ptrtoint ptr %payload66 to i64
  %float.val75 = bitcast i64 %float.bits74 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val75)
  br label %sout.merge73

sout.bool69:                                      ; preds = %sout.merge39
  %bool.val76 = ptrtoint ptr %payload66 to i64
  %is.true77 = icmp ne i64 %bool.val76, 0
  br i1 %is.true77, label %sout.true78, label %sout.false79

sout.str70:                                       ; preds = %sout.merge39
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload66)
  br label %sout.merge73

sout.null71:                                      ; preds = %sout.merge39
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge73

sout.default72:                                   ; preds = %sout.merge39
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload66)
  br label %sout.merge73

sout.merge73:                                     ; preds = %sout.default72, %sout.null71, %sout.str70, %sout.bool.merge80, %sout.float68, %sout.int67
  %23 = call i32 @putchar(i32 10)
  %rcc.val81 = alloca %RCCValue, align 8
  %tag.ptr82 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 0
  store i64 0, ptr %tag.ptr82, align 4
  %payload.ptr83 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val81, i32 0, i32 1
  store ptr null, ptr %payload.ptr83, align 8
  %a.load84 = load ptr, ptr %a, align 8
  %b.load85 = load ptr, ptr %b, align 8
  %payload.ptr86 = getelementptr inbounds nuw %RCCValue, ptr %a.load84, i32 0, i32 1
  %payload87 = load ptr, ptr %payload.ptr86, align 8
  %payload.ptr88 = getelementptr inbounds nuw %RCCValue, ptr %b.load85, i32 0, i32 1
  %payload89 = load ptr, ptr %payload.ptr88, align 8
  %left.int90 = ptrtoint ptr %payload87 to i64
  %right.int91 = ptrtoint ptr %payload89 to i64
  %cmp.gt = icmp sgt i64 %left.int90, %right.int91
  %cmp.ext92 = zext i1 %cmp.gt to i64
  %int.ptr93 = inttoptr i64 %cmp.ext92 to ptr
  %rcc.val94 = alloca %RCCValue, align 8
  %tag.ptr95 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val94, i32 0, i32 0
  store i64 1, ptr %tag.ptr95, align 4
  %payload.ptr96 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val94, i32 0, i32 1
  store ptr %int.ptr93, ptr %payload.ptr96, align 8
  %tag.ptr97 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val94, i32 0, i32 0
  %tag98 = load i64, ptr %tag.ptr97, align 4
  %payload.ptr99 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val94, i32 0, i32 1
  %payload100 = load ptr, ptr %payload.ptr99, align 8
  switch i64 %tag98, label %sout.default106 [
    i64 1, label %sout.int101
    i64 2, label %sout.float102
    i64 3, label %sout.bool103
    i64 4, label %sout.str104
    i64 0, label %sout.null105
  ]

sout.true78:                                      ; preds = %sout.bool69
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge80

sout.false79:                                     ; preds = %sout.bool69
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge80

sout.bool.merge80:                                ; preds = %sout.false79, %sout.true78
  br label %sout.merge73

sout.int101:                                      ; preds = %sout.merge73
  %26 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr %payload100)
  br label %sout.merge107

sout.float102:                                    ; preds = %sout.merge73
  %float.bits108 = ptrtoint ptr %payload100 to i64
  %float.val109 = bitcast i64 %float.bits108 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.31, double %float.val109)
  br label %sout.merge107

sout.bool103:                                     ; preds = %sout.merge73
  %bool.val110 = ptrtoint ptr %payload100 to i64
  %is.true111 = icmp ne i64 %bool.val110, 0
  br i1 %is.true111, label %sout.true112, label %sout.false113

sout.str104:                                      ; preds = %sout.merge73
  %28 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr %payload100)
  br label %sout.merge107

sout.null105:                                     ; preds = %sout.merge73
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.merge107

sout.default106:                                  ; preds = %sout.merge73
  %30 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload100)
  br label %sout.merge107

sout.merge107:                                    ; preds = %sout.default106, %sout.null105, %sout.str104, %sout.bool.merge114, %sout.float102, %sout.int101
  %31 = call i32 @putchar(i32 10)
  %rcc.val115 = alloca %RCCValue, align 8
  %tag.ptr116 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val115, i32 0, i32 0
  store i64 0, ptr %tag.ptr116, align 4
  %payload.ptr117 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val115, i32 0, i32 1
  store ptr null, ptr %payload.ptr117, align 8
  %a.load118 = load ptr, ptr %a, align 8
  %rcc.val119 = alloca %RCCValue, align 8
  %tag.ptr120 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val119, i32 0, i32 0
  store i64 1, ptr %tag.ptr120, align 4
  %payload.ptr121 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val119, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr121, align 8
  %payload.ptr122 = getelementptr inbounds nuw %RCCValue, ptr %a.load118, i32 0, i32 1
  %payload123 = load ptr, ptr %payload.ptr122, align 8
  %payload.ptr124 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val119, i32 0, i32 1
  %payload125 = load ptr, ptr %payload.ptr124, align 8
  %left.int126 = ptrtoint ptr %payload123 to i64
  %right.int127 = ptrtoint ptr %payload125 to i64
  %cmp.le = icmp sle i64 %left.int126, %right.int127
  %cmp.ext128 = zext i1 %cmp.le to i64
  %int.ptr129 = inttoptr i64 %cmp.ext128 to ptr
  %rcc.val130 = alloca %RCCValue, align 8
  %tag.ptr131 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val130, i32 0, i32 0
  store i64 1, ptr %tag.ptr131, align 4
  %payload.ptr132 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val130, i32 0, i32 1
  store ptr %int.ptr129, ptr %payload.ptr132, align 8
  %tag.ptr133 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val130, i32 0, i32 0
  %tag134 = load i64, ptr %tag.ptr133, align 4
  %payload.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val130, i32 0, i32 1
  %payload136 = load ptr, ptr %payload.ptr135, align 8
  switch i64 %tag134, label %sout.default142 [
    i64 1, label %sout.int137
    i64 2, label %sout.float138
    i64 3, label %sout.bool139
    i64 4, label %sout.str140
    i64 0, label %sout.null141
  ]

sout.true112:                                     ; preds = %sout.bool103
  %32 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr @.str.33)
  br label %sout.bool.merge114

sout.false113:                                    ; preds = %sout.bool103
  %33 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr @.str.35)
  br label %sout.bool.merge114

sout.bool.merge114:                               ; preds = %sout.false113, %sout.true112
  br label %sout.merge107

sout.int137:                                      ; preds = %sout.merge107
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr %payload136)
  br label %sout.merge143

sout.float138:                                    ; preds = %sout.merge107
  %float.bits144 = ptrtoint ptr %payload136 to i64
  %float.val145 = bitcast i64 %float.bits144 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val145)
  br label %sout.merge143

sout.bool139:                                     ; preds = %sout.merge107
  %bool.val146 = ptrtoint ptr %payload136 to i64
  %is.true147 = icmp ne i64 %bool.val146, 0
  br i1 %is.true147, label %sout.true148, label %sout.false149

sout.str140:                                      ; preds = %sout.merge107
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload136)
  br label %sout.merge143

sout.null141:                                     ; preds = %sout.merge107
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge143

sout.default142:                                  ; preds = %sout.merge107
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload136)
  br label %sout.merge143

sout.merge143:                                    ; preds = %sout.default142, %sout.null141, %sout.str140, %sout.bool.merge150, %sout.float138, %sout.int137
  %39 = call i32 @putchar(i32 10)
  %rcc.val151 = alloca %RCCValue, align 8
  %tag.ptr152 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val151, i32 0, i32 0
  store i64 0, ptr %tag.ptr152, align 4
  %payload.ptr153 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val151, i32 0, i32 1
  store ptr null, ptr %payload.ptr153, align 8
  %a.load154 = load ptr, ptr %a, align 8
  %rcc.val155 = alloca %RCCValue, align 8
  %tag.ptr156 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val155, i32 0, i32 0
  store i64 1, ptr %tag.ptr156, align 4
  %payload.ptr157 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val155, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr157, align 8
  %payload.ptr158 = getelementptr inbounds nuw %RCCValue, ptr %a.load154, i32 0, i32 1
  %payload159 = load ptr, ptr %payload.ptr158, align 8
  %payload.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val155, i32 0, i32 1
  %payload161 = load ptr, ptr %payload.ptr160, align 8
  %left.int162 = ptrtoint ptr %payload159 to i64
  %right.int163 = ptrtoint ptr %payload161 to i64
  %cmp.ge = icmp sge i64 %left.int162, %right.int163
  %cmp.ext164 = zext i1 %cmp.ge to i64
  %int.ptr165 = inttoptr i64 %cmp.ext164 to ptr
  %rcc.val166 = alloca %RCCValue, align 8
  %tag.ptr167 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val166, i32 0, i32 0
  store i64 1, ptr %tag.ptr167, align 4
  %payload.ptr168 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val166, i32 0, i32 1
  store ptr %int.ptr165, ptr %payload.ptr168, align 8
  %tag.ptr169 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val166, i32 0, i32 0
  %tag170 = load i64, ptr %tag.ptr169, align 4
  %payload.ptr171 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val166, i32 0, i32 1
  %payload172 = load ptr, ptr %payload.ptr171, align 8
  switch i64 %tag170, label %sout.default178 [
    i64 1, label %sout.int173
    i64 2, label %sout.float174
    i64 3, label %sout.bool175
    i64 4, label %sout.str176
    i64 0, label %sout.null177
  ]

sout.true148:                                     ; preds = %sout.bool139
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge150

sout.false149:                                    ; preds = %sout.bool139
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge150

sout.bool.merge150:                               ; preds = %sout.false149, %sout.true148
  br label %sout.merge143

sout.int173:                                      ; preds = %sout.merge143
  %42 = call i32 (ptr, ...) @printf(ptr @.str.50, ptr %payload172)
  br label %sout.merge179

sout.float174:                                    ; preds = %sout.merge143
  %float.bits180 = ptrtoint ptr %payload172 to i64
  %float.val181 = bitcast i64 %float.bits180 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.51, double %float.val181)
  br label %sout.merge179

sout.bool175:                                     ; preds = %sout.merge143
  %bool.val182 = ptrtoint ptr %payload172 to i64
  %is.true183 = icmp ne i64 %bool.val182, 0
  br i1 %is.true183, label %sout.true184, label %sout.false185

sout.str176:                                      ; preds = %sout.merge143
  %44 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr %payload172)
  br label %sout.merge179

sout.null177:                                     ; preds = %sout.merge143
  %45 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr @.str.58)
  br label %sout.merge179

sout.default178:                                  ; preds = %sout.merge143
  %46 = call i32 (ptr, ...) @printf(ptr @.str.59, ptr %payload172)
  br label %sout.merge179

sout.merge179:                                    ; preds = %sout.default178, %sout.null177, %sout.str176, %sout.bool.merge186, %sout.float174, %sout.int173
  %47 = call i32 @putchar(i32 10)
  %rcc.val187 = alloca %RCCValue, align 8
  %tag.ptr188 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val187, i32 0, i32 0
  store i64 0, ptr %tag.ptr188, align 4
  %payload.ptr189 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val187, i32 0, i32 1
  store ptr null, ptr %payload.ptr189, align 8
  %rcc.val190 = alloca %RCCValue, align 8
  %tag.ptr191 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val190, i32 0, i32 0
  store i64 3, ptr %tag.ptr191, align 4
  %payload.ptr192 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val190, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr192, align 8
  %rcc.val193 = alloca %RCCValue, align 8
  %tag.ptr194 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val193, i32 0, i32 0
  store i64 3, ptr %tag.ptr194, align 4
  %payload.ptr195 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val193, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr195, align 8
  %payload.ptr196 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val190, i32 0, i32 1
  %payload197 = load ptr, ptr %payload.ptr196, align 8
  %payload.ptr198 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val193, i32 0, i32 1
  %payload199 = load ptr, ptr %payload.ptr198, align 8
  %left.int200 = ptrtoint ptr %payload197 to i64
  %right.int201 = ptrtoint ptr %payload199 to i64
  %tag.ptr202 = getelementptr inbounds nuw %RCCValue, ptr %payload199, i32 0, i32 0
  %tag203 = load i64, ptr %tag.ptr202, align 4
  %payload.ptr204 = getelementptr inbounds nuw %RCCValue, ptr %payload199, i32 0, i32 1
  %payload205 = load ptr, ptr %payload.ptr204, align 8
  %is_null = icmp eq i64 %tag203, 0
  %is_int = icmp eq i64 %tag203, 1
  %is_bool = icmp eq i64 %tag203, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload205 to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  %tag.ptr206 = getelementptr inbounds nuw %RCCValue, ptr %payload197, i32 0, i32 0
  %tag207 = load i64, ptr %tag.ptr206, align 4
  %payload.ptr208 = getelementptr inbounds nuw %RCCValue, ptr %payload197, i32 0, i32 1
  %payload209 = load ptr, ptr %payload.ptr208, align 8
  %is_null210 = icmp eq i64 %tag207, 0
  %is_int211 = icmp eq i64 %tag207, 1
  %is_bool212 = icmp eq i64 %tag207, 3
  %is_bool_or_int213 = or i1 %is_bool212, %is_int211
  %payload.int214 = ptrtoint ptr %payload209 to i64
  %payload_nz215 = icmp ne i64 %payload.int214, 0
  %not_null216 = xor i1 %is_null210, true
  %bool_int_result217 = select i1 %is_bool_or_int213, i1 %payload_nz215, i1 true
  %coerced_bool218 = select i1 %is_null210, i1 false, i1 %bool_int_result217
  %land = and i1 %coerced_bool218, %coerced_bool
  %logic.ext = zext i1 %land to i64
  %int.ptr219 = inttoptr i64 %logic.ext to ptr
  %rcc.val220 = alloca %RCCValue, align 8
  %tag.ptr221 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 0
  store i64 1, ptr %tag.ptr221, align 4
  %payload.ptr222 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 1
  store ptr %int.ptr219, ptr %payload.ptr222, align 8
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

sout.true184:                                     ; preds = %sout.bool175
  %48 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.bool.merge186

sout.false185:                                    ; preds = %sout.bool175
  %49 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr @.str.55)
  br label %sout.bool.merge186

sout.bool.merge186:                               ; preds = %sout.false185, %sout.true184
  br label %sout.merge179

sout.int227:                                      ; preds = %sout.merge179
  %50 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload226)
  br label %sout.merge233

sout.float228:                                    ; preds = %sout.merge179
  %float.bits234 = ptrtoint ptr %payload226 to i64
  %float.val235 = bitcast i64 %float.bits234 to double
  %51 = call i32 (ptr, ...) @printf(ptr @.str.61, double %float.val235)
  br label %sout.merge233

sout.bool229:                                     ; preds = %sout.merge179
  %bool.val236 = ptrtoint ptr %payload226 to i64
  %is.true237 = icmp ne i64 %bool.val236, 0
  br i1 %is.true237, label %sout.true238, label %sout.false239

sout.str230:                                      ; preds = %sout.merge179
  %52 = call i32 (ptr, ...) @printf(ptr @.str.66, ptr %payload226)
  br label %sout.merge233

sout.null231:                                     ; preds = %sout.merge179
  %53 = call i32 (ptr, ...) @printf(ptr @.str.67, ptr @.str.68)
  br label %sout.merge233

sout.default232:                                  ; preds = %sout.merge179
  %54 = call i32 (ptr, ...) @printf(ptr @.str.69, ptr %payload226)
  br label %sout.merge233

sout.merge233:                                    ; preds = %sout.default232, %sout.null231, %sout.str230, %sout.bool.merge240, %sout.float228, %sout.int227
  %55 = call i32 @putchar(i32 10)
  %rcc.val241 = alloca %RCCValue, align 8
  %tag.ptr242 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val241, i32 0, i32 0
  store i64 0, ptr %tag.ptr242, align 4
  %payload.ptr243 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val241, i32 0, i32 1
  store ptr null, ptr %payload.ptr243, align 8
  %rcc.val244 = alloca %RCCValue, align 8
  %tag.ptr245 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val244, i32 0, i32 0
  store i64 3, ptr %tag.ptr245, align 4
  %payload.ptr246 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val244, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr246, align 8
  %rcc.val247 = alloca %RCCValue, align 8
  %tag.ptr248 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val247, i32 0, i32 0
  store i64 3, ptr %tag.ptr248, align 4
  %payload.ptr249 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val247, i32 0, i32 1
  store ptr null, ptr %payload.ptr249, align 8
  %payload.ptr250 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val244, i32 0, i32 1
  %payload251 = load ptr, ptr %payload.ptr250, align 8
  %payload.ptr252 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val247, i32 0, i32 1
  %payload253 = load ptr, ptr %payload.ptr252, align 8
  %left.int254 = ptrtoint ptr %payload251 to i64
  %right.int255 = ptrtoint ptr %payload253 to i64
  %tag.ptr256 = getelementptr inbounds nuw %RCCValue, ptr %payload253, i32 0, i32 0
  %tag257 = load i64, ptr %tag.ptr256, align 4
  %payload.ptr258 = getelementptr inbounds nuw %RCCValue, ptr %payload253, i32 0, i32 1
  %payload259 = load ptr, ptr %payload.ptr258, align 8
  %is_null260 = icmp eq i64 %tag257, 0
  %is_int261 = icmp eq i64 %tag257, 1
  %is_bool262 = icmp eq i64 %tag257, 3
  %is_bool_or_int263 = or i1 %is_bool262, %is_int261
  %payload.int264 = ptrtoint ptr %payload259 to i64
  %payload_nz265 = icmp ne i64 %payload.int264, 0
  %not_null266 = xor i1 %is_null260, true
  %bool_int_result267 = select i1 %is_bool_or_int263, i1 %payload_nz265, i1 true
  %coerced_bool268 = select i1 %is_null260, i1 false, i1 %bool_int_result267
  %tag.ptr269 = getelementptr inbounds nuw %RCCValue, ptr %payload251, i32 0, i32 0
  %tag270 = load i64, ptr %tag.ptr269, align 4
  %payload.ptr271 = getelementptr inbounds nuw %RCCValue, ptr %payload251, i32 0, i32 1
  %payload272 = load ptr, ptr %payload.ptr271, align 8
  %is_null273 = icmp eq i64 %tag270, 0
  %is_int274 = icmp eq i64 %tag270, 1
  %is_bool275 = icmp eq i64 %tag270, 3
  %is_bool_or_int276 = or i1 %is_bool275, %is_int274
  %payload.int277 = ptrtoint ptr %payload272 to i64
  %payload_nz278 = icmp ne i64 %payload.int277, 0
  %not_null279 = xor i1 %is_null273, true
  %bool_int_result280 = select i1 %is_bool_or_int276, i1 %payload_nz278, i1 true
  %coerced_bool281 = select i1 %is_null273, i1 false, i1 %bool_int_result280
  %land282 = and i1 %coerced_bool281, %coerced_bool268
  %logic.ext283 = zext i1 %land282 to i64
  %int.ptr284 = inttoptr i64 %logic.ext283 to ptr
  %rcc.val285 = alloca %RCCValue, align 8
  %tag.ptr286 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val285, i32 0, i32 0
  store i64 1, ptr %tag.ptr286, align 4
  %payload.ptr287 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val285, i32 0, i32 1
  store ptr %int.ptr284, ptr %payload.ptr287, align 8
  %tag.ptr288 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val285, i32 0, i32 0
  %tag289 = load i64, ptr %tag.ptr288, align 4
  %payload.ptr290 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val285, i32 0, i32 1
  %payload291 = load ptr, ptr %payload.ptr290, align 8
  switch i64 %tag289, label %sout.default297 [
    i64 1, label %sout.int292
    i64 2, label %sout.float293
    i64 3, label %sout.bool294
    i64 4, label %sout.str295
    i64 0, label %sout.null296
  ]

sout.true238:                                     ; preds = %sout.bool229
  %56 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr @.str.63)
  br label %sout.bool.merge240

sout.false239:                                    ; preds = %sout.bool229
  %57 = call i32 (ptr, ...) @printf(ptr @.str.64, ptr @.str.65)
  br label %sout.bool.merge240

sout.bool.merge240:                               ; preds = %sout.false239, %sout.true238
  br label %sout.merge233

sout.int292:                                      ; preds = %sout.merge233
  %58 = call i32 (ptr, ...) @printf(ptr @.str.70, ptr %payload291)
  br label %sout.merge298

sout.float293:                                    ; preds = %sout.merge233
  %float.bits299 = ptrtoint ptr %payload291 to i64
  %float.val300 = bitcast i64 %float.bits299 to double
  %59 = call i32 (ptr, ...) @printf(ptr @.str.71, double %float.val300)
  br label %sout.merge298

sout.bool294:                                     ; preds = %sout.merge233
  %bool.val301 = ptrtoint ptr %payload291 to i64
  %is.true302 = icmp ne i64 %bool.val301, 0
  br i1 %is.true302, label %sout.true303, label %sout.false304

sout.str295:                                      ; preds = %sout.merge233
  %60 = call i32 (ptr, ...) @printf(ptr @.str.76, ptr %payload291)
  br label %sout.merge298

sout.null296:                                     ; preds = %sout.merge233
  %61 = call i32 (ptr, ...) @printf(ptr @.str.77, ptr @.str.78)
  br label %sout.merge298

sout.default297:                                  ; preds = %sout.merge233
  %62 = call i32 (ptr, ...) @printf(ptr @.str.79, ptr %payload291)
  br label %sout.merge298

sout.merge298:                                    ; preds = %sout.default297, %sout.null296, %sout.str295, %sout.bool.merge305, %sout.float293, %sout.int292
  %63 = call i32 @putchar(i32 10)
  %rcc.val306 = alloca %RCCValue, align 8
  %tag.ptr307 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val306, i32 0, i32 0
  store i64 0, ptr %tag.ptr307, align 4
  %payload.ptr308 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val306, i32 0, i32 1
  store ptr null, ptr %payload.ptr308, align 8
  %rcc.val309 = alloca %RCCValue, align 8
  %tag.ptr310 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val309, i32 0, i32 0
  store i64 3, ptr %tag.ptr310, align 4
  %payload.ptr311 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val309, i32 0, i32 1
  store ptr null, ptr %payload.ptr311, align 8
  %rcc.val312 = alloca %RCCValue, align 8
  %tag.ptr313 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val312, i32 0, i32 0
  store i64 3, ptr %tag.ptr313, align 4
  %payload.ptr314 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val312, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr314, align 8
  %payload.ptr315 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val309, i32 0, i32 1
  %payload316 = load ptr, ptr %payload.ptr315, align 8
  %payload.ptr317 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val312, i32 0, i32 1
  %payload318 = load ptr, ptr %payload.ptr317, align 8
  %left.int319 = ptrtoint ptr %payload316 to i64
  %right.int320 = ptrtoint ptr %payload318 to i64
  %tag.ptr321 = getelementptr inbounds nuw %RCCValue, ptr %payload318, i32 0, i32 0
  %tag322 = load i64, ptr %tag.ptr321, align 4
  %payload.ptr323 = getelementptr inbounds nuw %RCCValue, ptr %payload318, i32 0, i32 1
  %payload324 = load ptr, ptr %payload.ptr323, align 8
  %is_null325 = icmp eq i64 %tag322, 0
  %is_int326 = icmp eq i64 %tag322, 1
  %is_bool327 = icmp eq i64 %tag322, 3
  %is_bool_or_int328 = or i1 %is_bool327, %is_int326
  %payload.int329 = ptrtoint ptr %payload324 to i64
  %payload_nz330 = icmp ne i64 %payload.int329, 0
  %not_null331 = xor i1 %is_null325, true
  %bool_int_result332 = select i1 %is_bool_or_int328, i1 %payload_nz330, i1 true
  %coerced_bool333 = select i1 %is_null325, i1 false, i1 %bool_int_result332
  %tag.ptr334 = getelementptr inbounds nuw %RCCValue, ptr %payload316, i32 0, i32 0
  %tag335 = load i64, ptr %tag.ptr334, align 4
  %payload.ptr336 = getelementptr inbounds nuw %RCCValue, ptr %payload316, i32 0, i32 1
  %payload337 = load ptr, ptr %payload.ptr336, align 8
  %is_null338 = icmp eq i64 %tag335, 0
  %is_int339 = icmp eq i64 %tag335, 1
  %is_bool340 = icmp eq i64 %tag335, 3
  %is_bool_or_int341 = or i1 %is_bool340, %is_int339
  %payload.int342 = ptrtoint ptr %payload337 to i64
  %payload_nz343 = icmp ne i64 %payload.int342, 0
  %not_null344 = xor i1 %is_null338, true
  %bool_int_result345 = select i1 %is_bool_or_int341, i1 %payload_nz343, i1 true
  %coerced_bool346 = select i1 %is_null338, i1 false, i1 %bool_int_result345
  %lor = or i1 %coerced_bool346, %coerced_bool333
  %logic.ext347 = zext i1 %lor to i64
  %int.ptr348 = inttoptr i64 %logic.ext347 to ptr
  %rcc.val349 = alloca %RCCValue, align 8
  %tag.ptr350 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val349, i32 0, i32 0
  store i64 1, ptr %tag.ptr350, align 4
  %payload.ptr351 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val349, i32 0, i32 1
  store ptr %int.ptr348, ptr %payload.ptr351, align 8
  %tag.ptr352 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val349, i32 0, i32 0
  %tag353 = load i64, ptr %tag.ptr352, align 4
  %payload.ptr354 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val349, i32 0, i32 1
  %payload355 = load ptr, ptr %payload.ptr354, align 8
  switch i64 %tag353, label %sout.default361 [
    i64 1, label %sout.int356
    i64 2, label %sout.float357
    i64 3, label %sout.bool358
    i64 4, label %sout.str359
    i64 0, label %sout.null360
  ]

sout.true303:                                     ; preds = %sout.bool294
  %64 = call i32 (ptr, ...) @printf(ptr @.str.72, ptr @.str.73)
  br label %sout.bool.merge305

sout.false304:                                    ; preds = %sout.bool294
  %65 = call i32 (ptr, ...) @printf(ptr @.str.74, ptr @.str.75)
  br label %sout.bool.merge305

sout.bool.merge305:                               ; preds = %sout.false304, %sout.true303
  br label %sout.merge298

sout.int356:                                      ; preds = %sout.merge298
  %66 = call i32 (ptr, ...) @printf(ptr @.str.80, ptr %payload355)
  br label %sout.merge362

sout.float357:                                    ; preds = %sout.merge298
  %float.bits363 = ptrtoint ptr %payload355 to i64
  %float.val364 = bitcast i64 %float.bits363 to double
  %67 = call i32 (ptr, ...) @printf(ptr @.str.81, double %float.val364)
  br label %sout.merge362

sout.bool358:                                     ; preds = %sout.merge298
  %bool.val365 = ptrtoint ptr %payload355 to i64
  %is.true366 = icmp ne i64 %bool.val365, 0
  br i1 %is.true366, label %sout.true367, label %sout.false368

sout.str359:                                      ; preds = %sout.merge298
  %68 = call i32 (ptr, ...) @printf(ptr @.str.86, ptr %payload355)
  br label %sout.merge362

sout.null360:                                     ; preds = %sout.merge298
  %69 = call i32 (ptr, ...) @printf(ptr @.str.87, ptr @.str.88)
  br label %sout.merge362

sout.default361:                                  ; preds = %sout.merge298
  %70 = call i32 (ptr, ...) @printf(ptr @.str.89, ptr %payload355)
  br label %sout.merge362

sout.merge362:                                    ; preds = %sout.default361, %sout.null360, %sout.str359, %sout.bool.merge369, %sout.float357, %sout.int356
  %71 = call i32 @putchar(i32 10)
  %rcc.val370 = alloca %RCCValue, align 8
  %tag.ptr371 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val370, i32 0, i32 0
  store i64 0, ptr %tag.ptr371, align 4
  %payload.ptr372 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val370, i32 0, i32 1
  store ptr null, ptr %payload.ptr372, align 8
  %rcc.val373 = alloca %RCCValue, align 8
  %tag.ptr374 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val373, i32 0, i32 0
  store i64 3, ptr %tag.ptr374, align 4
  %payload.ptr375 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val373, i32 0, i32 1
  store ptr null, ptr %payload.ptr375, align 8
  %rcc.val376 = alloca %RCCValue, align 8
  %tag.ptr377 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val376, i32 0, i32 0
  store i64 3, ptr %tag.ptr377, align 4
  %payload.ptr378 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val376, i32 0, i32 1
  store ptr null, ptr %payload.ptr378, align 8
  %payload.ptr379 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val373, i32 0, i32 1
  %payload380 = load ptr, ptr %payload.ptr379, align 8
  %payload.ptr381 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val376, i32 0, i32 1
  %payload382 = load ptr, ptr %payload.ptr381, align 8
  %left.int383 = ptrtoint ptr %payload380 to i64
  %right.int384 = ptrtoint ptr %payload382 to i64
  %tag.ptr385 = getelementptr inbounds nuw %RCCValue, ptr %payload382, i32 0, i32 0
  %tag386 = load i64, ptr %tag.ptr385, align 4
  %payload.ptr387 = getelementptr inbounds nuw %RCCValue, ptr %payload382, i32 0, i32 1
  %payload388 = load ptr, ptr %payload.ptr387, align 8
  %is_null389 = icmp eq i64 %tag386, 0
  %is_int390 = icmp eq i64 %tag386, 1
  %is_bool391 = icmp eq i64 %tag386, 3
  %is_bool_or_int392 = or i1 %is_bool391, %is_int390
  %payload.int393 = ptrtoint ptr %payload388 to i64
  %payload_nz394 = icmp ne i64 %payload.int393, 0
  %not_null395 = xor i1 %is_null389, true
  %bool_int_result396 = select i1 %is_bool_or_int392, i1 %payload_nz394, i1 true
  %coerced_bool397 = select i1 %is_null389, i1 false, i1 %bool_int_result396
  %tag.ptr398 = getelementptr inbounds nuw %RCCValue, ptr %payload380, i32 0, i32 0
  %tag399 = load i64, ptr %tag.ptr398, align 4
  %payload.ptr400 = getelementptr inbounds nuw %RCCValue, ptr %payload380, i32 0, i32 1
  %payload401 = load ptr, ptr %payload.ptr400, align 8
  %is_null402 = icmp eq i64 %tag399, 0
  %is_int403 = icmp eq i64 %tag399, 1
  %is_bool404 = icmp eq i64 %tag399, 3
  %is_bool_or_int405 = or i1 %is_bool404, %is_int403
  %payload.int406 = ptrtoint ptr %payload401 to i64
  %payload_nz407 = icmp ne i64 %payload.int406, 0
  %not_null408 = xor i1 %is_null402, true
  %bool_int_result409 = select i1 %is_bool_or_int405, i1 %payload_nz407, i1 true
  %coerced_bool410 = select i1 %is_null402, i1 false, i1 %bool_int_result409
  %lor411 = or i1 %coerced_bool410, %coerced_bool397
  %logic.ext412 = zext i1 %lor411 to i64
  %int.ptr413 = inttoptr i64 %logic.ext412 to ptr
  %rcc.val414 = alloca %RCCValue, align 8
  %tag.ptr415 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val414, i32 0, i32 0
  store i64 1, ptr %tag.ptr415, align 4
  %payload.ptr416 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val414, i32 0, i32 1
  store ptr %int.ptr413, ptr %payload.ptr416, align 8
  %tag.ptr417 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val414, i32 0, i32 0
  %tag418 = load i64, ptr %tag.ptr417, align 4
  %payload.ptr419 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val414, i32 0, i32 1
  %payload420 = load ptr, ptr %payload.ptr419, align 8
  switch i64 %tag418, label %sout.default426 [
    i64 1, label %sout.int421
    i64 2, label %sout.float422
    i64 3, label %sout.bool423
    i64 4, label %sout.str424
    i64 0, label %sout.null425
  ]

sout.true367:                                     ; preds = %sout.bool358
  %72 = call i32 (ptr, ...) @printf(ptr @.str.82, ptr @.str.83)
  br label %sout.bool.merge369

sout.false368:                                    ; preds = %sout.bool358
  %73 = call i32 (ptr, ...) @printf(ptr @.str.84, ptr @.str.85)
  br label %sout.bool.merge369

sout.bool.merge369:                               ; preds = %sout.false368, %sout.true367
  br label %sout.merge362

sout.int421:                                      ; preds = %sout.merge362
  %74 = call i32 (ptr, ...) @printf(ptr @.str.90, ptr %payload420)
  br label %sout.merge427

sout.float422:                                    ; preds = %sout.merge362
  %float.bits428 = ptrtoint ptr %payload420 to i64
  %float.val429 = bitcast i64 %float.bits428 to double
  %75 = call i32 (ptr, ...) @printf(ptr @.str.91, double %float.val429)
  br label %sout.merge427

sout.bool423:                                     ; preds = %sout.merge362
  %bool.val430 = ptrtoint ptr %payload420 to i64
  %is.true431 = icmp ne i64 %bool.val430, 0
  br i1 %is.true431, label %sout.true432, label %sout.false433

sout.str424:                                      ; preds = %sout.merge362
  %76 = call i32 (ptr, ...) @printf(ptr @.str.96, ptr %payload420)
  br label %sout.merge427

sout.null425:                                     ; preds = %sout.merge362
  %77 = call i32 (ptr, ...) @printf(ptr @.str.97, ptr @.str.98)
  br label %sout.merge427

sout.default426:                                  ; preds = %sout.merge362
  %78 = call i32 (ptr, ...) @printf(ptr @.str.99, ptr %payload420)
  br label %sout.merge427

sout.merge427:                                    ; preds = %sout.default426, %sout.null425, %sout.str424, %sout.bool.merge434, %sout.float422, %sout.int421
  %79 = call i32 @putchar(i32 10)
  %rcc.val435 = alloca %RCCValue, align 8
  %tag.ptr436 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val435, i32 0, i32 0
  store i64 0, ptr %tag.ptr436, align 4
  %payload.ptr437 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val435, i32 0, i32 1
  store ptr null, ptr %payload.ptr437, align 8
  %rcc.val438 = alloca %RCCValue, align 8
  %tag.ptr439 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val438, i32 0, i32 0
  store i64 3, ptr %tag.ptr439, align 4
  %payload.ptr440 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val438, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr440, align 8
  %unary.int = ptrtoint ptr %rcc.val438 to i64
  %tag.ptr441 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val438, i32 0, i32 0
  %tag442 = load i64, ptr %tag.ptr441, align 4
  %payload.ptr443 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val438, i32 0, i32 1
  %payload444 = load ptr, ptr %payload.ptr443, align 8
  %is_null445 = icmp eq i64 %tag442, 0
  %is_int446 = icmp eq i64 %tag442, 1
  %is_bool447 = icmp eq i64 %tag442, 3
  %is_bool_or_int448 = or i1 %is_bool447, %is_int446
  %payload.int449 = ptrtoint ptr %payload444 to i64
  %payload_nz450 = icmp ne i64 %payload.int449, 0
  %not_null451 = xor i1 %is_null445, true
  %bool_int_result452 = select i1 %is_bool_or_int448, i1 %payload_nz450, i1 true
  %coerced_bool453 = select i1 %is_null445, i1 false, i1 %bool_int_result452
  %not = xor i1 %coerced_bool453, true
  %not.ext = zext i1 %not to i64
  %not.is_true = icmp ne i64 %not.ext, 0
  %not.payload = select i1 %not.is_true, ptr inttoptr (i64 1 to ptr), ptr null
  %rcc.val454 = alloca %RCCValue, align 8
  %tag.ptr455 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val454, i32 0, i32 0
  store i64 3, ptr %tag.ptr455, align 4
  %payload.ptr456 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val454, i32 0, i32 1
  store ptr %not.payload, ptr %payload.ptr456, align 8
  %tag.ptr457 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val454, i32 0, i32 0
  %tag458 = load i64, ptr %tag.ptr457, align 4
  %payload.ptr459 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val454, i32 0, i32 1
  %payload460 = load ptr, ptr %payload.ptr459, align 8
  switch i64 %tag458, label %sout.default466 [
    i64 1, label %sout.int461
    i64 2, label %sout.float462
    i64 3, label %sout.bool463
    i64 4, label %sout.str464
    i64 0, label %sout.null465
  ]

sout.true432:                                     ; preds = %sout.bool423
  %80 = call i32 (ptr, ...) @printf(ptr @.str.92, ptr @.str.93)
  br label %sout.bool.merge434

sout.false433:                                    ; preds = %sout.bool423
  %81 = call i32 (ptr, ...) @printf(ptr @.str.94, ptr @.str.95)
  br label %sout.bool.merge434

sout.bool.merge434:                               ; preds = %sout.false433, %sout.true432
  br label %sout.merge427

sout.int461:                                      ; preds = %sout.merge427
  %82 = call i32 (ptr, ...) @printf(ptr @.str.100, ptr %payload460)
  br label %sout.merge467

sout.float462:                                    ; preds = %sout.merge427
  %float.bits468 = ptrtoint ptr %payload460 to i64
  %float.val469 = bitcast i64 %float.bits468 to double
  %83 = call i32 (ptr, ...) @printf(ptr @.str.101, double %float.val469)
  br label %sout.merge467

sout.bool463:                                     ; preds = %sout.merge427
  %bool.val470 = ptrtoint ptr %payload460 to i64
  %is.true471 = icmp ne i64 %bool.val470, 0
  br i1 %is.true471, label %sout.true472, label %sout.false473

sout.str464:                                      ; preds = %sout.merge427
  %84 = call i32 (ptr, ...) @printf(ptr @.str.106, ptr %payload460)
  br label %sout.merge467

sout.null465:                                     ; preds = %sout.merge427
  %85 = call i32 (ptr, ...) @printf(ptr @.str.107, ptr @.str.108)
  br label %sout.merge467

sout.default466:                                  ; preds = %sout.merge427
  %86 = call i32 (ptr, ...) @printf(ptr @.str.109, ptr %payload460)
  br label %sout.merge467

sout.merge467:                                    ; preds = %sout.default466, %sout.null465, %sout.str464, %sout.bool.merge474, %sout.float462, %sout.int461
  %87 = call i32 @putchar(i32 10)
  %rcc.val475 = alloca %RCCValue, align 8
  %tag.ptr476 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val475, i32 0, i32 0
  store i64 0, ptr %tag.ptr476, align 4
  %payload.ptr477 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val475, i32 0, i32 1
  store ptr null, ptr %payload.ptr477, align 8
  %rcc.val478 = alloca %RCCValue, align 8
  %tag.ptr479 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val478, i32 0, i32 0
  store i64 3, ptr %tag.ptr479, align 4
  %payload.ptr480 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val478, i32 0, i32 1
  store ptr null, ptr %payload.ptr480, align 8
  %unary.int481 = ptrtoint ptr %rcc.val478 to i64
  %tag.ptr482 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val478, i32 0, i32 0
  %tag483 = load i64, ptr %tag.ptr482, align 4
  %payload.ptr484 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val478, i32 0, i32 1
  %payload485 = load ptr, ptr %payload.ptr484, align 8
  %is_null486 = icmp eq i64 %tag483, 0
  %is_int487 = icmp eq i64 %tag483, 1
  %is_bool488 = icmp eq i64 %tag483, 3
  %is_bool_or_int489 = or i1 %is_bool488, %is_int487
  %payload.int490 = ptrtoint ptr %payload485 to i64
  %payload_nz491 = icmp ne i64 %payload.int490, 0
  %not_null492 = xor i1 %is_null486, true
  %bool_int_result493 = select i1 %is_bool_or_int489, i1 %payload_nz491, i1 true
  %coerced_bool494 = select i1 %is_null486, i1 false, i1 %bool_int_result493
  %not495 = xor i1 %coerced_bool494, true
  %not.ext496 = zext i1 %not495 to i64
  %not.is_true497 = icmp ne i64 %not.ext496, 0
  %not.payload498 = select i1 %not.is_true497, ptr inttoptr (i64 1 to ptr), ptr null
  %rcc.val499 = alloca %RCCValue, align 8
  %tag.ptr500 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val499, i32 0, i32 0
  store i64 3, ptr %tag.ptr500, align 4
  %payload.ptr501 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val499, i32 0, i32 1
  store ptr %not.payload498, ptr %payload.ptr501, align 8
  %tag.ptr502 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val499, i32 0, i32 0
  %tag503 = load i64, ptr %tag.ptr502, align 4
  %payload.ptr504 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val499, i32 0, i32 1
  %payload505 = load ptr, ptr %payload.ptr504, align 8
  switch i64 %tag503, label %sout.default511 [
    i64 1, label %sout.int506
    i64 2, label %sout.float507
    i64 3, label %sout.bool508
    i64 4, label %sout.str509
    i64 0, label %sout.null510
  ]

sout.true472:                                     ; preds = %sout.bool463
  %88 = call i32 (ptr, ...) @printf(ptr @.str.102, ptr @.str.103)
  br label %sout.bool.merge474

sout.false473:                                    ; preds = %sout.bool463
  %89 = call i32 (ptr, ...) @printf(ptr @.str.104, ptr @.str.105)
  br label %sout.bool.merge474

sout.bool.merge474:                               ; preds = %sout.false473, %sout.true472
  br label %sout.merge467

sout.int506:                                      ; preds = %sout.merge467
  %90 = call i32 (ptr, ...) @printf(ptr @.str.110, ptr %payload505)
  br label %sout.merge512

sout.float507:                                    ; preds = %sout.merge467
  %float.bits513 = ptrtoint ptr %payload505 to i64
  %float.val514 = bitcast i64 %float.bits513 to double
  %91 = call i32 (ptr, ...) @printf(ptr @.str.111, double %float.val514)
  br label %sout.merge512

sout.bool508:                                     ; preds = %sout.merge467
  %bool.val515 = ptrtoint ptr %payload505 to i64
  %is.true516 = icmp ne i64 %bool.val515, 0
  br i1 %is.true516, label %sout.true517, label %sout.false518

sout.str509:                                      ; preds = %sout.merge467
  %92 = call i32 (ptr, ...) @printf(ptr @.str.116, ptr %payload505)
  br label %sout.merge512

sout.null510:                                     ; preds = %sout.merge467
  %93 = call i32 (ptr, ...) @printf(ptr @.str.117, ptr @.str.118)
  br label %sout.merge512

sout.default511:                                  ; preds = %sout.merge467
  %94 = call i32 (ptr, ...) @printf(ptr @.str.119, ptr %payload505)
  br label %sout.merge512

sout.merge512:                                    ; preds = %sout.default511, %sout.null510, %sout.str509, %sout.bool.merge519, %sout.float507, %sout.int506
  %95 = call i32 @putchar(i32 10)
  %rcc.val520 = alloca %RCCValue, align 8
  %tag.ptr521 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val520, i32 0, i32 0
  store i64 0, ptr %tag.ptr521, align 4
  %payload.ptr522 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val520, i32 0, i32 1
  store ptr null, ptr %payload.ptr522, align 8
  ret i32 0

sout.true517:                                     ; preds = %sout.bool508
  %96 = call i32 (ptr, ...) @printf(ptr @.str.112, ptr @.str.113)
  br label %sout.bool.merge519

sout.false518:                                    ; preds = %sout.bool508
  %97 = call i32 (ptr, ...) @printf(ptr @.str.114, ptr @.str.115)
  br label %sout.bool.merge519

sout.bool.merge519:                               ; preds = %sout.false518, %sout.true517
  br label %sout.merge512
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
