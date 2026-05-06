; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%Point = type { ptr, ptr, ptr }
%RCCValue = type { i64, ptr }

@.str = private constant [11 x i8] c"test point\00"
@.str.1 = private constant [7 x i8] c"p.x = \00"
@.str.2 = private constant [5 x i8] c"%lld\00"
@.str.3 = private constant [3 x i8] c"%g\00"
@.str.4 = private constant [3 x i8] c"%s\00"
@.str.5 = private constant [5 x i8] c"true\00"
@.str.6 = private constant [3 x i8] c"%s\00"
@.str.7 = private constant [6 x i8] c"false\00"
@.str.8 = private constant [3 x i8] c"%s\00"
@.str.9 = private constant [3 x i8] c"%s\00"
@.str.10 = private constant [5 x i8] c"null\00"
@.str.11 = private constant [10 x i8] c"<ptr: %p>\00"
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
@.str.22 = private constant [7 x i8] c"p.y = \00"
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
@.str.33 = private constant [5 x i8] c"%lld\00"
@.str.34 = private constant [3 x i8] c"%g\00"
@.str.35 = private constant [3 x i8] c"%s\00"
@.str.36 = private constant [5 x i8] c"true\00"
@.str.37 = private constant [3 x i8] c"%s\00"
@.str.38 = private constant [6 x i8] c"false\00"
@.str.39 = private constant [3 x i8] c"%s\00"
@.str.40 = private constant [3 x i8] c"%s\00"
@.str.41 = private constant [5 x i8] c"null\00"
@.str.42 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.43 = private constant [7 x i8] c"p.z = \00"
@.str.44 = private constant [5 x i8] c"%lld\00"
@.str.45 = private constant [3 x i8] c"%g\00"
@.str.46 = private constant [3 x i8] c"%s\00"
@.str.47 = private constant [5 x i8] c"true\00"
@.str.48 = private constant [3 x i8] c"%s\00"
@.str.49 = private constant [6 x i8] c"false\00"
@.str.50 = private constant [3 x i8] c"%s\00"
@.str.51 = private constant [3 x i8] c"%s\00"
@.str.52 = private constant [5 x i8] c"null\00"
@.str.53 = private constant [10 x i8] c"<ptr: %p>\00"
@.str.54 = private constant [5 x i8] c"%lld\00"
@.str.55 = private constant [3 x i8] c"%g\00"
@.str.56 = private constant [3 x i8] c"%s\00"
@.str.57 = private constant [5 x i8] c"true\00"
@.str.58 = private constant [3 x i8] c"%s\00"
@.str.59 = private constant [6 x i8] c"false\00"
@.str.60 = private constant [3 x i8] c"%s\00"
@.str.61 = private constant [3 x i8] c"%s\00"
@.str.62 = private constant [5 x i8] c"null\00"
@.str.63 = private constant [10 x i8] c"<ptr: %p>\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %call.main = call i32 @main()
  ret i32 0
}

define ptr @Point.__init__(ptr %this, ptr %a, ptr %b, ptr %z) {
entry:
  %z4 = alloca ptr, align 8
  %b3 = alloca ptr, align 8
  %a2 = alloca ptr, align 8
  %this1 = alloca ptr, align 8
  store ptr %this, ptr %this1, align 8
  store ptr %a, ptr %a2, align 8
  store ptr %b, ptr %b3, align 8
  store ptr %z, ptr %z4, align 8
  %a.load = load ptr, ptr %a2, align 8
  %this.load = load ptr, ptr %this1, align 8
  %x.ptr = getelementptr inbounds nuw %Point, ptr %this.load, i32 0, i32 0
  store ptr %a.load, ptr %x.ptr, align 8
  %b.load = load ptr, ptr %b3, align 8
  %this.load5 = load ptr, ptr %this1, align 8
  %y.ptr = getelementptr inbounds nuw %Point, ptr %this.load5, i32 0, i32 1
  store ptr %b.load, ptr %y.ptr, align 8
  %z.load = load ptr, ptr %z4, align 8
  %this.load6 = load ptr, ptr %this1, align 8
  %z.ptr = getelementptr inbounds nuw %Point, ptr %this.load6, i32 0, i32 2
  store ptr %z.load, ptr %z.ptr, align 8
  %this.load7 = load ptr, ptr %this1, align 8
  ret ptr %this.load7
}

define private ptr @__rio_main() {
entry:
  %p = alloca ptr, align 8
  store ptr null, ptr %p, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr inttoptr (i64 10 to ptr), ptr %payload.ptr, align 8
  %rcc.val.heap1 = call ptr @malloc(i64 16)
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 1
  store ptr inttoptr (i64 20 to ptr), ptr %payload.ptr3, align 8
  %rcc.val.heap4 = call ptr @malloc(i64 16)
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 0
  store i64 4, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr6, align 8
  %new.Point = call ptr @malloc(i64 24)
  %rcc.val.heap7 = call ptr @malloc(i64 16)
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap7, i32 0, i32 0
  store i64 0, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap7, i32 0, i32 1
  store ptr %new.Point, ptr %payload.ptr9, align 8
  %ctor.Point = call ptr @Point.__init__(ptr %new.Point, ptr %rcc.val.heap, ptr %rcc.val.heap1, ptr %rcc.val.heap4)
  store ptr %rcc.val.heap7, ptr %p, align 8
  %rcc.val.heap10 = call ptr @malloc(i64 16)
  %tag.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap10, i32 0, i32 0
  store i64 4, ptr %tag.ptr11, align 4
  %payload.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap10, i32 0, i32 1
  store ptr @.str.1, ptr %payload.ptr12, align 8
  %p.load = load ptr, ptr %p, align 8
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %p.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr13, align 8
  %x.ptr = getelementptr inbounds nuw %Point, ptr %payload, i32 0, i32 0
  %x.val = load ptr, ptr %x.ptr, align 8
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap10, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr14, align 4
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap10, i32 0, i32 1
  %payload16 = load ptr, ptr %payload.ptr15, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %int.val = ptrtoint ptr %payload16 to i64
  %0 = call i32 (ptr, ...) @printf(ptr @.str.2, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload16 to i64
  %float.val = bitcast i64 %float.bits to double
  %1 = call i32 (ptr, ...) @printf(ptr @.str.3, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload16 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr %payload16)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %3 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr @.str.10)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.11, ptr %payload16)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %x.val, i32 0, i32 0
  %tag18 = load i64, ptr %tag.ptr17, align 4
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %x.val, i32 0, i32 1
  %payload20 = load ptr, ptr %payload.ptr19, align 8
  switch i64 %tag18, label %sout.default26 [
    i64 1, label %sout.int21
    i64 2, label %sout.float22
    i64 3, label %sout.bool23
    i64 4, label %sout.str24
    i64 0, label %sout.null25
  ]

sout.true:                                        ; preds = %sout.bool
  %5 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %6 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr @.str.7)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int21:                                       ; preds = %sout.merge
  %int.val28 = ptrtoint ptr %payload20 to i64
  %7 = call i32 (ptr, ...) @printf(ptr @.str.12, i64 %int.val28)
  br label %sout.merge27

sout.float22:                                     ; preds = %sout.merge
  %float.bits29 = ptrtoint ptr %payload20 to i64
  %float.val30 = bitcast i64 %float.bits29 to double
  %8 = call i32 (ptr, ...) @printf(ptr @.str.13, double %float.val30)
  br label %sout.merge27

sout.bool23:                                      ; preds = %sout.merge
  %bool.val31 = ptrtoint ptr %payload20 to i64
  %is.true32 = icmp ne i64 %bool.val31, 0
  br i1 %is.true32, label %sout.true33, label %sout.false34

sout.str24:                                       ; preds = %sout.merge
  %9 = call i32 (ptr, ...) @printf(ptr @.str.18, ptr %payload20)
  br label %sout.merge27

sout.null25:                                      ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr @.str.20)
  br label %sout.merge27

sout.default26:                                   ; preds = %sout.merge
  %11 = call i32 (ptr, ...) @printf(ptr @.str.21, ptr %payload20)
  br label %sout.merge27

sout.merge27:                                     ; preds = %sout.default26, %sout.null25, %sout.str24, %sout.bool.merge35, %sout.float22, %sout.int21
  %12 = call i32 @putchar(i32 10)
  %rcc.val.heap36 = call ptr @malloc(i64 16)
  %tag.ptr37 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap36, i32 0, i32 0
  store i64 0, ptr %tag.ptr37, align 4
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap36, i32 0, i32 1
  store ptr null, ptr %payload.ptr38, align 8
  %rcc.val.heap39 = call ptr @malloc(i64 16)
  %tag.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap39, i32 0, i32 0
  store i64 4, ptr %tag.ptr40, align 4
  %payload.ptr41 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap39, i32 0, i32 1
  store ptr @.str.22, ptr %payload.ptr41, align 8
  %p.load42 = load ptr, ptr %p, align 8
  %payload.ptr43 = getelementptr inbounds nuw %RCCValue, ptr %p.load42, i32 0, i32 1
  %payload44 = load ptr, ptr %payload.ptr43, align 8
  %y.ptr = getelementptr inbounds nuw %Point, ptr %payload44, i32 0, i32 1
  %y.val = load ptr, ptr %y.ptr, align 8
  %tag.ptr45 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap39, i32 0, i32 0
  %tag46 = load i64, ptr %tag.ptr45, align 4
  %payload.ptr47 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap39, i32 0, i32 1
  %payload48 = load ptr, ptr %payload.ptr47, align 8
  switch i64 %tag46, label %sout.default54 [
    i64 1, label %sout.int49
    i64 2, label %sout.float50
    i64 3, label %sout.bool51
    i64 4, label %sout.str52
    i64 0, label %sout.null53
  ]

sout.true33:                                      ; preds = %sout.bool23
  %13 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge35

sout.false34:                                     ; preds = %sout.bool23
  %14 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr @.str.17)
  br label %sout.bool.merge35

sout.bool.merge35:                                ; preds = %sout.false34, %sout.true33
  br label %sout.merge27

sout.int49:                                       ; preds = %sout.merge27
  %int.val56 = ptrtoint ptr %payload48 to i64
  %15 = call i32 (ptr, ...) @printf(ptr @.str.23, i64 %int.val56)
  br label %sout.merge55

sout.float50:                                     ; preds = %sout.merge27
  %float.bits57 = ptrtoint ptr %payload48 to i64
  %float.val58 = bitcast i64 %float.bits57 to double
  %16 = call i32 (ptr, ...) @printf(ptr @.str.24, double %float.val58)
  br label %sout.merge55

sout.bool51:                                      ; preds = %sout.merge27
  %bool.val59 = ptrtoint ptr %payload48 to i64
  %is.true60 = icmp ne i64 %bool.val59, 0
  br i1 %is.true60, label %sout.true61, label %sout.false62

sout.str52:                                       ; preds = %sout.merge27
  %17 = call i32 (ptr, ...) @printf(ptr @.str.29, ptr %payload48)
  br label %sout.merge55

sout.null53:                                      ; preds = %sout.merge27
  %18 = call i32 (ptr, ...) @printf(ptr @.str.30, ptr @.str.31)
  br label %sout.merge55

sout.default54:                                   ; preds = %sout.merge27
  %19 = call i32 (ptr, ...) @printf(ptr @.str.32, ptr %payload48)
  br label %sout.merge55

sout.merge55:                                     ; preds = %sout.default54, %sout.null53, %sout.str52, %sout.bool.merge63, %sout.float50, %sout.int49
  %tag.ptr64 = getelementptr inbounds nuw %RCCValue, ptr %y.val, i32 0, i32 0
  %tag65 = load i64, ptr %tag.ptr64, align 4
  %payload.ptr66 = getelementptr inbounds nuw %RCCValue, ptr %y.val, i32 0, i32 1
  %payload67 = load ptr, ptr %payload.ptr66, align 8
  switch i64 %tag65, label %sout.default73 [
    i64 1, label %sout.int68
    i64 2, label %sout.float69
    i64 3, label %sout.bool70
    i64 4, label %sout.str71
    i64 0, label %sout.null72
  ]

sout.true61:                                      ; preds = %sout.bool51
  %20 = call i32 (ptr, ...) @printf(ptr @.str.25, ptr @.str.26)
  br label %sout.bool.merge63

sout.false62:                                     ; preds = %sout.bool51
  %21 = call i32 (ptr, ...) @printf(ptr @.str.27, ptr @.str.28)
  br label %sout.bool.merge63

sout.bool.merge63:                                ; preds = %sout.false62, %sout.true61
  br label %sout.merge55

sout.int68:                                       ; preds = %sout.merge55
  %int.val75 = ptrtoint ptr %payload67 to i64
  %22 = call i32 (ptr, ...) @printf(ptr @.str.33, i64 %int.val75)
  br label %sout.merge74

sout.float69:                                     ; preds = %sout.merge55
  %float.bits76 = ptrtoint ptr %payload67 to i64
  %float.val77 = bitcast i64 %float.bits76 to double
  %23 = call i32 (ptr, ...) @printf(ptr @.str.34, double %float.val77)
  br label %sout.merge74

sout.bool70:                                      ; preds = %sout.merge55
  %bool.val78 = ptrtoint ptr %payload67 to i64
  %is.true79 = icmp ne i64 %bool.val78, 0
  br i1 %is.true79, label %sout.true80, label %sout.false81

sout.str71:                                       ; preds = %sout.merge55
  %24 = call i32 (ptr, ...) @printf(ptr @.str.39, ptr %payload67)
  br label %sout.merge74

sout.null72:                                      ; preds = %sout.merge55
  %25 = call i32 (ptr, ...) @printf(ptr @.str.40, ptr @.str.41)
  br label %sout.merge74

sout.default73:                                   ; preds = %sout.merge55
  %26 = call i32 (ptr, ...) @printf(ptr @.str.42, ptr %payload67)
  br label %sout.merge74

sout.merge74:                                     ; preds = %sout.default73, %sout.null72, %sout.str71, %sout.bool.merge82, %sout.float69, %sout.int68
  %27 = call i32 @putchar(i32 10)
  %rcc.val.heap83 = call ptr @malloc(i64 16)
  %tag.ptr84 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap83, i32 0, i32 0
  store i64 0, ptr %tag.ptr84, align 4
  %payload.ptr85 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap83, i32 0, i32 1
  store ptr null, ptr %payload.ptr85, align 8
  %rcc.val.heap86 = call ptr @malloc(i64 16)
  %tag.ptr87 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap86, i32 0, i32 0
  store i64 4, ptr %tag.ptr87, align 4
  %payload.ptr88 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap86, i32 0, i32 1
  store ptr @.str.43, ptr %payload.ptr88, align 8
  %p.load89 = load ptr, ptr %p, align 8
  %payload.ptr90 = getelementptr inbounds nuw %RCCValue, ptr %p.load89, i32 0, i32 1
  %payload91 = load ptr, ptr %payload.ptr90, align 8
  %z.ptr = getelementptr inbounds nuw %Point, ptr %payload91, i32 0, i32 2
  %z.val = load ptr, ptr %z.ptr, align 8
  %tag.ptr92 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap86, i32 0, i32 0
  %tag93 = load i64, ptr %tag.ptr92, align 4
  %payload.ptr94 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap86, i32 0, i32 1
  %payload95 = load ptr, ptr %payload.ptr94, align 8
  switch i64 %tag93, label %sout.default101 [
    i64 1, label %sout.int96
    i64 2, label %sout.float97
    i64 3, label %sout.bool98
    i64 4, label %sout.str99
    i64 0, label %sout.null100
  ]

sout.true80:                                      ; preds = %sout.bool70
  %28 = call i32 (ptr, ...) @printf(ptr @.str.35, ptr @.str.36)
  br label %sout.bool.merge82

sout.false81:                                     ; preds = %sout.bool70
  %29 = call i32 (ptr, ...) @printf(ptr @.str.37, ptr @.str.38)
  br label %sout.bool.merge82

sout.bool.merge82:                                ; preds = %sout.false81, %sout.true80
  br label %sout.merge74

sout.int96:                                       ; preds = %sout.merge74
  %int.val103 = ptrtoint ptr %payload95 to i64
  %30 = call i32 (ptr, ...) @printf(ptr @.str.44, i64 %int.val103)
  br label %sout.merge102

sout.float97:                                     ; preds = %sout.merge74
  %float.bits104 = ptrtoint ptr %payload95 to i64
  %float.val105 = bitcast i64 %float.bits104 to double
  %31 = call i32 (ptr, ...) @printf(ptr @.str.45, double %float.val105)
  br label %sout.merge102

sout.bool98:                                      ; preds = %sout.merge74
  %bool.val106 = ptrtoint ptr %payload95 to i64
  %is.true107 = icmp ne i64 %bool.val106, 0
  br i1 %is.true107, label %sout.true108, label %sout.false109

sout.str99:                                       ; preds = %sout.merge74
  %32 = call i32 (ptr, ...) @printf(ptr @.str.50, ptr %payload95)
  br label %sout.merge102

sout.null100:                                     ; preds = %sout.merge74
  %33 = call i32 (ptr, ...) @printf(ptr @.str.51, ptr @.str.52)
  br label %sout.merge102

sout.default101:                                  ; preds = %sout.merge74
  %34 = call i32 (ptr, ...) @printf(ptr @.str.53, ptr %payload95)
  br label %sout.merge102

sout.merge102:                                    ; preds = %sout.default101, %sout.null100, %sout.str99, %sout.bool.merge110, %sout.float97, %sout.int96
  %tag.ptr111 = getelementptr inbounds nuw %RCCValue, ptr %z.val, i32 0, i32 0
  %tag112 = load i64, ptr %tag.ptr111, align 4
  %payload.ptr113 = getelementptr inbounds nuw %RCCValue, ptr %z.val, i32 0, i32 1
  %payload114 = load ptr, ptr %payload.ptr113, align 8
  switch i64 %tag112, label %sout.default120 [
    i64 1, label %sout.int115
    i64 2, label %sout.float116
    i64 3, label %sout.bool117
    i64 4, label %sout.str118
    i64 0, label %sout.null119
  ]

sout.true108:                                     ; preds = %sout.bool98
  %35 = call i32 (ptr, ...) @printf(ptr @.str.46, ptr @.str.47)
  br label %sout.bool.merge110

sout.false109:                                    ; preds = %sout.bool98
  %36 = call i32 (ptr, ...) @printf(ptr @.str.48, ptr @.str.49)
  br label %sout.bool.merge110

sout.bool.merge110:                               ; preds = %sout.false109, %sout.true108
  br label %sout.merge102

sout.int115:                                      ; preds = %sout.merge102
  %int.val122 = ptrtoint ptr %payload114 to i64
  %37 = call i32 (ptr, ...) @printf(ptr @.str.54, i64 %int.val122)
  br label %sout.merge121

sout.float116:                                    ; preds = %sout.merge102
  %float.bits123 = ptrtoint ptr %payload114 to i64
  %float.val124 = bitcast i64 %float.bits123 to double
  %38 = call i32 (ptr, ...) @printf(ptr @.str.55, double %float.val124)
  br label %sout.merge121

sout.bool117:                                     ; preds = %sout.merge102
  %bool.val125 = ptrtoint ptr %payload114 to i64
  %is.true126 = icmp ne i64 %bool.val125, 0
  br i1 %is.true126, label %sout.true127, label %sout.false128

sout.str118:                                      ; preds = %sout.merge102
  %39 = call i32 (ptr, ...) @printf(ptr @.str.60, ptr %payload114)
  br label %sout.merge121

sout.null119:                                     ; preds = %sout.merge102
  %40 = call i32 (ptr, ...) @printf(ptr @.str.61, ptr @.str.62)
  br label %sout.merge121

sout.default120:                                  ; preds = %sout.merge102
  %41 = call i32 (ptr, ...) @printf(ptr @.str.63, ptr %payload114)
  br label %sout.merge121

sout.merge121:                                    ; preds = %sout.default120, %sout.null119, %sout.str118, %sout.bool.merge129, %sout.float116, %sout.int115
  %42 = call i32 @putchar(i32 10)
  %rcc.val.heap130 = call ptr @malloc(i64 16)
  %tag.ptr131 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap130, i32 0, i32 0
  store i64 0, ptr %tag.ptr131, align 4
  %payload.ptr132 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap130, i32 0, i32 1
  store ptr null, ptr %payload.ptr132, align 8
  ret ptr null

sout.true127:                                     ; preds = %sout.bool117
  %43 = call i32 (ptr, ...) @printf(ptr @.str.56, ptr @.str.57)
  br label %sout.bool.merge129

sout.false128:                                    ; preds = %sout.bool117
  %44 = call i32 (ptr, ...) @printf(ptr @.str.58, ptr @.str.59)
  br label %sout.bool.merge129

sout.bool.merge129:                               ; preds = %sout.false128, %sout.true127
  br label %sout.merge121
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
