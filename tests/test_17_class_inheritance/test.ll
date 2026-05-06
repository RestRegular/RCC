; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }
%Base = type { ptr }
%Derived = type { ptr, ptr }

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

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %call.user_main = call ptr @__rio_main()
  ret i32 0
}

define private ptr @__rio_main() {
entry:
  %d = alloca ptr, align 8
  store ptr null, ptr %d, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr inttoptr (i64 42 to ptr), ptr %payload.ptr, align 8
  %rcc.val.heap1 = call ptr @malloc(i64 16)
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 1
  store ptr inttoptr (i64 99 to ptr), ptr %payload.ptr3, align 8
  %new.Derived = call ptr @malloc(i64 16)
  %rcc.val.heap4 = call ptr @malloc(i64 16)
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 0
  store i64 0, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap4, i32 0, i32 1
  store ptr %new.Derived, ptr %payload.ptr6, align 8
  store ptr %rcc.val.heap4, ptr %d, align 8
  %d.load = load ptr, ptr %d, align 8
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %d.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr7, align 8
  %val.ptr = getelementptr inbounds nuw %Base, ptr %payload, i32 0, i32 0
  %val.val = load ptr, ptr %val.ptr, align 8
  %tag.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %val.val, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr8, align 4
  %payload.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %val.val, i32 0, i32 1
  %payload10 = load ptr, ptr %payload.ptr9, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %int.val = ptrtoint ptr %payload10 to i64
  %0 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload10 to i64
  %float.val = bitcast i64 %float.bits to double
  %1 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload10 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload10)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %3 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload10)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %5 = call i32 @putchar(i32 10)
  %rcc.val.heap11 = call ptr @malloc(i64 16)
  %tag.ptr12 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap11, i32 0, i32 0
  store i64 0, ptr %tag.ptr12, align 4
  %payload.ptr13 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap11, i32 0, i32 1
  store ptr null, ptr %payload.ptr13, align 8
  %d.load14 = load ptr, ptr %d, align 8
  %payload.ptr15 = getelementptr inbounds nuw %RCCValue, ptr %d.load14, i32 0, i32 1
  %payload16 = load ptr, ptr %payload.ptr15, align 8
  %extra.ptr = getelementptr inbounds nuw %Derived, ptr %payload16, i32 0, i32 1
  %extra.val = load ptr, ptr %extra.ptr, align 8
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %extra.val, i32 0, i32 0
  %tag18 = load i64, ptr %tag.ptr17, align 4
  %payload.ptr19 = getelementptr inbounds nuw %RCCValue, ptr %extra.val, i32 0, i32 1
  %payload20 = load ptr, ptr %payload.ptr19, align 8
  switch i64 %tag18, label %sout.default26 [
    i64 1, label %sout.int21
    i64 2, label %sout.float22
    i64 3, label %sout.bool23
    i64 4, label %sout.str24
    i64 0, label %sout.null25
  ]

sout.true:                                        ; preds = %sout.bool
  %6 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %7 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge

sout.int21:                                       ; preds = %sout.merge
  %int.val28 = ptrtoint ptr %payload20 to i64
  %8 = call i32 (ptr, ...) @printf(ptr @.str.10, i64 %int.val28)
  br label %sout.merge27

sout.float22:                                     ; preds = %sout.merge
  %float.bits29 = ptrtoint ptr %payload20 to i64
  %float.val30 = bitcast i64 %float.bits29 to double
  %9 = call i32 (ptr, ...) @printf(ptr @.str.11, double %float.val30)
  br label %sout.merge27

sout.bool23:                                      ; preds = %sout.merge
  %bool.val31 = ptrtoint ptr %payload20 to i64
  %is.true32 = icmp ne i64 %bool.val31, 0
  br i1 %is.true32, label %sout.true33, label %sout.false34

sout.str24:                                       ; preds = %sout.merge
  %10 = call i32 (ptr, ...) @printf(ptr @.str.16, ptr %payload20)
  br label %sout.merge27

sout.null25:                                      ; preds = %sout.merge
  %11 = call i32 (ptr, ...) @printf(ptr @.str.17, ptr @.str.18)
  br label %sout.merge27

sout.default26:                                   ; preds = %sout.merge
  %12 = call i32 (ptr, ...) @printf(ptr @.str.19, ptr %payload20)
  br label %sout.merge27

sout.merge27:                                     ; preds = %sout.default26, %sout.null25, %sout.str24, %sout.bool.merge35, %sout.float22, %sout.int21
  %13 = call i32 @putchar(i32 10)
  %rcc.val.heap36 = call ptr @malloc(i64 16)
  %tag.ptr37 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap36, i32 0, i32 0
  store i64 0, ptr %tag.ptr37, align 4
  %payload.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap36, i32 0, i32 1
  store ptr null, ptr %payload.ptr38, align 8
  ret ptr null

sout.true33:                                      ; preds = %sout.bool23
  %14 = call i32 (ptr, ...) @printf(ptr @.str.12, ptr @.str.13)
  br label %sout.bool.merge35

sout.false34:                                     ; preds = %sout.bool23
  %15 = call i32 (ptr, ...) @printf(ptr @.str.14, ptr @.str.15)
  br label %sout.bool.merge35

sout.bool.merge35:                                ; preds = %sout.false34, %sout.true33
  br label %sout.merge27
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
