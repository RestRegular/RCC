; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }
%Object = type { ptr }

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

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %o = alloca ptr, align 8
  store ptr null, ptr %o, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 1, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr inttoptr (i64 42 to ptr), ptr %payload.ptr, align 8
  %new.Object = call ptr @malloc(i64 8)
  %rcc.val.heap1 = call ptr @malloc(i64 16)
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 0
  store i64 0, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 1
  store ptr %new.Object, ptr %payload.ptr3, align 8
  %ctor.Object = call ptr @Object.__init__(ptr %new.Object, ptr %rcc.val.heap)
  store ptr %rcc.val.heap1, ptr %o, align 8
  %o.load = load ptr, ptr %o, align 8
  %payload.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %o.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr4, align 8
  %x.ptr = getelementptr inbounds nuw %Object, ptr %payload, i32 0, i32 0
  %x.val = load ptr, ptr %x.ptr, align 8
  %tag.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %x.val, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr5, align 4
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %x.val, i32 0, i32 1
  %payload7 = load ptr, ptr %payload.ptr6, align 8
  switch i64 %tag, label %sout.default [
    i64 1, label %sout.int
    i64 2, label %sout.float
    i64 3, label %sout.bool
    i64 4, label %sout.str
    i64 0, label %sout.null
  ]

sout.int:                                         ; preds = %entry
  %int.val = ptrtoint ptr %payload7 to i64
  %2 = call i32 (ptr, ...) @printf(ptr @.str, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload7 to i64
  %float.val = bitcast i64 %float.bits to double
  %3 = call i32 (ptr, ...) @printf(ptr @.str.1, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload7 to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr %payload7)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %5 = call i32 (ptr, ...) @printf(ptr @.str.7, ptr @.str.8)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %6 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr %payload7)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %7 = call i32 @putchar(i32 10)
  %rcc.val.heap8 = call ptr @malloc(i64 16)
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 0
  store i64 0, ptr %tag.ptr9, align 4
  %payload.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 1
  store ptr null, ptr %payload.ptr10, align 8
  %call. = call ptr @0()
  ret i32 0

sout.true:                                        ; preds = %sout.bool
  %8 = call i32 (ptr, ...) @printf(ptr @.str.2, ptr @.str.3)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %9 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge
}

define ptr @Object.__init__(ptr %this, ptr %x) {
entry:
  %x2 = alloca ptr, align 8
  %this1 = alloca ptr, align 8
  store ptr %this, ptr %this1, align 8
  store ptr %x, ptr %x2, align 8
  %x.load = load ptr, ptr %x2, align 8
  %this.load = load ptr, ptr %this1, align 8
  %x.ptr = getelementptr inbounds nuw %Object, ptr %this.load, i32 0, i32 0
  store ptr %x.load, ptr %x.ptr, align 8
  %this.load3 = load ptr, ptr %this1, align 8
  ret ptr %this.load3
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)

declare ptr @0()
