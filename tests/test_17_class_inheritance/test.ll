; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [6 x i8] c"Buddy\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %call.user_main = call ptr @main.1()
  ret i32 0
}

define private ptr @main.1() {
entry:
  %n = alloca ptr, align 8
  %s = alloca ptr, align 8
  %arith.result23 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
  %d = alloca ptr, align 8
  store ptr null, ptr %d, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 4, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr, align 8
  %call.Dog = call ptr @Dog(ptr %rcc.val.heap)
  store ptr %call.Dog, ptr %d, align 8
  %d.load = load ptr, ptr %d, align 8
  %tag.ptr1 = getelementptr inbounds nuw %RCCValue, ptr %d.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr1, align 4
  %tag2 = load i64, ptr null, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag2, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %d.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr3, align 8
  %payload4 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload4 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val.heap5 = call ptr @malloc(i64 16)
  %tag.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap5, i32 0, i32 0
  store i64 2, ptr %tag.ptr6, align 4
  %payload.ptr7 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap5, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr7, align 8
  store ptr %rcc.val.heap5, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr8 = getelementptr inbounds nuw %RCCValue, ptr %d.load, i32 0, i32 1
  %payload9 = load ptr, ptr %payload.ptr8, align 8
  %payload10 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.int = ptrtoint ptr %payload9 to i64
  %right.int = ptrtoint ptr %payload10 to i64
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %d.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %call.sout = call ptr @sout(ptr null)
  %d.load13 = load ptr, ptr %d, align 8
  %tag.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %d.load13, i32 0, i32 0
  %tag15 = load i64, ptr %tag.ptr14, align 4
  %tag16 = load i64, ptr null, align 4
  %is_float.left17 = icmp eq i64 %tag15, 2
  %is_float.right18 = icmp eq i64 %tag16, 2
  %is_float19 = or i1 %is_float.left17, %is_float.right18
  store ptr null, ptr %arith.result23, align 8
  br i1 %is_float19, label %arith.float20, label %arith.int21

arith.merge:                                      ; preds = %arith.float

arith.float20:                                    ; preds = %arith.int
  %payload.ptr24 = getelementptr inbounds nuw %RCCValue, ptr %d.load13, i32 0, i32 1
  %payload25 = load ptr, ptr %payload.ptr24, align 8
  %payload26 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.bits27 = ptrtoint ptr %payload25 to i64
  %right.bits28 = ptrtoint ptr %payload26 to i64
  %left.sitofp29 = sitofp i64 %left.bits27 to double
  %left.bitcast30 = bitcast i64 %left.bits27 to double
  %left.double31 = select i1 %is_float.left17, double %left.bitcast30, double %left.sitofp29
  %right.sitofp32 = sitofp i64 %right.bits28 to double
  %right.bitcast33 = bitcast i64 %right.bits28 to double
  %right.double34 = select i1 %is_float.right18, double %right.bitcast33, double %right.sitofp32
  %result.bits35 = bitcast double %left.double31 to i64
  %result.float.ptr36 = inttoptr i64 %result.bits35 to ptr
  %rcc.val.heap37 = call ptr @malloc(i64 16)
  %tag.ptr38 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap37, i32 0, i32 0
  store i64 2, ptr %tag.ptr38, align 4
  %payload.ptr39 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap37, i32 0, i32 1
  store ptr %result.float.ptr36, ptr %payload.ptr39, align 8
  store ptr %rcc.val.heap37, ptr %arith.result23, align 8
  br label %arith.merge22

arith.int21:                                      ; preds = %arith.int
  %payload.ptr40 = getelementptr inbounds nuw %RCCValue, ptr %d.load13, i32 0, i32 1
  %payload41 = load ptr, ptr %payload.ptr40, align 8
  %payload42 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.int43 = ptrtoint ptr %payload41 to i64
  %right.int44 = ptrtoint ptr %payload42 to i64
  %payload.ptr45 = getelementptr inbounds nuw %RCCValue, ptr %d.load13, i32 0, i32 1
  %payload46 = load ptr, ptr %payload.ptr45, align 8
  %call.sout47 = call ptr @sout(ptr null)
  %call. = call ptr @0()
  store ptr null, ptr %s, align 8
  %call.48 = call ptr @1()
  store ptr %call.48, ptr %s, align 8
  store ptr null, ptr %n, align 8
  %call.49 = call ptr @2()
  store ptr %call.49, ptr %n, align 8
  %s.load = load ptr, ptr %s, align 8
  %call.sout50 = call ptr @sout(ptr %s.load)
  %n.load = load ptr, ptr %n, align 8
  %call.sout51 = call ptr @sout(ptr %n.load)
  ret ptr null

arith.merge22:                                    ; preds = %arith.float20
}

declare ptr @malloc(i64)

declare ptr @Dog(ptr)

declare ptr @sout(ptr)

declare ptr @0()

declare ptr @1()

declare ptr @2()
