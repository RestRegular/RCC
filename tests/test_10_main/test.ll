; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [17 x i8] c"main entry: pass\00"
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

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %call.user_main = call ptr @main.1()
  ret i32 0
}

define private ptr @main.1() {
entry:
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 4, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr, align 8
  %tag.ptr1 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr1, align 4
  %payload.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
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
  %0 = call i32 (ptr, ...) @printf(ptr @.str.2, i64 %int.val)
  br label %sout.merge

sout.float:                                       ; preds = %entry
  %float.bits = ptrtoint ptr %payload to i64
  %float.val = bitcast i64 %float.bits to double
  %1 = call i32 (ptr, ...) @printf(ptr @.str.3, double %float.val)
  br label %sout.merge

sout.bool:                                        ; preds = %entry
  %bool.val = ptrtoint ptr %payload to i64
  %is.true = icmp ne i64 %bool.val, 0
  br i1 %is.true, label %sout.true, label %sout.false

sout.str:                                         ; preds = %entry
  %2 = call i32 (ptr, ...) @printf(ptr @.str.8, ptr %payload)
  br label %sout.merge

sout.null:                                        ; preds = %entry
  %3 = call i32 (ptr, ...) @printf(ptr @.str.9, ptr @.str.10)
  br label %sout.merge

sout.default:                                     ; preds = %entry
  %4 = call i32 (ptr, ...) @printf(ptr @.str.11, ptr %payload)
  br label %sout.merge

sout.merge:                                       ; preds = %sout.default, %sout.null, %sout.str, %sout.bool.merge, %sout.float, %sout.int
  %5 = call i32 @putchar(i32 10)
  %rcc.val.heap3 = call ptr @malloc(i64 16)
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap3, i32 0, i32 0
  store i64 0, ptr %tag.ptr4, align 4
  %payload.ptr5 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap3, i32 0, i32 1
  store ptr null, ptr %payload.ptr5, align 8
  ret ptr null

sout.true:                                        ; preds = %sout.bool
  %6 = call i32 (ptr, ...) @printf(ptr @.str.4, ptr @.str.5)
  br label %sout.bool.merge

sout.false:                                       ; preds = %sout.bool
  %7 = call i32 (ptr, ...) @printf(ptr @.str.6, ptr @.str.7)
  br label %sout.bool.merge

sout.bool.merge:                                  ; preds = %sout.false, %sout.true
  br label %sout.merge
}

declare ptr @malloc(i64)

declare i32 @putchar(i32)

declare i32 @printf(ptr, ...)

declare i32 @strcmp(ptr, ptr)
