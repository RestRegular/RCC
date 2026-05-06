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
  %arith.result299 = alloca ptr, align 8
  %arith.result233 = alloca ptr, align 8
  %arith.result167 = alloca ptr, align 8
  %arith.result103 = alloca ptr, align 8
  %arith.result39 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
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
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr4, align 4
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 0
  %tag6 = load i64, ptr %tag.ptr5, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag6, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr7, align 8
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload9 = load ptr, ptr %payload.ptr8, align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload9 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val10 = alloca %RCCValue, align 8
  %tag.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val10, i32 0, i32 0
  store i64 2, ptr %tag.ptr11, align 4
  %payload.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val10, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr12, align 8
  store ptr %rcc.val10, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %a.load, i32 0, i32 1
  %payload14 = load ptr, ptr %payload.ptr13, align 8
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %b.load, i32 0, i32 1
  %payload16 = load ptr, ptr %payload.ptr15, align 8
  %left.int = ptrtoint ptr %payload14 to i64
  %right.int = ptrtoint ptr %payload16 to i64
  %cmp.eq = icmp eq i64 %left.int, %right.int
  %cmp.ext = zext i1 %cmp.eq to i64
  %int.ptr = inttoptr i64 %cmp.ext to ptr
  %rcc.val17 = alloca %RCCValue, align 8
  %tag.ptr18 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val17, i32 0, i32 0
  store i64 3, ptr %tag.ptr18, align 4
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val17, i32 0, i32 1
  store ptr %int.ptr, ptr %payload.ptr19, align 8
  store ptr %rcc.val17, ptr %arith.result, align 8
  br label %arith.merge

arith.merge:                                      ; preds = %arith.int, %arith.float
  %arith.result.load = load ptr, ptr %arith.result, align 8
  %tag.ptr20 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 0
  %tag21 = load i64, ptr %tag.ptr20, align 4
  %payload.ptr22 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load, i32 0, i32 1
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
  %a.load27 = load ptr, ptr %a, align 8
  %b.load28 = load ptr, ptr %b, align 8
  %tag.ptr29 = getelementptr inbounds nuw %RCCValue, ptr %a.load27, i32 0, i32 0
  %tag30 = load i64, ptr %tag.ptr29, align 4
  %tag.ptr31 = getelementptr inbounds nuw %RCCValue, ptr %b.load28, i32 0, i32 0
  %tag32 = load i64, ptr %tag.ptr31, align 4
  %is_float.left33 = icmp eq i64 %tag30, 2
  %is_float.right34 = icmp eq i64 %tag32, 2
  %is_float35 = or i1 %is_float.left33, %is_float.right34
  store ptr null, ptr %arith.result39, align 8
  br i1 %is_float35, label %arith.float36, label %arith.int37

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

arith.float36:                                    ; preds = %sout.merge
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %a.load27, i32 0, i32 1
  %payload41 = load ptr, ptr %payload.ptr40, align 8
  %payload.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %b.load28, i32 0, i32 1
  %payload43 = load ptr, ptr %payload.ptr42, align 8
  %left.bits44 = ptrtoint ptr %payload41 to i64
  %right.bits45 = ptrtoint ptr %payload43 to i64
  %left.sitofp46 = sitofp i64 %left.bits44 to double
  %left.bitcast47 = bitcast i64 %left.bits44 to double
  %left.double48 = select i1 %is_float.left33, double %left.bitcast47, double %left.sitofp46
  %right.sitofp49 = sitofp i64 %right.bits45 to double
  %right.bitcast50 = bitcast i64 %right.bits45 to double
  %right.double51 = select i1 %is_float.right34, double %right.bitcast50, double %right.sitofp49
  %result.bits52 = bitcast double %left.double48 to i64
  %result.float.ptr53 = inttoptr i64 %result.bits52 to ptr
  %rcc.val54 = alloca %RCCValue, align 8
  %tag.ptr55 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 0
  store i64 2, ptr %tag.ptr55, align 4
  %payload.ptr56 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val54, i32 0, i32 1
  store ptr %result.float.ptr53, ptr %payload.ptr56, align 8
  store ptr %rcc.val54, ptr %arith.result39, align 8
  br label %arith.merge38

arith.int37:                                      ; preds = %sout.merge
  %payload.ptr57 = getelementptr inbounds nuw %RCCValue, ptr %a.load27, i32 0, i32 1
  %payload58 = load ptr, ptr %payload.ptr57, align 8
  %payload.ptr59 = getelementptr inbounds nuw %RCCValue, ptr %b.load28, i32 0, i32 1
  %payload60 = load ptr, ptr %payload.ptr59, align 8
  %left.int61 = ptrtoint ptr %payload58 to i64
  %right.int62 = ptrtoint ptr %payload60 to i64
  %cmp.ne = icmp ne i64 %left.int61, %right.int62
  %cmp.ext63 = zext i1 %cmp.ne to i64
  %int.ptr64 = inttoptr i64 %cmp.ext63 to ptr
  %rcc.val65 = alloca %RCCValue, align 8
  %tag.ptr66 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val65, i32 0, i32 0
  store i64 3, ptr %tag.ptr66, align 4
  %payload.ptr67 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val65, i32 0, i32 1
  store ptr %int.ptr64, ptr %payload.ptr67, align 8
  store ptr %rcc.val65, ptr %arith.result39, align 8
  br label %arith.merge38

arith.merge38:                                    ; preds = %arith.int37, %arith.float36
  %arith.result.load68 = load ptr, ptr %arith.result39, align 8
  %tag.ptr69 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load68, i32 0, i32 0
  %tag70 = load i64, ptr %tag.ptr69, align 4
  %payload.ptr71 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load68, i32 0, i32 1
  %payload72 = load ptr, ptr %payload.ptr71, align 8
  switch i64 %tag70, label %sout.default78 [
    i64 1, label %sout.int73
    i64 2, label %sout.float74
    i64 3, label %sout.bool75
    i64 4, label %sout.str76
    i64 0, label %sout.null77
  ]

sout.int73:                                       ; preds = %arith.merge38
  %int.val80 = ptrtoint ptr %payload72 to i64
  %10 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val80)
  br label %sout.merge79

sout.float74:                                     ; preds = %arith.merge38
  %float.bits81 = ptrtoint ptr %payload72 to i64
  %float.val82 = bitcast i64 %float.bits81 to double
  %11 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val82)
  br label %sout.merge79

sout.bool75:                                      ; preds = %arith.merge38
  %bool.val83 = ptrtoint ptr %payload72 to i64
  %is.true84 = icmp ne i64 %bool.val83, 0
  br i1 %is.true84, label %sout.true85, label %sout.false86

sout.str76:                                       ; preds = %arith.merge38
  %12 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload72)
  br label %sout.merge79

sout.null77:                                      ; preds = %arith.merge38
  %13 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge79

sout.default78:                                   ; preds = %arith.merge38
  %14 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload72)
  br label %sout.merge79

sout.merge79:                                     ; preds = %sout.default78, %sout.null77, %sout.str76, %sout.bool.merge87, %sout.float74, %sout.int73
  %15 = call i32 @putchar(i32 10)
  %rcc.val88 = alloca %RCCValue, align 8
  %tag.ptr89 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val88, i32 0, i32 0
  store i64 0, ptr %tag.ptr89, align 4
  %payload.ptr90 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val88, i32 0, i32 1
  store ptr null, ptr %payload.ptr90, align 8
  %a.load91 = load ptr, ptr %a, align 8
  %b.load92 = load ptr, ptr %b, align 8
  %tag.ptr93 = getelementptr inbounds nuw %RCCValue, ptr %a.load91, i32 0, i32 0
  %tag94 = load i64, ptr %tag.ptr93, align 4
  %tag.ptr95 = getelementptr inbounds nuw %RCCValue, ptr %b.load92, i32 0, i32 0
  %tag96 = load i64, ptr %tag.ptr95, align 4
  %is_float.left97 = icmp eq i64 %tag94, 2
  %is_float.right98 = icmp eq i64 %tag96, 2
  %is_float99 = or i1 %is_float.left97, %is_float.right98
  store ptr null, ptr %arith.result103, align 8
  br i1 %is_float99, label %arith.float100, label %arith.int101

sout.true85:                                      ; preds = %sout.bool75
  %16 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge87

sout.false86:                                     ; preds = %sout.bool75
  %17 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge87

sout.bool.merge87:                                ; preds = %sout.false86, %sout.true85
  br label %sout.merge79

arith.float100:                                   ; preds = %sout.merge79
  %payload.ptr104 = getelementptr inbounds nuw %RCCValue, ptr %a.load91, i32 0, i32 1
  %payload105 = load ptr, ptr %payload.ptr104, align 8
  %payload.ptr106 = getelementptr inbounds nuw %RCCValue, ptr %b.load92, i32 0, i32 1
  %payload107 = load ptr, ptr %payload.ptr106, align 8
  %left.bits108 = ptrtoint ptr %payload105 to i64
  %right.bits109 = ptrtoint ptr %payload107 to i64
  %left.sitofp110 = sitofp i64 %left.bits108 to double
  %left.bitcast111 = bitcast i64 %left.bits108 to double
  %left.double112 = select i1 %is_float.left97, double %left.bitcast111, double %left.sitofp110
  %right.sitofp113 = sitofp i64 %right.bits109 to double
  %right.bitcast114 = bitcast i64 %right.bits109 to double
  %right.double115 = select i1 %is_float.right98, double %right.bitcast114, double %right.sitofp113
  %result.bits116 = bitcast double %left.double112 to i64
  %result.float.ptr117 = inttoptr i64 %result.bits116 to ptr
  %rcc.val118 = alloca %RCCValue, align 8
  %tag.ptr119 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val118, i32 0, i32 0
  store i64 2, ptr %tag.ptr119, align 4
  %payload.ptr120 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val118, i32 0, i32 1
  store ptr %result.float.ptr117, ptr %payload.ptr120, align 8
  store ptr %rcc.val118, ptr %arith.result103, align 8
  br label %arith.merge102

arith.int101:                                     ; preds = %sout.merge79
  %payload.ptr121 = getelementptr inbounds nuw %RCCValue, ptr %a.load91, i32 0, i32 1
  %payload122 = load ptr, ptr %payload.ptr121, align 8
  %payload.ptr123 = getelementptr inbounds nuw %RCCValue, ptr %b.load92, i32 0, i32 1
  %payload124 = load ptr, ptr %payload.ptr123, align 8
  %left.int125 = ptrtoint ptr %payload122 to i64
  %right.int126 = ptrtoint ptr %payload124 to i64
  %cmp.lt = icmp slt i64 %left.int125, %right.int126
  %cmp.ext127 = zext i1 %cmp.lt to i64
  %int.ptr128 = inttoptr i64 %cmp.ext127 to ptr
  %rcc.val129 = alloca %RCCValue, align 8
  %tag.ptr130 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val129, i32 0, i32 0
  store i64 3, ptr %tag.ptr130, align 4
  %payload.ptr131 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val129, i32 0, i32 1
  store ptr %int.ptr128, ptr %payload.ptr131, align 8
  store ptr %rcc.val129, ptr %arith.result103, align 8
  br label %arith.merge102

arith.merge102:                                   ; preds = %arith.int101, %arith.float100
  %arith.result.load132 = load ptr, ptr %arith.result103, align 8
  %tag.ptr133 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load132, i32 0, i32 0
  %tag134 = load i64, ptr %tag.ptr133, align 4
  %payload.ptr135 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load132, i32 0, i32 1
  %payload136 = load ptr, ptr %payload.ptr135, align 8
  switch i64 %tag134, label %sout.default142 [
    i64 1, label %sout.int137
    i64 2, label %sout.float138
    i64 3, label %sout.bool139
    i64 4, label %sout.str140
    i64 0, label %sout.null141
  ]

sout.int137:                                      ; preds = %arith.merge102
  %int.val144 = ptrtoint ptr %payload136 to i64
  %18 = call i32 (ptr, ...) @printf(ptr @.str.20, i64 %int.val144)
  br label %sout.merge143

sout.float138:                                    ; preds = %arith.merge102
  %float.bits145 = ptrtoint ptr %payload136 to i64
  %float.val146 = bitcast i64 %float.bits145 to double
  %19 = call i32 (ptr, ...) @printf(ptr @.str.21, double %float.val146)
  br label %sout.merge143

sout.bool139:                                     ; preds = %arith.merge102
  %bool.val147 = ptrtoint ptr %payload136 to i64
  %is.true148 = icmp ne i64 %bool.val147, 0
  br i1 %is.true148, label %sout.true149, label %sout.false150

sout.str140:                                      ; preds = %arith.merge102
  %20 = call i32 (ptr, ...) @printf(ptr @.str.26, ptr %payload136)
  br label %sout.merge143

sout.null141:                                     ; preds = %arith.merge102
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.merge143

sout.default142:                                  ; preds = %arith.merge102
  %22 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload136)
  br label %sout.merge143

sout.merge143:                                    ; preds = %sout.default142, %sout.null141, %sout.str140, %sout.bool.merge151, %sout.float138, %sout.int137
  %23 = call i32 @putchar(i32 10)
  %rcc.val152 = alloca %RCCValue, align 8
  %tag.ptr153 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val152, i32 0, i32 0
  store i64 0, ptr %tag.ptr153, align 4
  %payload.ptr154 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val152, i32 0, i32 1
  store ptr null, ptr %payload.ptr154, align 8
  %a.load155 = load ptr, ptr %a, align 8
  %b.load156 = load ptr, ptr %b, align 8
  %tag.ptr157 = getelementptr inbounds nuw %RCCValue, ptr %a.load155, i32 0, i32 0
  %tag158 = load i64, ptr %tag.ptr157, align 4
  %tag.ptr159 = getelementptr inbounds nuw %RCCValue, ptr %b.load156, i32 0, i32 0
  %tag160 = load i64, ptr %tag.ptr159, align 4
  %is_float.left161 = icmp eq i64 %tag158, 2
  %is_float.right162 = icmp eq i64 %tag160, 2
  %is_float163 = or i1 %is_float.left161, %is_float.right162
  store ptr null, ptr %arith.result167, align 8
  br i1 %is_float163, label %arith.float164, label %arith.int165

sout.true149:                                     ; preds = %sout.bool139
  %24 = call i32 (ptr, ...) @printf(ptr @.str.22, ptr @.str.23)
  br label %sout.bool.merge151

sout.false150:                                    ; preds = %sout.bool139
  %25 = call i32 (ptr, ...) @printf(ptr @.str.24, ptr @.str.25)
  br label %sout.bool.merge151

sout.bool.merge151:                               ; preds = %sout.false150, %sout.true149
  br label %sout.merge143

arith.float164:                                   ; preds = %sout.merge143
  %payload.ptr168 = getelementptr inbounds nuw %RCCValue, ptr %a.load155, i32 0, i32 1
  %payload169 = load ptr, ptr %payload.ptr168, align 8
  %payload.ptr170 = getelementptr inbounds nuw %RCCValue, ptr %b.load156, i32 0, i32 1
  %payload171 = load ptr, ptr %payload.ptr170, align 8
  %left.bits172 = ptrtoint ptr %payload169 to i64
  %right.bits173 = ptrtoint ptr %payload171 to i64
  %left.sitofp174 = sitofp i64 %left.bits172 to double
  %left.bitcast175 = bitcast i64 %left.bits172 to double
  %left.double176 = select i1 %is_float.left161, double %left.bitcast175, double %left.sitofp174
  %right.sitofp177 = sitofp i64 %right.bits173 to double
  %right.bitcast178 = bitcast i64 %right.bits173 to double
  %right.double179 = select i1 %is_float.right162, double %right.bitcast178, double %right.sitofp177
  %result.bits180 = bitcast double %left.double176 to i64
  %result.float.ptr181 = inttoptr i64 %result.bits180 to ptr
  %rcc.val182 = alloca %RCCValue, align 8
  %tag.ptr183 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val182, i32 0, i32 0
  store i64 2, ptr %tag.ptr183, align 4
  %payload.ptr184 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val182, i32 0, i32 1
  store ptr %result.float.ptr181, ptr %payload.ptr184, align 8
  store ptr %rcc.val182, ptr %arith.result167, align 8
  br label %arith.merge166

arith.int165:                                     ; preds = %sout.merge143
  %payload.ptr185 = getelementptr inbounds nuw %RCCValue, ptr %a.load155, i32 0, i32 1
  %payload186 = load ptr, ptr %payload.ptr185, align 8
  %payload.ptr187 = getelementptr inbounds nuw %RCCValue, ptr %b.load156, i32 0, i32 1
  %payload188 = load ptr, ptr %payload.ptr187, align 8
  %left.int189 = ptrtoint ptr %payload186 to i64
  %right.int190 = ptrtoint ptr %payload188 to i64
  %cmp.gt = icmp sgt i64 %left.int189, %right.int190
  %cmp.ext191 = zext i1 %cmp.gt to i64
  %int.ptr192 = inttoptr i64 %cmp.ext191 to ptr
  %rcc.val193 = alloca %RCCValue, align 8
  %tag.ptr194 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val193, i32 0, i32 0
  store i64 3, ptr %tag.ptr194, align 4
  %payload.ptr195 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val193, i32 0, i32 1
  store ptr %int.ptr192, ptr %payload.ptr195, align 8
  store ptr %rcc.val193, ptr %arith.result167, align 8
  br label %arith.merge166

arith.merge166:                                   ; preds = %arith.int165, %arith.float164
  %arith.result.load196 = load ptr, ptr %arith.result167, align 8
  %tag.ptr197 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load196, i32 0, i32 0
  %tag198 = load i64, ptr %tag.ptr197, align 4
  %payload.ptr199 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load196, i32 0, i32 1
  %payload200 = load ptr, ptr %payload.ptr199, align 8
  switch i64 %tag198, label %sout.default206 [
    i64 1, label %sout.int201
    i64 2, label %sout.float202
    i64 3, label %sout.bool203
    i64 4, label %sout.str204
    i64 0, label %sout.null205
  ]

sout.int201:                                      ; preds = %arith.merge166
  %int.val208 = ptrtoint ptr %payload200 to i64
  %26 = call i32 (ptr, ...) @printf(ptr @.str.30, i64 %int.val208)
  br label %sout.merge207

sout.float202:                                    ; preds = %arith.merge166
  %float.bits209 = ptrtoint ptr %payload200 to i64
  %float.val210 = bitcast i64 %float.bits209 to double
  %27 = call i32 (ptr, ...) @printf(ptr @.str.31, double %float.val210)
  br label %sout.merge207

sout.bool203:                                     ; preds = %arith.merge166
  %bool.val211 = ptrtoint ptr %payload200 to i64
  %is.true212 = icmp ne i64 %bool.val211, 0
  br i1 %is.true212, label %sout.true213, label %sout.false214

sout.str204:                                      ; preds = %arith.merge166
  %28 = call i32 (ptr, ...) @printf(ptr @.str.36, ptr %payload200)
  br label %sout.merge207

sout.null205:                                     ; preds = %arith.merge166
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.merge207

sout.default206:                                  ; preds = %arith.merge166
  %30 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload200)
  br label %sout.merge207

sout.merge207:                                    ; preds = %sout.default206, %sout.null205, %sout.str204, %sout.bool.merge215, %sout.float202, %sout.int201
  %31 = call i32 @putchar(i32 10)
  %rcc.val216 = alloca %RCCValue, align 8
  %tag.ptr217 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val216, i32 0, i32 0
  store i64 0, ptr %tag.ptr217, align 4
  %payload.ptr218 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val216, i32 0, i32 1
  store ptr null, ptr %payload.ptr218, align 8
  %a.load219 = load ptr, ptr %a, align 8
  %rcc.val220 = alloca %RCCValue, align 8
  %tag.ptr221 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 0
  store i64 1, ptr %tag.ptr221, align 4
  %payload.ptr222 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr222, align 8
  %tag.ptr223 = getelementptr inbounds nuw %RCCValue, ptr %a.load219, i32 0, i32 0
  %tag224 = load i64, ptr %tag.ptr223, align 4
  %tag.ptr225 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 0
  %tag226 = load i64, ptr %tag.ptr225, align 4
  %is_float.left227 = icmp eq i64 %tag224, 2
  %is_float.right228 = icmp eq i64 %tag226, 2
  %is_float229 = or i1 %is_float.left227, %is_float.right228
  store ptr null, ptr %arith.result233, align 8
  br i1 %is_float229, label %arith.float230, label %arith.int231

sout.true213:                                     ; preds = %sout.bool203
  %32 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr @.str.33)
  br label %sout.bool.merge215

sout.false214:                                    ; preds = %sout.bool203
  %33 = call i32 (ptr, ...) @printf(ptr @.str.34, ptr @.str.35)
  br label %sout.bool.merge215

sout.bool.merge215:                               ; preds = %sout.false214, %sout.true213
  br label %sout.merge207

arith.float230:                                   ; preds = %sout.merge207
  %payload.ptr234 = getelementptr inbounds nuw %RCCValue, ptr %a.load219, i32 0, i32 1
  %payload235 = load ptr, ptr %payload.ptr234, align 8
  %payload.ptr236 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 1
  %payload237 = load ptr, ptr %payload.ptr236, align 8
  %left.bits238 = ptrtoint ptr %payload235 to i64
  %right.bits239 = ptrtoint ptr %payload237 to i64
  %left.sitofp240 = sitofp i64 %left.bits238 to double
  %left.bitcast241 = bitcast i64 %left.bits238 to double
  %left.double242 = select i1 %is_float.left227, double %left.bitcast241, double %left.sitofp240
  %right.sitofp243 = sitofp i64 %right.bits239 to double
  %right.bitcast244 = bitcast i64 %right.bits239 to double
  %right.double245 = select i1 %is_float.right228, double %right.bitcast244, double %right.sitofp243
  %result.bits246 = bitcast double %left.double242 to i64
  %result.float.ptr247 = inttoptr i64 %result.bits246 to ptr
  %rcc.val248 = alloca %RCCValue, align 8
  %tag.ptr249 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val248, i32 0, i32 0
  store i64 2, ptr %tag.ptr249, align 4
  %payload.ptr250 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val248, i32 0, i32 1
  store ptr %result.float.ptr247, ptr %payload.ptr250, align 8
  store ptr %rcc.val248, ptr %arith.result233, align 8
  br label %arith.merge232

arith.int231:                                     ; preds = %sout.merge207
  %payload.ptr251 = getelementptr inbounds nuw %RCCValue, ptr %a.load219, i32 0, i32 1
  %payload252 = load ptr, ptr %payload.ptr251, align 8
  %payload.ptr253 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val220, i32 0, i32 1
  %payload254 = load ptr, ptr %payload.ptr253, align 8
  %left.int255 = ptrtoint ptr %payload252 to i64
  %right.int256 = ptrtoint ptr %payload254 to i64
  %cmp.le = icmp sle i64 %left.int255, %right.int256
  %cmp.ext257 = zext i1 %cmp.le to i64
  %int.ptr258 = inttoptr i64 %cmp.ext257 to ptr
  %rcc.val259 = alloca %RCCValue, align 8
  %tag.ptr260 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val259, i32 0, i32 0
  store i64 3, ptr %tag.ptr260, align 4
  %payload.ptr261 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val259, i32 0, i32 1
  store ptr %int.ptr258, ptr %payload.ptr261, align 8
  store ptr %rcc.val259, ptr %arith.result233, align 8
  br label %arith.merge232

arith.merge232:                                   ; preds = %arith.int231, %arith.float230
  %arith.result.load262 = load ptr, ptr %arith.result233, align 8
  %tag.ptr263 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load262, i32 0, i32 0
  %tag264 = load i64, ptr %tag.ptr263, align 4
  %payload.ptr265 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load262, i32 0, i32 1
  %payload266 = load ptr, ptr %payload.ptr265, align 8
  switch i64 %tag264, label %sout.default272 [
    i64 1, label %sout.int267
    i64 2, label %sout.float268
    i64 3, label %sout.bool269
    i64 4, label %sout.str270
    i64 0, label %sout.null271
  ]

sout.int267:                                      ; preds = %arith.merge232
  %int.val274 = ptrtoint ptr %payload266 to i64
  %34 = call i32 (ptr, ...) @printf(ptr @.str.40, i64 %int.val274)
  br label %sout.merge273

sout.float268:                                    ; preds = %arith.merge232
  %float.bits275 = ptrtoint ptr %payload266 to i64
  %float.val276 = bitcast i64 %float.bits275 to double
  %35 = call i32 (ptr, ...) @printf(ptr @.str.41, double %float.val276)
  br label %sout.merge273

sout.bool269:                                     ; preds = %arith.merge232
  %bool.val277 = ptrtoint ptr %payload266 to i64
  %is.true278 = icmp ne i64 %bool.val277, 0
  br i1 %is.true278, label %sout.true279, label %sout.false280

sout.str270:                                      ; preds = %arith.merge232
  %36 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr %payload266)
  br label %sout.merge273

sout.null271:                                     ; preds = %arith.merge232
  %37 = call i32 (ptr, ...) @printf(ptr @.str.47, ptr @.str.48)
  br label %sout.merge273

sout.default272:                                  ; preds = %arith.merge232
  %38 = call i32 (ptr, ...) @printf(ptr @.str.49, ptr %payload266)
  br label %sout.merge273

sout.merge273:                                    ; preds = %sout.default272, %sout.null271, %sout.str270, %sout.bool.merge281, %sout.float268, %sout.int267
  %39 = call i32 @putchar(i32 10)
  %rcc.val282 = alloca %RCCValue, align 8
  %tag.ptr283 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val282, i32 0, i32 0
  store i64 0, ptr %tag.ptr283, align 4
  %payload.ptr284 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val282, i32 0, i32 1
  store ptr null, ptr %payload.ptr284, align 8
  %a.load285 = load ptr, ptr %a, align 8
  %rcc.val286 = alloca %RCCValue, align 8
  %tag.ptr287 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 0
  store i64 1, ptr %tag.ptr287, align 4
  %payload.ptr288 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr288, align 8
  %tag.ptr289 = getelementptr inbounds nuw %RCCValue, ptr %a.load285, i32 0, i32 0
  %tag290 = load i64, ptr %tag.ptr289, align 4
  %tag.ptr291 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 0
  %tag292 = load i64, ptr %tag.ptr291, align 4
  %is_float.left293 = icmp eq i64 %tag290, 2
  %is_float.right294 = icmp eq i64 %tag292, 2
  %is_float295 = or i1 %is_float.left293, %is_float.right294
  store ptr null, ptr %arith.result299, align 8
  br i1 %is_float295, label %arith.float296, label %arith.int297

sout.true279:                                     ; preds = %sout.bool269
  %40 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr @.str.43)
  br label %sout.bool.merge281

sout.false280:                                    ; preds = %sout.bool269
  %41 = call i32 (ptr, ...) @printf(ptr @.str.44, ptr @.str.45)
  br label %sout.bool.merge281

sout.bool.merge281:                               ; preds = %sout.false280, %sout.true279
  br label %sout.merge273

arith.float296:                                   ; preds = %sout.merge273
  %payload.ptr300 = getelementptr inbounds nuw %RCCValue, ptr %a.load285, i32 0, i32 1
  %payload301 = load ptr, ptr %payload.ptr300, align 8
  %payload.ptr302 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 1
  %payload303 = load ptr, ptr %payload.ptr302, align 8
  %left.bits304 = ptrtoint ptr %payload301 to i64
  %right.bits305 = ptrtoint ptr %payload303 to i64
  %left.sitofp306 = sitofp i64 %left.bits304 to double
  %left.bitcast307 = bitcast i64 %left.bits304 to double
  %left.double308 = select i1 %is_float.left293, double %left.bitcast307, double %left.sitofp306
  %right.sitofp309 = sitofp i64 %right.bits305 to double
  %right.bitcast310 = bitcast i64 %right.bits305 to double
  %right.double311 = select i1 %is_float.right294, double %right.bitcast310, double %right.sitofp309
  %result.bits312 = bitcast double %left.double308 to i64
  %result.float.ptr313 = inttoptr i64 %result.bits312 to ptr
  %rcc.val314 = alloca %RCCValue, align 8
  %tag.ptr315 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val314, i32 0, i32 0
  store i64 2, ptr %tag.ptr315, align 4
  %payload.ptr316 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val314, i32 0, i32 1
  store ptr %result.float.ptr313, ptr %payload.ptr316, align 8
  store ptr %rcc.val314, ptr %arith.result299, align 8
  br label %arith.merge298

arith.int297:                                     ; preds = %sout.merge273
  %payload.ptr317 = getelementptr inbounds nuw %RCCValue, ptr %a.load285, i32 0, i32 1
  %payload318 = load ptr, ptr %payload.ptr317, align 8
  %payload.ptr319 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val286, i32 0, i32 1
  %payload320 = load ptr, ptr %payload.ptr319, align 8
  %left.int321 = ptrtoint ptr %payload318 to i64
  %right.int322 = ptrtoint ptr %payload320 to i64
  %cmp.ge = icmp sge i64 %left.int321, %right.int322
  %cmp.ext323 = zext i1 %cmp.ge to i64
  %int.ptr324 = inttoptr i64 %cmp.ext323 to ptr
  %rcc.val325 = alloca %RCCValue, align 8
  %tag.ptr326 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val325, i32 0, i32 0
  store i64 3, ptr %tag.ptr326, align 4
  %payload.ptr327 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val325, i32 0, i32 1
  store ptr %int.ptr324, ptr %payload.ptr327, align 8
  store ptr %rcc.val325, ptr %arith.result299, align 8
  br label %arith.merge298

arith.merge298:                                   ; preds = %arith.int297, %arith.float296
  %arith.result.load328 = load ptr, ptr %arith.result299, align 8
  %tag.ptr329 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load328, i32 0, i32 0
  %tag330 = load i64, ptr %tag.ptr329, align 4
  %payload.ptr331 = getelementptr inbounds nuw %RCCValue, ptr %arith.result.load328, i32 0, i32 1
  %payload332 = load ptr, ptr %payload.ptr331, align 8
  switch i64 %tag330, label %sout.default338 [
    i64 1, label %sout.int333
    i64 2, label %sout.float334
    i64 3, label %sout.bool335
    i64 4, label %sout.str336
    i64 0, label %sout.null337
  ]

sout.int333:                                      ; preds = %arith.merge298
  %int.val340 = ptrtoint ptr %payload332 to i64
  %42 = call i32 (ptr, ...) @printf(ptr @.str.50, i64 %int.val340)
  br label %sout.merge339

sout.float334:                                    ; preds = %arith.merge298
  %float.bits341 = ptrtoint ptr %payload332 to i64
  %float.val342 = bitcast i64 %float.bits341 to double
  %43 = call i32 (ptr, ...) @printf(ptr @.str.51, double %float.val342)
  br label %sout.merge339

sout.bool335:                                     ; preds = %arith.merge298
  %bool.val343 = ptrtoint ptr %payload332 to i64
  %is.true344 = icmp ne i64 %bool.val343, 0
  br i1 %is.true344, label %sout.true345, label %sout.false346

sout.str336:                                      ; preds = %arith.merge298
  %44 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr %payload332)
  br label %sout.merge339

sout.null337:                                     ; preds = %arith.merge298
  %45 = call i32 (ptr, ...) @printf(ptr @.str.57, ptr @.str.58)
  br label %sout.merge339

sout.default338:                                  ; preds = %arith.merge298
  %46 = call i32 (ptr, ...) @printf(ptr @.str.59, ptr %payload332)
  br label %sout.merge339

sout.merge339:                                    ; preds = %sout.default338, %sout.null337, %sout.str336, %sout.bool.merge347, %sout.float334, %sout.int333
  %47 = call i32 @putchar(i32 10)
  %rcc.val348 = alloca %RCCValue, align 8
  %tag.ptr349 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val348, i32 0, i32 0
  store i64 0, ptr %tag.ptr349, align 4
  %payload.ptr350 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val348, i32 0, i32 1
  store ptr null, ptr %payload.ptr350, align 8
  %rcc.val351 = alloca %RCCValue, align 8
  %tag.ptr352 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val351, i32 0, i32 0
  store i64 3, ptr %tag.ptr352, align 4
  %payload.ptr353 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val351, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr353, align 8
  %tag.ptr354 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val351, i32 0, i32 0
  %tag355 = load i64, ptr %tag.ptr354, align 4
  %payload.ptr356 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val351, i32 0, i32 1
  %payload357 = load ptr, ptr %payload.ptr356, align 8
  %is_null = icmp eq i64 %tag355, 0
  %is_int = icmp eq i64 %tag355, 1
  %is_bool = icmp eq i64 %tag355, 3
  %is_bool_or_int = or i1 %is_bool, %is_int
  %payload.int = ptrtoint ptr %payload357 to i64
  %payload_nz = icmp ne i64 %payload.int, 0
  %not_null = xor i1 %is_null, true
  %bool_int_result = select i1 %is_bool_or_int, i1 %payload_nz, i1 true
  %coerced_bool = select i1 %is_null, i1 false, i1 %bool_int_result
  %rcc.val358 = alloca %RCCValue, align 8
  %tag.ptr359 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val358, i32 0, i32 0
  store i64 3, ptr %tag.ptr359, align 4
  %payload.ptr360 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val358, i32 0, i32 1
  store ptr null, ptr %payload.ptr360, align 8
  br i1 %coerced_bool, label %eval.right, label %merge

sout.true345:                                     ; preds = %sout.bool335
  %48 = call i32 (ptr, ...) @printf(ptr @.str.52, ptr @.str.53)
  br label %sout.bool.merge347

sout.false346:                                    ; preds = %sout.bool335
  %49 = call i32 (ptr, ...) @printf(ptr @.str.54, ptr @.str.55)
  br label %sout.bool.merge347

sout.bool.merge347:                               ; preds = %sout.false346, %sout.true345
  br label %sout.merge339

eval.right:                                       ; preds = %sout.merge339
  %rcc.val361 = alloca %RCCValue, align 8
  %tag.ptr362 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val361, i32 0, i32 0
  store i64 3, ptr %tag.ptr362, align 4
  %payload.ptr363 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val361, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr363, align 8
  br label %merge

merge:                                            ; preds = %eval.right, %sout.merge339
  %logical.phi = phi ptr [ %rcc.val358, %sout.merge339 ], [ %rcc.val361, %eval.right ]
  %tag.ptr364 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi, i32 0, i32 0
  %tag365 = load i64, ptr %tag.ptr364, align 4
  %payload.ptr366 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi, i32 0, i32 1
  %payload367 = load ptr, ptr %payload.ptr366, align 8
  switch i64 %tag365, label %sout.default373 [
    i64 1, label %sout.int368
    i64 2, label %sout.float369
    i64 3, label %sout.bool370
    i64 4, label %sout.str371
    i64 0, label %sout.null372
  ]

sout.int368:                                      ; preds = %merge
  %int.val375 = ptrtoint ptr %payload367 to i64
  %50 = call i32 (ptr, ...) @printf(ptr @.str.60, i64 %int.val375)
  br label %sout.merge374

sout.float369:                                    ; preds = %merge
  %float.bits376 = ptrtoint ptr %payload367 to i64
  %float.val377 = bitcast i64 %float.bits376 to double
  %51 = call i32 (ptr, ...) @printf(ptr @.str.61, double %float.val377)
  br label %sout.merge374

sout.bool370:                                     ; preds = %merge
  %bool.val378 = ptrtoint ptr %payload367 to i64
  %is.true379 = icmp ne i64 %bool.val378, 0
  br i1 %is.true379, label %sout.true380, label %sout.false381

sout.str371:                                      ; preds = %merge
  %52 = call i32 (ptr, ...) @printf(ptr @.str.66, ptr %payload367)
  br label %sout.merge374

sout.null372:                                     ; preds = %merge
  %53 = call i32 (ptr, ...) @printf(ptr @.str.67, ptr @.str.68)
  br label %sout.merge374

sout.default373:                                  ; preds = %merge
  %54 = call i32 (ptr, ...) @printf(ptr @.str.69, ptr %payload367)
  br label %sout.merge374

sout.merge374:                                    ; preds = %sout.default373, %sout.null372, %sout.str371, %sout.bool.merge382, %sout.float369, %sout.int368
  %55 = call i32 @putchar(i32 10)
  %rcc.val383 = alloca %RCCValue, align 8
  %tag.ptr384 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val383, i32 0, i32 0
  store i64 0, ptr %tag.ptr384, align 4
  %payload.ptr385 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val383, i32 0, i32 1
  store ptr null, ptr %payload.ptr385, align 8
  %rcc.val388 = alloca %RCCValue, align 8
  %tag.ptr389 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val388, i32 0, i32 0
  store i64 3, ptr %tag.ptr389, align 4
  %payload.ptr390 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val388, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr390, align 8
  %tag.ptr391 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val388, i32 0, i32 0
  %tag392 = load i64, ptr %tag.ptr391, align 4
  %payload.ptr393 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val388, i32 0, i32 1
  %payload394 = load ptr, ptr %payload.ptr393, align 8
  %is_null395 = icmp eq i64 %tag392, 0
  %is_int396 = icmp eq i64 %tag392, 1
  %is_bool397 = icmp eq i64 %tag392, 3
  %is_bool_or_int398 = or i1 %is_bool397, %is_int396
  %payload.int399 = ptrtoint ptr %payload394 to i64
  %payload_nz400 = icmp ne i64 %payload.int399, 0
  %not_null401 = xor i1 %is_null395, true
  %bool_int_result402 = select i1 %is_bool_or_int398, i1 %payload_nz400, i1 true
  %coerced_bool403 = select i1 %is_null395, i1 false, i1 %bool_int_result402
  %rcc.val404 = alloca %RCCValue, align 8
  %tag.ptr405 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val404, i32 0, i32 0
  store i64 3, ptr %tag.ptr405, align 4
  %payload.ptr406 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val404, i32 0, i32 1
  store ptr null, ptr %payload.ptr406, align 8
  br i1 %coerced_bool403, label %eval.right386, label %merge387

sout.true380:                                     ; preds = %sout.bool370
  %56 = call i32 (ptr, ...) @printf(ptr @.str.62, ptr @.str.63)
  br label %sout.bool.merge382

sout.false381:                                    ; preds = %sout.bool370
  %57 = call i32 (ptr, ...) @printf(ptr @.str.64, ptr @.str.65)
  br label %sout.bool.merge382

sout.bool.merge382:                               ; preds = %sout.false381, %sout.true380
  br label %sout.merge374

eval.right386:                                    ; preds = %sout.merge374
  %rcc.val407 = alloca %RCCValue, align 8
  %tag.ptr408 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val407, i32 0, i32 0
  store i64 3, ptr %tag.ptr408, align 4
  %payload.ptr409 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val407, i32 0, i32 1
  store ptr null, ptr %payload.ptr409, align 8
  br label %merge387

merge387:                                         ; preds = %eval.right386, %sout.merge374
  %logical.phi410 = phi ptr [ %rcc.val404, %sout.merge374 ], [ %rcc.val407, %eval.right386 ]
  %tag.ptr411 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi410, i32 0, i32 0
  %tag412 = load i64, ptr %tag.ptr411, align 4
  %payload.ptr413 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi410, i32 0, i32 1
  %payload414 = load ptr, ptr %payload.ptr413, align 8
  switch i64 %tag412, label %sout.default420 [
    i64 1, label %sout.int415
    i64 2, label %sout.float416
    i64 3, label %sout.bool417
    i64 4, label %sout.str418
    i64 0, label %sout.null419
  ]

sout.int415:                                      ; preds = %merge387
  %int.val422 = ptrtoint ptr %payload414 to i64
  %58 = call i32 (ptr, ...) @printf(ptr @.str.70, i64 %int.val422)
  br label %sout.merge421

sout.float416:                                    ; preds = %merge387
  %float.bits423 = ptrtoint ptr %payload414 to i64
  %float.val424 = bitcast i64 %float.bits423 to double
  %59 = call i32 (ptr, ...) @printf(ptr @.str.71, double %float.val424)
  br label %sout.merge421

sout.bool417:                                     ; preds = %merge387
  %bool.val425 = ptrtoint ptr %payload414 to i64
  %is.true426 = icmp ne i64 %bool.val425, 0
  br i1 %is.true426, label %sout.true427, label %sout.false428

sout.str418:                                      ; preds = %merge387
  %60 = call i32 (ptr, ...) @printf(ptr @.str.76, ptr %payload414)
  br label %sout.merge421

sout.null419:                                     ; preds = %merge387
  %61 = call i32 (ptr, ...) @printf(ptr @.str.77, ptr @.str.78)
  br label %sout.merge421

sout.default420:                                  ; preds = %merge387
  %62 = call i32 (ptr, ...) @printf(ptr @.str.79, ptr %payload414)
  br label %sout.merge421

sout.merge421:                                    ; preds = %sout.default420, %sout.null419, %sout.str418, %sout.bool.merge429, %sout.float416, %sout.int415
  %63 = call i32 @putchar(i32 10)
  %rcc.val430 = alloca %RCCValue, align 8
  %tag.ptr431 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val430, i32 0, i32 0
  store i64 0, ptr %tag.ptr431, align 4
  %payload.ptr432 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val430, i32 0, i32 1
  store ptr null, ptr %payload.ptr432, align 8
  %rcc.val435 = alloca %RCCValue, align 8
  %tag.ptr436 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val435, i32 0, i32 0
  store i64 3, ptr %tag.ptr436, align 4
  %payload.ptr437 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val435, i32 0, i32 1
  store ptr null, ptr %payload.ptr437, align 8
  %tag.ptr438 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val435, i32 0, i32 0
  %tag439 = load i64, ptr %tag.ptr438, align 4
  %payload.ptr440 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val435, i32 0, i32 1
  %payload441 = load ptr, ptr %payload.ptr440, align 8
  %is_null442 = icmp eq i64 %tag439, 0
  %is_int443 = icmp eq i64 %tag439, 1
  %is_bool444 = icmp eq i64 %tag439, 3
  %is_bool_or_int445 = or i1 %is_bool444, %is_int443
  %payload.int446 = ptrtoint ptr %payload441 to i64
  %payload_nz447 = icmp ne i64 %payload.int446, 0
  %not_null448 = xor i1 %is_null442, true
  %bool_int_result449 = select i1 %is_bool_or_int445, i1 %payload_nz447, i1 true
  %coerced_bool450 = select i1 %is_null442, i1 false, i1 %bool_int_result449
  %rcc.val451 = alloca %RCCValue, align 8
  %tag.ptr452 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val451, i32 0, i32 0
  store i64 3, ptr %tag.ptr452, align 4
  %payload.ptr453 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val451, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr453, align 8
  br i1 %coerced_bool450, label %merge434, label %eval.right433

sout.true427:                                     ; preds = %sout.bool417
  %64 = call i32 (ptr, ...) @printf(ptr @.str.72, ptr @.str.73)
  br label %sout.bool.merge429

sout.false428:                                    ; preds = %sout.bool417
  %65 = call i32 (ptr, ...) @printf(ptr @.str.74, ptr @.str.75)
  br label %sout.bool.merge429

sout.bool.merge429:                               ; preds = %sout.false428, %sout.true427
  br label %sout.merge421

eval.right433:                                    ; preds = %sout.merge421
  %rcc.val454 = alloca %RCCValue, align 8
  %tag.ptr455 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val454, i32 0, i32 0
  store i64 3, ptr %tag.ptr455, align 4
  %payload.ptr456 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val454, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr456, align 8
  br label %merge434

merge434:                                         ; preds = %eval.right433, %sout.merge421
  %logical.phi457 = phi ptr [ %rcc.val451, %sout.merge421 ], [ %rcc.val454, %eval.right433 ]
  %tag.ptr458 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi457, i32 0, i32 0
  %tag459 = load i64, ptr %tag.ptr458, align 4
  %payload.ptr460 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi457, i32 0, i32 1
  %payload461 = load ptr, ptr %payload.ptr460, align 8
  switch i64 %tag459, label %sout.default467 [
    i64 1, label %sout.int462
    i64 2, label %sout.float463
    i64 3, label %sout.bool464
    i64 4, label %sout.str465
    i64 0, label %sout.null466
  ]

sout.int462:                                      ; preds = %merge434
  %int.val469 = ptrtoint ptr %payload461 to i64
  %66 = call i32 (ptr, ...) @printf(ptr @.str.80, i64 %int.val469)
  br label %sout.merge468

sout.float463:                                    ; preds = %merge434
  %float.bits470 = ptrtoint ptr %payload461 to i64
  %float.val471 = bitcast i64 %float.bits470 to double
  %67 = call i32 (ptr, ...) @printf(ptr @.str.81, double %float.val471)
  br label %sout.merge468

sout.bool464:                                     ; preds = %merge434
  %bool.val472 = ptrtoint ptr %payload461 to i64
  %is.true473 = icmp ne i64 %bool.val472, 0
  br i1 %is.true473, label %sout.true474, label %sout.false475

sout.str465:                                      ; preds = %merge434
  %68 = call i32 (ptr, ...) @printf(ptr @.str.86, ptr %payload461)
  br label %sout.merge468

sout.null466:                                     ; preds = %merge434
  %69 = call i32 (ptr, ...) @printf(ptr @.str.87, ptr @.str.88)
  br label %sout.merge468

sout.default467:                                  ; preds = %merge434
  %70 = call i32 (ptr, ...) @printf(ptr @.str.89, ptr %payload461)
  br label %sout.merge468

sout.merge468:                                    ; preds = %sout.default467, %sout.null466, %sout.str465, %sout.bool.merge476, %sout.float463, %sout.int462
  %71 = call i32 @putchar(i32 10)
  %rcc.val477 = alloca %RCCValue, align 8
  %tag.ptr478 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val477, i32 0, i32 0
  store i64 0, ptr %tag.ptr478, align 4
  %payload.ptr479 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val477, i32 0, i32 1
  store ptr null, ptr %payload.ptr479, align 8
  %rcc.val482 = alloca %RCCValue, align 8
  %tag.ptr483 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val482, i32 0, i32 0
  store i64 3, ptr %tag.ptr483, align 4
  %payload.ptr484 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val482, i32 0, i32 1
  store ptr null, ptr %payload.ptr484, align 8
  %tag.ptr485 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val482, i32 0, i32 0
  %tag486 = load i64, ptr %tag.ptr485, align 4
  %payload.ptr487 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val482, i32 0, i32 1
  %payload488 = load ptr, ptr %payload.ptr487, align 8
  %is_null489 = icmp eq i64 %tag486, 0
  %is_int490 = icmp eq i64 %tag486, 1
  %is_bool491 = icmp eq i64 %tag486, 3
  %is_bool_or_int492 = or i1 %is_bool491, %is_int490
  %payload.int493 = ptrtoint ptr %payload488 to i64
  %payload_nz494 = icmp ne i64 %payload.int493, 0
  %not_null495 = xor i1 %is_null489, true
  %bool_int_result496 = select i1 %is_bool_or_int492, i1 %payload_nz494, i1 true
  %coerced_bool497 = select i1 %is_null489, i1 false, i1 %bool_int_result496
  %rcc.val498 = alloca %RCCValue, align 8
  %tag.ptr499 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val498, i32 0, i32 0
  store i64 3, ptr %tag.ptr499, align 4
  %payload.ptr500 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val498, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr500, align 8
  br i1 %coerced_bool497, label %merge481, label %eval.right480

sout.true474:                                     ; preds = %sout.bool464
  %72 = call i32 (ptr, ...) @printf(ptr @.str.82, ptr @.str.83)
  br label %sout.bool.merge476

sout.false475:                                    ; preds = %sout.bool464
  %73 = call i32 (ptr, ...) @printf(ptr @.str.84, ptr @.str.85)
  br label %sout.bool.merge476

sout.bool.merge476:                               ; preds = %sout.false475, %sout.true474
  br label %sout.merge468

eval.right480:                                    ; preds = %sout.merge468
  %rcc.val501 = alloca %RCCValue, align 8
  %tag.ptr502 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val501, i32 0, i32 0
  store i64 3, ptr %tag.ptr502, align 4
  %payload.ptr503 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val501, i32 0, i32 1
  store ptr null, ptr %payload.ptr503, align 8
  br label %merge481

merge481:                                         ; preds = %eval.right480, %sout.merge468
  %logical.phi504 = phi ptr [ %rcc.val498, %sout.merge468 ], [ %rcc.val501, %eval.right480 ]
  %tag.ptr505 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi504, i32 0, i32 0
  %tag506 = load i64, ptr %tag.ptr505, align 4
  %payload.ptr507 = getelementptr inbounds nuw %RCCValue, ptr %logical.phi504, i32 0, i32 1
  %payload508 = load ptr, ptr %payload.ptr507, align 8
  switch i64 %tag506, label %sout.default514 [
    i64 1, label %sout.int509
    i64 2, label %sout.float510
    i64 3, label %sout.bool511
    i64 4, label %sout.str512
    i64 0, label %sout.null513
  ]

sout.int509:                                      ; preds = %merge481
  %int.val516 = ptrtoint ptr %payload508 to i64
  %74 = call i32 (ptr, ...) @printf(ptr @.str.90, i64 %int.val516)
  br label %sout.merge515

sout.float510:                                    ; preds = %merge481
  %float.bits517 = ptrtoint ptr %payload508 to i64
  %float.val518 = bitcast i64 %float.bits517 to double
  %75 = call i32 (ptr, ...) @printf(ptr @.str.91, double %float.val518)
  br label %sout.merge515

sout.bool511:                                     ; preds = %merge481
  %bool.val519 = ptrtoint ptr %payload508 to i64
  %is.true520 = icmp ne i64 %bool.val519, 0
  br i1 %is.true520, label %sout.true521, label %sout.false522

sout.str512:                                      ; preds = %merge481
  %76 = call i32 (ptr, ...) @printf(ptr @.str.96, ptr %payload508)
  br label %sout.merge515

sout.null513:                                     ; preds = %merge481
  %77 = call i32 (ptr, ...) @printf(ptr @.str.97, ptr @.str.98)
  br label %sout.merge515

sout.default514:                                  ; preds = %merge481
  %78 = call i32 (ptr, ...) @printf(ptr @.str.99, ptr %payload508)
  br label %sout.merge515

sout.merge515:                                    ; preds = %sout.default514, %sout.null513, %sout.str512, %sout.bool.merge523, %sout.float510, %sout.int509
  %79 = call i32 @putchar(i32 10)
  %rcc.val524 = alloca %RCCValue, align 8
  %tag.ptr525 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val524, i32 0, i32 0
  store i64 0, ptr %tag.ptr525, align 4
  %payload.ptr526 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val524, i32 0, i32 1
  store ptr null, ptr %payload.ptr526, align 8
  %rcc.val527 = alloca %RCCValue, align 8
  %tag.ptr528 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val527, i32 0, i32 0
  store i64 3, ptr %tag.ptr528, align 4
  %payload.ptr529 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val527, i32 0, i32 1
  store ptr inttoptr (i64 1 to ptr), ptr %payload.ptr529, align 8
  %unary.int = ptrtoint ptr %rcc.val527 to i64
  %tag.ptr530 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val527, i32 0, i32 0
  %tag531 = load i64, ptr %tag.ptr530, align 4
  %payload.ptr532 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val527, i32 0, i32 1
  %payload533 = load ptr, ptr %payload.ptr532, align 8
  %is_null534 = icmp eq i64 %tag531, 0
  %is_int535 = icmp eq i64 %tag531, 1
  %is_bool536 = icmp eq i64 %tag531, 3
  %is_bool_or_int537 = or i1 %is_bool536, %is_int535
  %payload.int538 = ptrtoint ptr %payload533 to i64
  %payload_nz539 = icmp ne i64 %payload.int538, 0
  %not_null540 = xor i1 %is_null534, true
  %bool_int_result541 = select i1 %is_bool_or_int537, i1 %payload_nz539, i1 true
  %coerced_bool542 = select i1 %is_null534, i1 false, i1 %bool_int_result541
  %not = xor i1 %coerced_bool542, true
  %not.ext = zext i1 %not to i64
  %not.is_true = icmp ne i64 %not.ext, 0
  %not.payload = select i1 %not.is_true, ptr inttoptr (i64 1 to ptr), ptr null
  %rcc.val543 = alloca %RCCValue, align 8
  %tag.ptr544 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val543, i32 0, i32 0
  store i64 3, ptr %tag.ptr544, align 4
  %payload.ptr545 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val543, i32 0, i32 1
  store ptr %not.payload, ptr %payload.ptr545, align 8
  %tag.ptr546 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val543, i32 0, i32 0
  %tag547 = load i64, ptr %tag.ptr546, align 4
  %payload.ptr548 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val543, i32 0, i32 1
  %payload549 = load ptr, ptr %payload.ptr548, align 8
  switch i64 %tag547, label %sout.default555 [
    i64 1, label %sout.int550
    i64 2, label %sout.float551
    i64 3, label %sout.bool552
    i64 4, label %sout.str553
    i64 0, label %sout.null554
  ]

sout.true521:                                     ; preds = %sout.bool511
  %80 = call i32 (ptr, ...) @printf(ptr @.str.92, ptr @.str.93)
  br label %sout.bool.merge523

sout.false522:                                    ; preds = %sout.bool511
  %81 = call i32 (ptr, ...) @printf(ptr @.str.94, ptr @.str.95)
  br label %sout.bool.merge523

sout.bool.merge523:                               ; preds = %sout.false522, %sout.true521
  br label %sout.merge515

sout.int550:                                      ; preds = %sout.merge515
  %int.val557 = ptrtoint ptr %payload549 to i64
  %82 = call i32 (ptr, ...) @printf(ptr @.str.100, i64 %int.val557)
  br label %sout.merge556

sout.float551:                                    ; preds = %sout.merge515
  %float.bits558 = ptrtoint ptr %payload549 to i64
  %float.val559 = bitcast i64 %float.bits558 to double
  %83 = call i32 (ptr, ...) @printf(ptr @.str.101, double %float.val559)
  br label %sout.merge556

sout.bool552:                                     ; preds = %sout.merge515
  %bool.val560 = ptrtoint ptr %payload549 to i64
  %is.true561 = icmp ne i64 %bool.val560, 0
  br i1 %is.true561, label %sout.true562, label %sout.false563

sout.str553:                                      ; preds = %sout.merge515
  %84 = call i32 (ptr, ...) @printf(ptr @.str.106, ptr %payload549)
  br label %sout.merge556

sout.null554:                                     ; preds = %sout.merge515
  %85 = call i32 (ptr, ...) @printf(ptr @.str.107, ptr @.str.108)
  br label %sout.merge556

sout.default555:                                  ; preds = %sout.merge515
  %86 = call i32 (ptr, ...) @printf(ptr @.str.109, ptr %payload549)
  br label %sout.merge556

sout.merge556:                                    ; preds = %sout.default555, %sout.null554, %sout.str553, %sout.bool.merge564, %sout.float551, %sout.int550
  %87 = call i32 @putchar(i32 10)
  %rcc.val565 = alloca %RCCValue, align 8
  %tag.ptr566 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val565, i32 0, i32 0
  store i64 0, ptr %tag.ptr566, align 4
  %payload.ptr567 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val565, i32 0, i32 1
  store ptr null, ptr %payload.ptr567, align 8
  %rcc.val568 = alloca %RCCValue, align 8
  %tag.ptr569 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val568, i32 0, i32 0
  store i64 3, ptr %tag.ptr569, align 4
  %payload.ptr570 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val568, i32 0, i32 1
  store ptr null, ptr %payload.ptr570, align 8
  %unary.int571 = ptrtoint ptr %rcc.val568 to i64
  %tag.ptr572 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val568, i32 0, i32 0
  %tag573 = load i64, ptr %tag.ptr572, align 4
  %payload.ptr574 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val568, i32 0, i32 1
  %payload575 = load ptr, ptr %payload.ptr574, align 8
  %is_null576 = icmp eq i64 %tag573, 0
  %is_int577 = icmp eq i64 %tag573, 1
  %is_bool578 = icmp eq i64 %tag573, 3
  %is_bool_or_int579 = or i1 %is_bool578, %is_int577
  %payload.int580 = ptrtoint ptr %payload575 to i64
  %payload_nz581 = icmp ne i64 %payload.int580, 0
  %not_null582 = xor i1 %is_null576, true
  %bool_int_result583 = select i1 %is_bool_or_int579, i1 %payload_nz581, i1 true
  %coerced_bool584 = select i1 %is_null576, i1 false, i1 %bool_int_result583
  %not585 = xor i1 %coerced_bool584, true
  %not.ext586 = zext i1 %not585 to i64
  %not.is_true587 = icmp ne i64 %not.ext586, 0
  %not.payload588 = select i1 %not.is_true587, ptr inttoptr (i64 1 to ptr), ptr null
  %rcc.val589 = alloca %RCCValue, align 8
  %tag.ptr590 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val589, i32 0, i32 0
  store i64 3, ptr %tag.ptr590, align 4
  %payload.ptr591 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val589, i32 0, i32 1
  store ptr %not.payload588, ptr %payload.ptr591, align 8
  %tag.ptr592 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val589, i32 0, i32 0
  %tag593 = load i64, ptr %tag.ptr592, align 4
  %payload.ptr594 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val589, i32 0, i32 1
  %payload595 = load ptr, ptr %payload.ptr594, align 8
  switch i64 %tag593, label %sout.default601 [
    i64 1, label %sout.int596
    i64 2, label %sout.float597
    i64 3, label %sout.bool598
    i64 4, label %sout.str599
    i64 0, label %sout.null600
  ]

sout.true562:                                     ; preds = %sout.bool552
  %88 = call i32 (ptr, ...) @printf(ptr @.str.102, ptr @.str.103)
  br label %sout.bool.merge564

sout.false563:                                    ; preds = %sout.bool552
  %89 = call i32 (ptr, ...) @printf(ptr @.str.104, ptr @.str.105)
  br label %sout.bool.merge564

sout.bool.merge564:                               ; preds = %sout.false563, %sout.true562
  br label %sout.merge556

sout.int596:                                      ; preds = %sout.merge556
  %int.val603 = ptrtoint ptr %payload595 to i64
  %90 = call i32 (ptr, ...) @printf(ptr @.str.110, i64 %int.val603)
  br label %sout.merge602

sout.float597:                                    ; preds = %sout.merge556
  %float.bits604 = ptrtoint ptr %payload595 to i64
  %float.val605 = bitcast i64 %float.bits604 to double
  %91 = call i32 (ptr, ...) @printf(ptr @.str.111, double %float.val605)
  br label %sout.merge602

sout.bool598:                                     ; preds = %sout.merge556
  %bool.val606 = ptrtoint ptr %payload595 to i64
  %is.true607 = icmp ne i64 %bool.val606, 0
  br i1 %is.true607, label %sout.true608, label %sout.false609

sout.str599:                                      ; preds = %sout.merge556
  %92 = call i32 (ptr, ...) @printf(ptr @.str.116, ptr %payload595)
  br label %sout.merge602

sout.null600:                                     ; preds = %sout.merge556
  %93 = call i32 (ptr, ...) @printf(ptr @.str.117, ptr @.str.118)
  br label %sout.merge602

sout.default601:                                  ; preds = %sout.merge556
  %94 = call i32 (ptr, ...) @printf(ptr @.str.119, ptr %payload595)
  br label %sout.merge602

sout.merge602:                                    ; preds = %sout.default601, %sout.null600, %sout.str599, %sout.bool.merge610, %sout.float597, %sout.int596
  %95 = call i32 @putchar(i32 10)
  %rcc.val611 = alloca %RCCValue, align 8
  %tag.ptr612 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val611, i32 0, i32 0
  store i64 0, ptr %tag.ptr612, align 4
  %payload.ptr613 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val611, i32 0, i32 1
  store ptr null, ptr %payload.ptr613, align 8
  ret i32 0

sout.true608:                                     ; preds = %sout.bool598
  %96 = call i32 (ptr, ...) @printf(ptr @.str.112, ptr @.str.113)
  br label %sout.bool.merge610

sout.false609:                                    ; preds = %sout.bool598
  %97 = call i32 (ptr, ...) @printf(ptr @.str.114, ptr @.str.115)
  br label %sout.bool.merge610

sout.bool.merge610:                               ; preds = %sout.false609, %sout.true608
  br label %sout.merge602
}

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
