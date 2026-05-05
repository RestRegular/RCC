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
  store i64 3, ptr %tag.ptr8, align 4
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
  store i64 3, ptr %tag.ptr27, align 4
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
  %int.val40 = ptrtoint ptr %payload32 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val40)
  br label %sout.merge39

sout.float34:                                     ; preds = %sout.merge
  %float.bits41 = ptrtoint ptr %payload32 to i64
  %float.val42 = bitcast i64 %float.bits41 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val42)
  br label %sout.merge39

sout.bool35:                                      ; preds = %sout.merge
  %bool.val43 = ptrtoint ptr %payload32 to i64
  %is.true44 = icmp ne i64 %bool.val43, 0
  br i1 %is.true44, label %sout.true45, label %sout.false46

sout.str36:                                       ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload32)
  br label %sout.merge39

sout.null37:                                      ; preds = %sout.merge
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge39

sout.default38:                                   ; preds = %sout.merge
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload32)
  br label %sout.merge39

sout.merge39:                                     ; preds = %sout.default38, %sout.null37, %sout.str36, %sout.bool.merge47, %sout.float34, %sout.int33
  %15 = call i32 @putchar(i32 10)
  %rcc.val48 = alloca %RCCValue, align 8
  %tag.ptr49 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val48, i32 0, i32 0
  store i64 0, ptr %tag.ptr49, align 4
  %payload.ptr50 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val48, i32 0, i32 1
  store ptr null, ptr %payload.ptr50, align 8
  %a.load51 = load ptr, ptr %a, align 8
  %b.load52 = load ptr, ptr %b, align 8
  %payload.ptr53 = getelementptr inbounds nuw %RCCValue, ptr %a.load51, i32 0, i32 1
  %payload54 = load ptr, ptr %payload.ptr53, align 8
  %payload.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %b.load52, i32 0, i32 1
  %payload56 = load ptr, ptr %payload.ptr55, align 8
  %left.int57 = ptrtoint ptr %payload54 to i64
  %right.int58 = ptrtoint ptr %payload56 to i64
  %cmp.lt = icmp slt i64 %left.int57, %right.int58
  %cmp.ext59 = zext i1 %cmp.lt to i64
  %int.ptr60 = inttoptr i64 %cmp.ext59 to ptr
  %rcc.val61 = alloca %RCCValue, align 8
  %tag.ptr62 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val61, i32 0, i32 0
  store i64 3, ptr %tag.ptr62, align 4
  %payload.ptr63 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val61, i32 0, i32 1
  store ptr %int.ptr60, ptr %payload.ptr63, align 8
  %tag.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val61, i32 0, i32 0
  %tag65 = load i64, ptr %tag.ptr64, align 4
  %payload.ptr66 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val61, i32 0, i32 1
  %payload67 = load ptr, ptr %payload.ptr66, align 8
  switch i64 %tag65, label %sout.default73 [
    i64 1, label %sout.int68
    i64 2, label %sout.float69
    i64 3, label %sout.bool70
    i64 4, label %sout.str71
    i64 0, label %sout.null72
  ]

sout.true45:                                      ; preds = %sout.bool35
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge47

sout.false46:                                     ; preds = %sout.bool35
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge47

sout.bool.merge47:                                ; preds = %sout.false46, %sout.true45
  br label %sout.merge39

sout.int68:                                       ; preds = %sout.merge39
  %int.val75 = ptrtoint ptr %payload67 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val75)
  br label %sout.merge74

sout.float69:                                     ; preds = %sout.merge39
  %float.bits76 = ptrtoint ptr %payload67 to i64
  %float.val77 = bitcast i64 %float.bits76 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val77)
  br label %sout.merge74

sout.bool70:                                      ; preds = %sout.merge39
  %bool.val78 = ptrtoint ptr %payload67 to i64
  %is.true79 = icmp ne i64 %bool.val78, 0
  br i1 %is.true79, label %sout.true80, label %sout.false81

sout.str71:                                       ; preds = %sout.merge39
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload67)
  br label %sout.merge74

sout.null72:                                      ; preds = %sout.merge39
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge74

sout.default73:                                   ; preds = %sout.merge39
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload67)
  br label %sout.merge74

sout.merge74:                                     ; preds = %sout.default73, %sout.null72, %sout.str71, %sout.bool.merge82, %sout.float69, %sout.int68
  %23 = call i32 @putchar(i32 10)
  %rcc.val83 = alloca %RCCValue, align 8
  %tag.ptr84 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val83, i32 0, i32 0
  store i64 0, ptr %tag.ptr84, align 4
  %payload.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val83, i32 0, i32 1
  store ptr null, ptr %payload.ptr85, align 8
  %a.load86 = load ptr, ptr %a, align 8
  %b.load87 = load ptr, ptr %b, align 8
  %payload.ptr88 = getelementptr inbounds nuw %RCCValue, ptr %a.load86, i32 0, i32 1
  %payload89 = load ptr, ptr %payload.ptr88, align 8
  %payload.ptr90 = getelementptr inbounds nuw %RCCValue, ptr %b.load87, i32 0, i32 1
  %payload91 = load ptr, ptr %payload.ptr90, align 8
  %left.int92 = ptrtoint ptr %payload89 to i64
  %right.int93 = ptrtoint ptr %payload91 to i64
  %cmp.gt = icmp sgt i64 %left.int92, %right.int93
  %cmp.ext94 = zext i1 %cmp.gt to i64
  %int.ptr95 = inttoptr i64 %cmp.ext94 to ptr
  %rcc.val96 = alloca %RCCValue, align 8
  %tag.ptr97 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val96, i32 0, i32 0
  store i64 3, ptr %tag.ptr97, align 4
  %payload.ptr98 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val96, i32 0, i32 1
  store ptr %int.ptr95, ptr %payload.ptr98, align 8
  %tag.ptr99 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val96, i32 0, i32 0
  %tag100 = load i64, ptr %tag.ptr99, align 4
  %payload.ptr101 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val96, i32 0, i32 1
  %payload102 = load ptr, ptr %payload.ptr101, align 8
  switch i64 %tag100, label %sout.default108 [
    i64 1, label %sout.int103
    i64 2, label %sout.float104
    i64 3, label %sout.bool105
    i64 4, label %sout.str106
    i64 0, label %sout.null107
  ]

sout.true80:                                      ; preds = %sout.bool70
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge82

sout.false81:                                     ; preds = %sout.bool70
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge82

sout.bool.merge82:                                ; preds = %sout.false81, %sout.true80
  br label %sout.merge74

sout.int103:                                      ; preds = %sout.merge74
  %int.val110 = ptrtoint ptr %payload102 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.30, i64 %int.val110)
  br label %sout.merge109

sout.float104:                                    ; preds = %sout.merge74
  %float.bits111 = ptrtoint ptr %payload102 to i64
  %float.val112 = bitcast i64 %float.bits111 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.31, double %float.val112)
  br label %sout.merge109

sout.bool105:                                     ; preds = %sout.merge74
  %bool.val113 = ptrtoint ptr %payload102 to i64
  %is.true114 = icmp ne i64 %bool.val113, 0
  br i1 %is.true114, label %sout.true115, label %sout.false116

sout.str106:                                      ; preds = %sout.merge74
  %28 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr %payload102)
  br label %sout.merge109

sout.null107:                                     ; preds = %sout.merge74
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.merge109

sout.default108:                                  ; preds = %sout.merge74
  %30 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload102)
  br label %sout.merge109

sout.merge109:                                    ; preds = %sout.default108, %sout.null107, %sout.str106, %sout.bool.merge117, %sout.float104, %sout.int103
  %31 = call i32 @putchar(i32 10)
  %rcc.val118 = alloca %RCCValue, align 8
  %tag.ptr119 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val118, i32 0, i32 0
  store i64 0, ptr %tag.ptr119, align 4
  %payload.ptr120 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val118, i32 0, i32 1
  store ptr null, ptr %payload.ptr120, align 8
  %a.load121 = load ptr, ptr %a, align 8
  %rcc.val122 = alloca %RCCValue, align 8
  %tag.ptr123 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val122, i32 0, i32 0
  store i64 1, ptr %tag.ptr123, align 4
  %payload.ptr124 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val122, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr124, align 8
  %payload.ptr125 = getelementptr inbounds nuw %RCCValue, ptr %a.load121, i32 0, i32 1
  %payload126 = load ptr, ptr %payload.ptr125, align 8
  %payload.ptr127 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val122, i32 0, i32 1
  %payload128 = load ptr, ptr %payload.ptr127, align 8
  %left.int129 = ptrtoint ptr %payload126 to i64
  %right.int130 = ptrtoint ptr %payload128 to i64
  %cmp.le = icmp sle i64 %left.int129, %right.int130
  %cmp.ext131 = zext i1 %cmp.le to i64
  %int.ptr132 = inttoptr i64 %cmp.ext131 to ptr
  %rcc.val133 = alloca %RCCValue, align 8
  %tag.ptr134 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val133, i32 0, i32 0
  store i64 3, ptr %tag.ptr134, align 4
  %payload.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val133, i32 0, i32 1
  store ptr %int.ptr132, ptr %payload.ptr135, align 8
  %tag.ptr136 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val133, i32 0, i32 0
  %tag137 = load i64, ptr %tag.ptr136, align 4
  %payload.ptr138 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val133, i32 0, i32 1
  %payload139 = load ptr, ptr %payload.ptr138, align 8
  switch i64 %tag137, label %sout.default145 [
    i64 1, label %sout.int140
    i64 2, label %sout.float141
    i64 3, label %sout.bool142
    i64 4, label %sout.str143
    i64 0, label %sout.null144
  ]

sout.true115:                                     ; preds = %sout.bool105
  %32 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr @.str.33)
  br label %sout.bool.merge117

sout.false116:                                    ; preds = %sout.bool105
  %33 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr @.str.35)
  br label %sout.bool.merge117

sout.bool.merge117:                               ; preds = %sout.false116, %sout.true115
  br label %sout.merge109

sout.int140:                                      ; preds = %sout.merge109
  %int.val147 = ptrtoint ptr %payload139 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, i64 %int.val147)
  br label %sout.merge146

sout.float141:                                    ; preds = %sout.merge109
  %float.bits148 = ptrtoint ptr %payload139 to i64
  %float.val149 = bitcast i64 %float.bits148 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val149)
  br label %sout.merge146

sout.bool142:                                     ; preds = %sout.merge109
  %bool.val150 = ptrtoint ptr %payload139 to i64
  %is.true151 = icmp ne i64 %bool.val150, 0
  br i1 %is.true151, label %sout.true152, label %sout.false153

sout.str143:                                      ; preds = %sout.merge109
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload139)
  br label %sout.merge146

sout.null144:                                     ; preds = %sout.merge109
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge146

sout.default145:                                  ; preds = %sout.merge109
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload139)
  br label %sout.merge146

sout.merge146:                                    ; preds = %sout.default145, %sout.null144, %sout.str143, %sout.bool.merge154, %sout.float141, %sout.int140
  %39 = call i32 @putchar(i32 10)
  %rcc.val155 = alloca %RCCValue, align 8
  %tag.ptr156 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val155, i32 0, i32 0
  store i64 0, ptr %tag.ptr156, align 4
  %payload.ptr157 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val155, i32 0, i32 1
  store ptr null, ptr %payload.ptr157, align 8
  %a.load158 = load ptr, ptr %a, align 8
  %rcc.val159 = alloca %RCCValue, align 8
  %tag.ptr160 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val159, i32 0, i32 0
  store i64 1, ptr %tag.ptr160, align 4
  %payload.ptr161 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val159, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr161, align 8
  %payload.ptr162 = getelementptr inbounds nuw %RCCValue, ptr %a.load158, i32 0, i32 1
  %payload163 = load ptr, ptr %payload.ptr162, align 8
  %payload.ptr164 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val159, i32 0, i32 1
  %payload165 = load ptr, ptr %payload.ptr164, align 8
  %left.int166 = ptrtoint ptr %payload163 to i64
  %right.int167 = ptrtoint ptr %payload165 to i64
  %cmp.ge = icmp sge i64 %left.int166, %right.int167
  %cmp.ext168 = zext i1 %cmp.ge to i64
  %int.ptr169 = inttoptr i64 %cmp.ext168 to ptr
  %rcc.val170 = alloca %RCCValue, align 8
  %tag.ptr171 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val170, i32 0, i32 0
  store i64 3, ptr %tag.ptr171, align 4
  %payload.ptr172 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val170, i32 0, i32 1
  store ptr %int.ptr169, ptr %payload.ptr172, align 8
  %tag.ptr173 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val170, i32 0, i32 0
  %tag174 = load i64, ptr %tag.ptr173, align 4
  %payload.ptr175 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val170, i32 0, i32 1
  %payload176 = load ptr, ptr %payload.ptr175, align 8
  switch i64 %tag174, label %sout.default182 [
    i64 1, label %sout.int177
    i64 2, label %sout.float178
    i64 3, label %sout.bool179
    i64 4, label %sout.str180
    i64 0, label %sout.null181
  ]

sout.true152:                                     ; preds = %sout.bool142
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge154

sout.false153:                                    ; preds = %sout.bool142
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge154

sout.bool.merge154:                               ; preds = %sout.false153, %sout.true152
  br label %sout.merge146

sout.int177:                                      ; preds = %sout.merge146
  %int.val184 = ptrtoint ptr %payload176 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.50, i64 %int.val184)
  br label %sout.merge183

sout.float178:                                    ; preds = %sout.merge146
  %float.bits185 = ptrtoint ptr %payload176 to i64
  %float.val186 = bitcast i64 %float.bits185 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.51, double %float.val186)
  br label %sout.merge183

sout.bool179:                                     ; preds = %sout.merge146
  %bool.val187 = ptrtoint ptr %payload176 to i64
  %is.true188 = icmp ne i64 %bool.val187, 0
  br i1 %is.true188, label %sout.true189, label %sout.false190

sout.str180:                                      ; preds = %sout.merge146
  %44 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr %payload176)
  br label %sout.merge183

sout.null181:                                     ; preds = %sout.merge146
  %45 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr @.str.58)
  br label %sout.merge183

sout.default182:                                  ; preds = %sout.merge146
  %46 = call i32 (ptr, ...) @printf(ptr @.str.59, ptr %payload176)
  br label %sout.merge183

sout.merge183:                                    ; preds = %sout.default182, %sout.null181, %sout.str180, %sout.bool.merge191, %sout.float178, %sout.int177
  %47 = call i32 @putchar(i32 10)
  %rcc.val192 = alloca %RCCValue, align 8
  %tag.ptr193 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val192, i32 0, i32 0
  store i64 0, ptr %tag.ptr193, align 4
  %payload.ptr194 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val192, i32 0, i32 1
  store ptr null, ptr %payload.ptr194, align 8
  %rcc.val195 = alloca %RCCValue, align 8
  %tag.ptr196 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val195, i32 0, i32 0
  store i64 3, ptr %tag.ptr196, align 4
  %payload.ptr197 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val195, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr197, align 8
  %rcc.val198 = alloca %RCCValue, align 8
  %tag.ptr199 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val198, i32 0, i32 0
  store i64 3, ptr %tag.ptr199, align 4
  %payload.ptr200 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val198, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr200, align 8
  %payload.ptr201 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val195, i32 0, i32 1
  %payload202 = load ptr, ptr %payload.ptr201, align 8
  %payload.ptr203 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val198, i32 0, i32 1
  %payload204 = load ptr, ptr %payload.ptr203, align 8
  %left.int205 = ptrtoint ptr %payload202 to i64
  %right.int206 = ptrtoint ptr %payload204 to i64
  %tag.ptr207 = getelementptr inbounds nuw %RCCValue, ptr %payload204, i32 0, i32 0
  %tag208 = load i64, ptr %tag.ptr207, align 4
  %payload.ptr209 = getelementptr inbounds nuw %RCCValue, ptr %payload204, i32 0, i32 1
  %payload210 = load ptr, ptr %payload.ptr209, align 8
  %is_null = icmp eq i64 %tag208, 0
  %is_int = icmp eq i64 %tag208, 1
  %is_bool = icmp eq i64 %tag208, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload210 to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  %tag.ptr211 = getelementptr inbounds nuw %RCCValue, ptr %payload202, i32 0, i32 0
  %tag212 = load i64, ptr %tag.ptr211, align 4
  %payload.ptr213 = getelementptr inbounds nuw %RCCValue, ptr %payload202, i32 0, i32 1
  %payload214 = load ptr, ptr %payload.ptr213, align 8
  %is_null215 = icmp eq i64 %tag212, 0
  %is_int216 = icmp eq i64 %tag212, 1
  %is_bool217 = icmp eq i64 %tag212, 3
  %is_bool_or_int218 = or i1 %is_bool217, %is_int216
  %payload.int219 = ptrtoint ptr %payload214 to i64
  %payload_nz220 = icmp ne i64 %payload.int219, 0
  %not_null221 = xor i1 %is_null215, true
  %bool_int_result222 = select i1 %is_bool_or_int218, i1 %payload_nz220, i1 true
  %coerced_bool223 = select i1 %is_null215, i1 false, i1 %bool_int_result222
  %land = and i1 %coerced_bool223, %coerced_bool
  %logic.ext = zext i1 %land to i64
  %int.ptr224 = inttoptr i64 %logic.ext to ptr
  %rcc.val225 = alloca %RCCValue, align 8
  %tag.ptr226 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val225, i32 0, i32 0
  store i64 1, ptr %tag.ptr226, align 4
  %payload.ptr227 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val225, i32 0, i32 1
  store ptr %int.ptr224, ptr %payload.ptr227, align 8
  %tag.ptr228 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val225, i32 0, i32 0
  %tag229 = load i64, ptr %tag.ptr228, align 4
  %payload.ptr230 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val225, i32 0, i32 1
  %payload231 = load ptr, ptr %payload.ptr230, align 8
  switch i64 %tag229, label %sout.default237 [
    i64 1, label %sout.int232
    i64 2, label %sout.float233
    i64 3, label %sout.bool234
    i64 4, label %sout.str235
    i64 0, label %sout.null236
  ]

sout.true189:                                     ; preds = %sout.bool179
  %48 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.bool.merge191

sout.false190:                                    ; preds = %sout.bool179
  %49 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr @.str.55)
  br label %sout.bool.merge191

sout.bool.merge191:                               ; preds = %sout.false190, %sout.true189
  br label %sout.merge183

sout.int232:                                      ; preds = %sout.merge183
  %int.val239 = ptrtoint ptr %payload231 to i64
  %50 = call i32 (ptr, ...) @printf(ptr @.str.60, i64 %int.val239)
  br label %sout.merge238

sout.float233:                                    ; preds = %sout.merge183
  %float.bits240 = ptrtoint ptr %payload231 to i64
  %float.val241 = bitcast i64 %float.bits240 to double
  %51 = call i32 (ptr, ...) @printf(ptr @.str.61, double %float.val241)
  br label %sout.merge238

sout.bool234:                                     ; preds = %sout.merge183
  %bool.val242 = ptrtoint ptr %payload231 to i64
  %is.true243 = icmp ne i64 %bool.val242, 0
  br i1 %is.true243, label %sout.true244, label %sout.false245

sout.str235:                                      ; preds = %sout.merge183
  %52 = call i32 (ptr, ...) @printf(ptr @.str.66, ptr %payload231)
  br label %sout.merge238

sout.null236:                                     ; preds = %sout.merge183
  %53 = call i32 (ptr, ...) @printf(ptr @.str.67, ptr @.str.68)
  br label %sout.merge238

sout.default237:                                  ; preds = %sout.merge183
  %54 = call i32 (ptr, ...) @printf(ptr @.str.69, ptr %payload231)
  br label %sout.merge238

sout.merge238:                                    ; preds = %sout.default237, %sout.null236, %sout.str235, %sout.bool.merge246, %sout.float233, %sout.int232
  %55 = call i32 @putchar(i32 10)
  %rcc.val247 = alloca %RCCValue, align 8
  %tag.ptr248 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val247, i32 0, i32 0
  store i64 0, ptr %tag.ptr248, align 4
  %payload.ptr249 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val247, i32 0, i32 1
  store ptr null, ptr %payload.ptr249, align 8
  %rcc.val250 = alloca %RCCValue, align 8
  %tag.ptr251 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val250, i32 0, i32 0
  store i64 3, ptr %tag.ptr251, align 4
  %payload.ptr252 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val250, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr252, align 8
  %rcc.val253 = alloca %RCCValue, align 8
  %tag.ptr254 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val253, i32 0, i32 0
  store i64 3, ptr %tag.ptr254, align 4
  %payload.ptr255 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val253, i32 0, i32 1
  store ptr null, ptr %payload.ptr255, align 8
  %payload.ptr256 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val250, i32 0, i32 1
  %payload257 = load ptr, ptr %payload.ptr256, align 8
  %payload.ptr258 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val253, i32 0, i32 1
  %payload259 = load ptr, ptr %payload.ptr258, align 8
  %left.int260 = ptrtoint ptr %payload257 to i64
  %right.int261 = ptrtoint ptr %payload259 to i64
  %tag.ptr262 = getelementptr inbounds nuw %RCCValue, ptr %payload259, i32 0, i32 0
  %tag263 = load i64, ptr %tag.ptr262, align 4
  %payload.ptr264 = getelementptr inbounds nuw %RCCValue, ptr %payload259, i32 0, i32 1
  %payload265 = load ptr, ptr %payload.ptr264, align 8
  %is_null266 = icmp eq i64 %tag263, 0
  %is_int267 = icmp eq i64 %tag263, 1
  %is_bool268 = icmp eq i64 %tag263, 3
  %is_bool_or_int269 = or i1 %is_bool268, %is_int267
  %payload.int270 = ptrtoint ptr %payload265 to i64
  %payload_nz271 = icmp ne i64 %payload.int270, 0
  %not_null272 = xor i1 %is_null266, true
  %bool_int_result273 = select i1 %is_bool_or_int269, i1 %payload_nz271, i1 true
  %coerced_bool274 = select i1 %is_null266, i1 false, i1 %bool_int_result273
  %tag.ptr275 = getelementptr inbounds nuw %RCCValue, ptr %payload257, i32 0, i32 0
  %tag276 = load i64, ptr %tag.ptr275, align 4
  %payload.ptr277 = getelementptr inbounds nuw %RCCValue, ptr %payload257, i32 0, i32 1
  %payload278 = load ptr, ptr %payload.ptr277, align 8
  %is_null279 = icmp eq i64 %tag276, 0
  %is_int280 = icmp eq i64 %tag276, 1
  %is_bool281 = icmp eq i64 %tag276, 3
  %is_bool_or_int282 = or i1 %is_bool281, %is_int280
  %payload.int283 = ptrtoint ptr %payload278 to i64
  %payload_nz284 = icmp ne i64 %payload.int283, 0
  %not_null285 = xor i1 %is_null279, true
  %bool_int_result286 = select i1 %is_bool_or_int282, i1 %payload_nz284, i1 true
  %coerced_bool287 = select i1 %is_null279, i1 false, i1 %bool_int_result286
  %land288 = and i1 %coerced_bool287, %coerced_bool274
  %logic.ext289 = zext i1 %land288 to i64
  %int.ptr290 = inttoptr i64 %logic.ext289 to ptr
  %rcc.val291 = alloca %RCCValue, align 8
  %tag.ptr292 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val291, i32 0, i32 0
  store i64 1, ptr %tag.ptr292, align 4
  %payload.ptr293 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val291, i32 0, i32 1
  store ptr %int.ptr290, ptr %payload.ptr293, align 8
  %tag.ptr294 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val291, i32 0, i32 0
  %tag295 = load i64, ptr %tag.ptr294, align 4
  %payload.ptr296 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val291, i32 0, i32 1
  %payload297 = load ptr, ptr %payload.ptr296, align 8
  switch i64 %tag295, label %sout.default303 [
    i64 1, label %sout.int298
    i64 2, label %sout.float299
    i64 3, label %sout.bool300
    i64 4, label %sout.str301
    i64 0, label %sout.null302
  ]

sout.true244:                                     ; preds = %sout.bool234
  %56 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr @.str.63)
  br label %sout.bool.merge246

sout.false245:                                    ; preds = %sout.bool234
  %57 = call i32 (ptr, ...) @printf(ptr @.str.64, ptr @.str.65)
  br label %sout.bool.merge246

sout.bool.merge246:                               ; preds = %sout.false245, %sout.true244
  br label %sout.merge238

sout.int298:                                      ; preds = %sout.merge238
  %int.val305 = ptrtoint ptr %payload297 to i64
  %58 = call i32 (ptr, ...) @printf(ptr @.str.70, i64 %int.val305)
  br label %sout.merge304

sout.float299:                                    ; preds = %sout.merge238
  %float.bits306 = ptrtoint ptr %payload297 to i64
  %float.val307 = bitcast i64 %float.bits306 to double
  %59 = call i32 (ptr, ...) @printf(ptr @.str.71, double %float.val307)
  br label %sout.merge304

sout.bool300:                                     ; preds = %sout.merge238
  %bool.val308 = ptrtoint ptr %payload297 to i64
  %is.true309 = icmp ne i64 %bool.val308, 0
  br i1 %is.true309, label %sout.true310, label %sout.false311

sout.str301:                                      ; preds = %sout.merge238
  %60 = call i32 (ptr, ...) @printf(ptr @.str.76, ptr %payload297)
  br label %sout.merge304

sout.null302:                                     ; preds = %sout.merge238
  %61 = call i32 (ptr, ...) @printf(ptr @.str.77, ptr @.str.78)
  br label %sout.merge304

sout.default303:                                  ; preds = %sout.merge238
  %62 = call i32 (ptr, ...) @printf(ptr @.str.79, ptr %payload297)
  br label %sout.merge304

sout.merge304:                                    ; preds = %sout.default303, %sout.null302, %sout.str301, %sout.bool.merge312, %sout.float299, %sout.int298
  %63 = call i32 @putchar(i32 10)
  %rcc.val313 = alloca %RCCValue, align 8
  %tag.ptr314 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val313, i32 0, i32 0
  store i64 0, ptr %tag.ptr314, align 4
  %payload.ptr315 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val313, i32 0, i32 1
  store ptr null, ptr %payload.ptr315, align 8
  %rcc.val316 = alloca %RCCValue, align 8
  %tag.ptr317 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val316, i32 0, i32 0
  store i64 3, ptr %tag.ptr317, align 4
  %payload.ptr318 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val316, i32 0, i32 1
  store ptr null, ptr %payload.ptr318, align 8
  %rcc.val319 = alloca %RCCValue, align 8
  %tag.ptr320 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val319, i32 0, i32 0
  store i64 3, ptr %tag.ptr320, align 4
  %payload.ptr321 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val319, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr321, align 8
  %payload.ptr322 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val316, i32 0, i32 1
  %payload323 = load ptr, ptr %payload.ptr322, align 8
  %payload.ptr324 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val319, i32 0, i32 1
  %payload325 = load ptr, ptr %payload.ptr324, align 8
  %left.int326 = ptrtoint ptr %payload323 to i64
  %right.int327 = ptrtoint ptr %payload325 to i64
  %tag.ptr328 = getelementptr inbounds nuw %RCCValue, ptr %payload325, i32 0, i32 0
  %tag329 = load i64, ptr %tag.ptr328, align 4
  %payload.ptr330 = getelementptr inbounds nuw %RCCValue, ptr %payload325, i32 0, i32 1
  %payload331 = load ptr, ptr %payload.ptr330, align 8
  %is_null332 = icmp eq i64 %tag329, 0
  %is_int333 = icmp eq i64 %tag329, 1
  %is_bool334 = icmp eq i64 %tag329, 3
  %is_bool_or_int335 = or i1 %is_bool334, %is_int333
  %payload.int336 = ptrtoint ptr %payload331 to i64
  %payload_nz337 = icmp ne i64 %payload.int336, 0
  %not_null338 = xor i1 %is_null332, true
  %bool_int_result339 = select i1 %is_bool_or_int335, i1 %payload_nz337, i1 true
  %coerced_bool340 = select i1 %is_null332, i1 false, i1 %bool_int_result339
  %tag.ptr341 = getelementptr inbounds nuw %RCCValue, ptr %payload323, i32 0, i32 0
  %tag342 = load i64, ptr %tag.ptr341, align 4
  %payload.ptr343 = getelementptr inbounds nuw %RCCValue, ptr %payload323, i32 0, i32 1
  %payload344 = load ptr, ptr %payload.ptr343, align 8
  %is_null345 = icmp eq i64 %tag342, 0
  %is_int346 = icmp eq i64 %tag342, 1
  %is_bool347 = icmp eq i64 %tag342, 3
  %is_bool_or_int348 = or i1 %is_bool347, %is_int346
  %payload.int349 = ptrtoint ptr %payload344 to i64
  %payload_nz350 = icmp ne i64 %payload.int349, 0
  %not_null351 = xor i1 %is_null345, true
  %bool_int_result352 = select i1 %is_bool_or_int348, i1 %payload_nz350, i1 true
  %coerced_bool353 = select i1 %is_null345, i1 false, i1 %bool_int_result352
  %lor = or i1 %coerced_bool353, %coerced_bool340
  %logic.ext354 = zext i1 %lor to i64
  %int.ptr355 = inttoptr i64 %logic.ext354 to ptr
  %rcc.val356 = alloca %RCCValue, align 8
  %tag.ptr357 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val356, i32 0, i32 0
  store i64 1, ptr %tag.ptr357, align 4
  %payload.ptr358 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val356, i32 0, i32 1
  store ptr %int.ptr355, ptr %payload.ptr358, align 8
  %tag.ptr359 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val356, i32 0, i32 0
  %tag360 = load i64, ptr %tag.ptr359, align 4
  %payload.ptr361 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val356, i32 0, i32 1
  %payload362 = load ptr, ptr %payload.ptr361, align 8
  switch i64 %tag360, label %sout.default368 [
    i64 1, label %sout.int363
    i64 2, label %sout.float364
    i64 3, label %sout.bool365
    i64 4, label %sout.str366
    i64 0, label %sout.null367
  ]

sout.true310:                                     ; preds = %sout.bool300
  %64 = call i32 (ptr, ...) @printf(ptr @.str.72, ptr @.str.73)
  br label %sout.bool.merge312

sout.false311:                                    ; preds = %sout.bool300
  %65 = call i32 (ptr, ...) @printf(ptr @.str.74, ptr @.str.75)
  br label %sout.bool.merge312

sout.bool.merge312:                               ; preds = %sout.false311, %sout.true310
  br label %sout.merge304

sout.int363:                                      ; preds = %sout.merge304
  %int.val370 = ptrtoint ptr %payload362 to i64
  %66 = call i32 (ptr, ...) @printf(ptr @.str.80, i64 %int.val370)
  br label %sout.merge369

sout.float364:                                    ; preds = %sout.merge304
  %float.bits371 = ptrtoint ptr %payload362 to i64
  %float.val372 = bitcast i64 %float.bits371 to double
  %67 = call i32 (ptr, ...) @printf(ptr @.str.81, double %float.val372)
  br label %sout.merge369

sout.bool365:                                     ; preds = %sout.merge304
  %bool.val373 = ptrtoint ptr %payload362 to i64
  %is.true374 = icmp ne i64 %bool.val373, 0
  br i1 %is.true374, label %sout.true375, label %sout.false376

sout.str366:                                      ; preds = %sout.merge304
  %68 = call i32 (ptr, ...) @printf(ptr @.str.86, ptr %payload362)
  br label %sout.merge369

sout.null367:                                     ; preds = %sout.merge304
  %69 = call i32 (ptr, ...) @printf(ptr @.str.87, ptr @.str.88)
  br label %sout.merge369

sout.default368:                                  ; preds = %sout.merge304
  %70 = call i32 (ptr, ...) @printf(ptr @.str.89, ptr %payload362)
  br label %sout.merge369

sout.merge369:                                    ; preds = %sout.default368, %sout.null367, %sout.str366, %sout.bool.merge377, %sout.float364, %sout.int363
  %71 = call i32 @putchar(i32 10)
  %rcc.val378 = alloca %RCCValue, align 8
  %tag.ptr379 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val378, i32 0, i32 0
  store i64 0, ptr %tag.ptr379, align 4
  %payload.ptr380 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val378, i32 0, i32 1
  store ptr null, ptr %payload.ptr380, align 8
  %rcc.val381 = alloca %RCCValue, align 8
  %tag.ptr382 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val381, i32 0, i32 0
  store i64 3, ptr %tag.ptr382, align 4
  %payload.ptr383 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val381, i32 0, i32 1
  store ptr null, ptr %payload.ptr383, align 8
  %rcc.val384 = alloca %RCCValue, align 8
  %tag.ptr385 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val384, i32 0, i32 0
  store i64 3, ptr %tag.ptr385, align 4
  %payload.ptr386 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val384, i32 0, i32 1
  store ptr null, ptr %payload.ptr386, align 8
  %payload.ptr387 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val381, i32 0, i32 1
  %payload388 = load ptr, ptr %payload.ptr387, align 8
  %payload.ptr389 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val384, i32 0, i32 1
  %payload390 = load ptr, ptr %payload.ptr389, align 8
  %left.int391 = ptrtoint ptr %payload388 to i64
  %right.int392 = ptrtoint ptr %payload390 to i64
  %tag.ptr393 = getelementptr inbounds nuw %RCCValue, ptr %payload390, i32 0, i32 0
  %tag394 = load i64, ptr %tag.ptr393, align 4
  %payload.ptr395 = getelementptr inbounds nuw %RCCValue, ptr %payload390, i32 0, i32 1
  %payload396 = load ptr, ptr %payload.ptr395, align 8
  %is_null397 = icmp eq i64 %tag394, 0
  %is_int398 = icmp eq i64 %tag394, 1
  %is_bool399 = icmp eq i64 %tag394, 3
  %is_bool_or_int400 = or i1 %is_bool399, %is_int398
  %payload.int401 = ptrtoint ptr %payload396 to i64
  %payload_nz402 = icmp ne i64 %payload.int401, 0
  %not_null403 = xor i1 %is_null397, true
  %bool_int_result404 = select i1 %is_bool_or_int400, i1 %payload_nz402, i1 true
  %coerced_bool405 = select i1 %is_null397, i1 false, i1 %bool_int_result404
  %tag.ptr406 = getelementptr inbounds nuw %RCCValue, ptr %payload388, i32 0, i32 0
  %tag407 = load i64, ptr %tag.ptr406, align 4
  %payload.ptr408 = getelementptr inbounds nuw %RCCValue, ptr %payload388, i32 0, i32 1
  %payload409 = load ptr, ptr %payload.ptr408, align 8
  %is_null410 = icmp eq i64 %tag407, 0
  %is_int411 = icmp eq i64 %tag407, 1
  %is_bool412 = icmp eq i64 %tag407, 3
  %is_bool_or_int413 = or i1 %is_bool412, %is_int411
  %payload.int414 = ptrtoint ptr %payload409 to i64
  %payload_nz415 = icmp ne i64 %payload.int414, 0
  %not_null416 = xor i1 %is_null410, true
  %bool_int_result417 = select i1 %is_bool_or_int413, i1 %payload_nz415, i1 true
  %coerced_bool418 = select i1 %is_null410, i1 false, i1 %bool_int_result417
  %lor419 = or i1 %coerced_bool418, %coerced_bool405
  %logic.ext420 = zext i1 %lor419 to i64
  %int.ptr421 = inttoptr i64 %logic.ext420 to ptr
  %rcc.val422 = alloca %RCCValue, align 8
  %tag.ptr423 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val422, i32 0, i32 0
  store i64 1, ptr %tag.ptr423, align 4
  %payload.ptr424 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val422, i32 0, i32 1
  store ptr %int.ptr421, ptr %payload.ptr424, align 8
  %tag.ptr425 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val422, i32 0, i32 0
  %tag426 = load i64, ptr %tag.ptr425, align 4
  %payload.ptr427 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val422, i32 0, i32 1
  %payload428 = load ptr, ptr %payload.ptr427, align 8
  switch i64 %tag426, label %sout.default434 [
    i64 1, label %sout.int429
    i64 2, label %sout.float430
    i64 3, label %sout.bool431
    i64 4, label %sout.str432
    i64 0, label %sout.null433
  ]

sout.true375:                                     ; preds = %sout.bool365
  %72 = call i32 (ptr, ...) @printf(ptr @.str.82, ptr @.str.83)
  br label %sout.bool.merge377

sout.false376:                                    ; preds = %sout.bool365
  %73 = call i32 (ptr, ...) @printf(ptr @.str.84, ptr @.str.85)
  br label %sout.bool.merge377

sout.bool.merge377:                               ; preds = %sout.false376, %sout.true375
  br label %sout.merge369

sout.int429:                                      ; preds = %sout.merge369
  %int.val436 = ptrtoint ptr %payload428 to i64
  %74 = call i32 (ptr, ...) @printf(ptr @.str.90, i64 %int.val436)
  br label %sout.merge435

sout.float430:                                    ; preds = %sout.merge369
  %float.bits437 = ptrtoint ptr %payload428 to i64
  %float.val438 = bitcast i64 %float.bits437 to double
  %75 = call i32 (ptr, ...) @printf(ptr @.str.91, double %float.val438)
  br label %sout.merge435

sout.bool431:                                     ; preds = %sout.merge369
  %bool.val439 = ptrtoint ptr %payload428 to i64
  %is.true440 = icmp ne i64 %bool.val439, 0
  br i1 %is.true440, label %sout.true441, label %sout.false442

sout.str432:                                      ; preds = %sout.merge369
  %76 = call i32 (ptr, ...) @printf(ptr @.str.96, ptr %payload428)
  br label %sout.merge435

sout.null433:                                     ; preds = %sout.merge369
  %77 = call i32 (ptr, ...) @printf(ptr @.str.97, ptr @.str.98)
  br label %sout.merge435

sout.default434:                                  ; preds = %sout.merge369
  %78 = call i32 (ptr, ...) @printf(ptr @.str.99, ptr %payload428)
  br label %sout.merge435

sout.merge435:                                    ; preds = %sout.default434, %sout.null433, %sout.str432, %sout.bool.merge443, %sout.float430, %sout.int429
  %79 = call i32 @putchar(i32 10)
  %rcc.val444 = alloca %RCCValue, align 8
  %tag.ptr445 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val444, i32 0, i32 0
  store i64 0, ptr %tag.ptr445, align 4
  %payload.ptr446 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val444, i32 0, i32 1
  store ptr null, ptr %payload.ptr446, align 8
  %rcc.val447 = alloca %RCCValue, align 8
  %tag.ptr448 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val447, i32 0, i32 0
  store i64 3, ptr %tag.ptr448, align 4
  %payload.ptr449 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val447, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr449, align 8
  %unary.int = ptrtoint ptr %rcc.val447 to i64
  %tag.ptr450 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val447, i32 0, i32 0
  %tag451 = load i64, ptr %tag.ptr450, align 4
  %payload.ptr452 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val447, i32 0, i32 1
  %payload453 = load ptr, ptr %payload.ptr452, align 8
  %is_null454 = icmp eq i64 %tag451, 0
  %is_int455 = icmp eq i64 %tag451, 1
  %is_bool456 = icmp eq i64 %tag451, 3
  %is_bool_or_int457 = or i1 %is_bool456, %is_int455
  %payload.int458 = ptrtoint ptr %payload453 to i64
  %payload_nz459 = icmp ne i64 %payload.int458, 0
  %not_null460 = xor i1 %is_null454, true
  %bool_int_result461 = select i1 %is_bool_or_int457, i1 %payload_nz459, i1 true
  %coerced_bool462 = select i1 %is_null454, i1 false, i1 %bool_int_result461
  %not = xor i1 %coerced_bool462, true
  %not.ext = zext i1 %not to i64
  %not.is_true = icmp ne i64 %not.ext, 0
  %not.payload = select i1 %not.is_true, ptr inttoptr (i64 1 to ptr), ptr null
  %rcc.val463 = alloca %RCCValue, align 8
  %tag.ptr464 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val463, i32 0, i32 0
  store i64 3, ptr %tag.ptr464, align 4
  %payload.ptr465 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val463, i32 0, i32 1
  store ptr %not.payload, ptr %payload.ptr465, align 8
  %tag.ptr466 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val463, i32 0, i32 0
  %tag467 = load i64, ptr %tag.ptr466, align 4
  %payload.ptr468 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val463, i32 0, i32 1
  %payload469 = load ptr, ptr %payload.ptr468, align 8
  switch i64 %tag467, label %sout.default475 [
    i64 1, label %sout.int470
    i64 2, label %sout.float471
    i64 3, label %sout.bool472
    i64 4, label %sout.str473
    i64 0, label %sout.null474
  ]

sout.true441:                                     ; preds = %sout.bool431
  %80 = call i32 (ptr, ...) @printf(ptr @.str.92, ptr @.str.93)
  br label %sout.bool.merge443

sout.false442:                                    ; preds = %sout.bool431
  %81 = call i32 (ptr, ...) @printf(ptr @.str.94, ptr @.str.95)
  br label %sout.bool.merge443

sout.bool.merge443:                               ; preds = %sout.false442, %sout.true441
  br label %sout.merge435

sout.int470:                                      ; preds = %sout.merge435
  %int.val477 = ptrtoint ptr %payload469 to i64
  %82 = call i32 (ptr, ...) @printf(ptr @.str.100, i64 %int.val477)
  br label %sout.merge476

sout.float471:                                    ; preds = %sout.merge435
  %float.bits478 = ptrtoint ptr %payload469 to i64
  %float.val479 = bitcast i64 %float.bits478 to double
  %83 = call i32 (ptr, ...) @printf(ptr @.str.101, double %float.val479)
  br label %sout.merge476

sout.bool472:                                     ; preds = %sout.merge435
  %bool.val480 = ptrtoint ptr %payload469 to i64
  %is.true481 = icmp ne i64 %bool.val480, 0
  br i1 %is.true481, label %sout.true482, label %sout.false483

sout.str473:                                      ; preds = %sout.merge435
  %84 = call i32 (ptr, ...) @printf(ptr @.str.106, ptr %payload469)
  br label %sout.merge476

sout.null474:                                     ; preds = %sout.merge435
  %85 = call i32 (ptr, ...) @printf(ptr @.str.107, ptr @.str.108)
  br label %sout.merge476

sout.default475:                                  ; preds = %sout.merge435
  %86 = call i32 (ptr, ...) @printf(ptr @.str.109, ptr %payload469)
  br label %sout.merge476

sout.merge476:                                    ; preds = %sout.default475, %sout.null474, %sout.str473, %sout.bool.merge484, %sout.float471, %sout.int470
  %87 = call i32 @putchar(i32 10)
  %rcc.val485 = alloca %RCCValue, align 8
  %tag.ptr486 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val485, i32 0, i32 0
  store i64 0, ptr %tag.ptr486, align 4
  %payload.ptr487 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val485, i32 0, i32 1
  store ptr null, ptr %payload.ptr487, align 8
  %rcc.val488 = alloca %RCCValue, align 8
  %tag.ptr489 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val488, i32 0, i32 0
  store i64 3, ptr %tag.ptr489, align 4
  %payload.ptr490 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val488, i32 0, i32 1
  store ptr null, ptr %payload.ptr490, align 8
  %unary.int491 = ptrtoint ptr %rcc.val488 to i64
  %tag.ptr492 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val488, i32 0, i32 0
  %tag493 = load i64, ptr %tag.ptr492, align 4
  %payload.ptr494 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val488, i32 0, i32 1
  %payload495 = load ptr, ptr %payload.ptr494, align 8
  %is_null496 = icmp eq i64 %tag493, 0
  %is_int497 = icmp eq i64 %tag493, 1
  %is_bool498 = icmp eq i64 %tag493, 3
  %is_bool_or_int499 = or i1 %is_bool498, %is_int497
  %payload.int500 = ptrtoint ptr %payload495 to i64
  %payload_nz501 = icmp ne i64 %payload.int500, 0
  %not_null502 = xor i1 %is_null496, true
  %bool_int_result503 = select i1 %is_bool_or_int499, i1 %payload_nz501, i1 true
  %coerced_bool504 = select i1 %is_null496, i1 false, i1 %bool_int_result503
  %not505 = xor i1 %coerced_bool504, true
  %not.ext506 = zext i1 %not505 to i64
  %not.is_true507 = icmp ne i64 %not.ext506, 0
  %not.payload508 = select i1 %not.is_true507, ptr inttoptr (i64 1 to ptr), ptr null
  %rcc.val509 = alloca %RCCValue, align 8
  %tag.ptr510 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val509, i32 0, i32 0
  store i64 3, ptr %tag.ptr510, align 4
  %payload.ptr511 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val509, i32 0, i32 1
  store ptr %not.payload508, ptr %payload.ptr511, align 8
  %tag.ptr512 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val509, i32 0, i32 0
  %tag513 = load i64, ptr %tag.ptr512, align 4
  %payload.ptr514 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val509, i32 0, i32 1
  %payload515 = load ptr, ptr %payload.ptr514, align 8
  switch i64 %tag513, label %sout.default521 [
    i64 1, label %sout.int516
    i64 2, label %sout.float517
    i64 3, label %sout.bool518
    i64 4, label %sout.str519
    i64 0, label %sout.null520
  ]

sout.true482:                                     ; preds = %sout.bool472
  %88 = call i32 (ptr, ...) @printf(ptr @.str.102, ptr @.str.103)
  br label %sout.bool.merge484

sout.false483:                                    ; preds = %sout.bool472
  %89 = call i32 (ptr, ...) @printf(ptr @.str.104, ptr @.str.105)
  br label %sout.bool.merge484

sout.bool.merge484:                               ; preds = %sout.false483, %sout.true482
  br label %sout.merge476

sout.int516:                                      ; preds = %sout.merge476
  %int.val523 = ptrtoint ptr %payload515 to i64
  %90 = call i32 (ptr, ...) @printf(ptr @.str.110, i64 %int.val523)
  br label %sout.merge522

sout.float517:                                    ; preds = %sout.merge476
  %float.bits524 = ptrtoint ptr %payload515 to i64
  %float.val525 = bitcast i64 %float.bits524 to double
  %91 = call i32 (ptr, ...) @printf(ptr @.str.111, double %float.val525)
  br label %sout.merge522

sout.bool518:                                     ; preds = %sout.merge476
  %bool.val526 = ptrtoint ptr %payload515 to i64
  %is.true527 = icmp ne i64 %bool.val526, 0
  br i1 %is.true527, label %sout.true528, label %sout.false529

sout.str519:                                      ; preds = %sout.merge476
  %92 = call i32 (ptr, ...) @printf(ptr @.str.116, ptr %payload515)
  br label %sout.merge522

sout.null520:                                     ; preds = %sout.merge476
  %93 = call i32 (ptr, ...) @printf(ptr @.str.117, ptr @.str.118)
  br label %sout.merge522

sout.default521:                                  ; preds = %sout.merge476
  %94 = call i32 (ptr, ...) @printf(ptr @.str.119, ptr %payload515)
  br label %sout.merge522

sout.merge522:                                    ; preds = %sout.default521, %sout.null520, %sout.str519, %sout.bool.merge530, %sout.float517, %sout.int516
  %95 = call i32 @putchar(i32 10)
  %rcc.val531 = alloca %RCCValue, align 8
  %tag.ptr532 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val531, i32 0, i32 0
  store i64 0, ptr %tag.ptr532, align 4
  %payload.ptr533 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val531, i32 0, i32 1
  store ptr null, ptr %payload.ptr533, align 8
  ret i32 0

sout.true528:                                     ; preds = %sout.bool518
  %96 = call i32 (ptr, ...) @printf(ptr @.str.112, ptr @.str.113)
  br label %sout.bool.merge530

sout.false529:                                    ; preds = %sout.bool518
  %97 = call i32 (ptr, ...) @printf(ptr @.str.114, ptr @.str.115)
  br label %sout.bool.merge530

sout.bool.merge530:                               ; preds = %sout.false529, %sout.true528
  br label %sout.merge522
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
