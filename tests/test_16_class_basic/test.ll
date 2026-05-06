; ModuleID = 'rcc_module'
source_filename = "rcc_module"

%RCCValue = type { i64, ptr }

@.str = private constant [6 x i8] c"Alice\00"

define dso_local i32 @main(i32 %0, ptr %1) {
entry:
  %call.user_main = call ptr @main.1()
  ret i32 0
}

define private ptr @main.1() {
entry:
  %age = alloca ptr, align 8
  %arith.result26 = alloca ptr, align 8
  %arith.result = alloca ptr, align 8
  %p = alloca ptr, align 8
  store ptr null, ptr %p, align 8
  %rcc.val.heap = call ptr @malloc(i64 16)
  %tag.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 0
  store i64 4, ptr %tag.ptr, align 4
  %payload.ptr = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap, i32 0, i32 1
  store ptr @.str, ptr %payload.ptr, align 8
  %rcc.val.heap1 = call ptr @malloc(i64 16)
  %tag.ptr2 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 0
  store i64 1, ptr %tag.ptr2, align 4
  %payload.ptr3 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap1, i32 0, i32 1
  store ptr inttoptr (i64 25 to ptr), ptr %payload.ptr3, align 8
  %call.Person = call ptr @Person(ptr %rcc.val.heap, ptr %rcc.val.heap1)
  store ptr %call.Person, ptr %p, align 8
  %p.load = load ptr, ptr %p, align 8
  %tag.ptr4 = getelementptr inbounds nuw %RCCValue, ptr %p.load, i32 0, i32 0
  %tag = load i64, ptr %tag.ptr4, align 4
  %tag5 = load i64, ptr null, align 4
  %is_float.left = icmp eq i64 %tag, 2
  %is_float.right = icmp eq i64 %tag5, 2
  %is_float = or i1 %is_float.left, %is_float.right
  store ptr null, ptr %arith.result, align 8
  br i1 %is_float, label %arith.float, label %arith.int

arith.float:                                      ; preds = %entry
  %payload.ptr6 = getelementptr inbounds nuw %RCCValue, ptr %p.load, i32 0, i32 1
  %payload = load ptr, ptr %payload.ptr6, align 8
  %payload7 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.bits = ptrtoint ptr %payload to i64
  %right.bits = ptrtoint ptr %payload7 to i64
  %left.sitofp = sitofp i64 %left.bits to double
  %left.bitcast = bitcast i64 %left.bits to double
  %left.double = select i1 %is_float.left, double %left.bitcast, double %left.sitofp
  %right.sitofp = sitofp i64 %right.bits to double
  %right.bitcast = bitcast i64 %right.bits to double
  %right.double = select i1 %is_float.right, double %right.bitcast, double %right.sitofp
  %result.bits = bitcast double %left.double to i64
  %result.float.ptr = inttoptr i64 %result.bits to ptr
  %rcc.val.heap8 = call ptr @malloc(i64 16)
  %tag.ptr9 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 0
  store i64 2, ptr %tag.ptr9, align 4
  %payload.ptr10 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap8, i32 0, i32 1
  store ptr %result.float.ptr, ptr %payload.ptr10, align 8
  store ptr %rcc.val.heap8, ptr %arith.result, align 8
  br label %arith.merge

arith.int:                                        ; preds = %entry
  %payload.ptr11 = getelementptr inbounds nuw %RCCValue, ptr %p.load, i32 0, i32 1
  %payload12 = load ptr, ptr %payload.ptr11, align 8
  %payload13 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.int = ptrtoint ptr %payload12 to i64
  %right.int = ptrtoint ptr %payload13 to i64
  %payload.ptr14 = getelementptr inbounds nuw %RCCValue, ptr %p.load, i32 0, i32 1
  %payload15 = load ptr, ptr %payload.ptr14, align 8
  %call.sout = call ptr @sout(ptr null)
  %p.load16 = load ptr, ptr %p, align 8
  %tag.ptr17 = getelementptr inbounds nuw %RCCValue, ptr %p.load16, i32 0, i32 0
  %tag18 = load i64, ptr %tag.ptr17, align 4
  %tag19 = load i64, ptr null, align 4
  %is_float.left20 = icmp eq i64 %tag18, 2
  %is_float.right21 = icmp eq i64 %tag19, 2
  %is_float22 = or i1 %is_float.left20, %is_float.right21
  store ptr null, ptr %arith.result26, align 8
  br i1 %is_float22, label %arith.float23, label %arith.int24

arith.merge:                                      ; preds = %arith.float

arith.float23:                                    ; preds = %arith.int
  %payload.ptr27 = getelementptr inbounds nuw %RCCValue, ptr %p.load16, i32 0, i32 1
  %payload28 = load ptr, ptr %payload.ptr27, align 8
  %payload29 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.bits30 = ptrtoint ptr %payload28 to i64
  %right.bits31 = ptrtoint ptr %payload29 to i64
  %left.sitofp32 = sitofp i64 %left.bits30 to double
  %left.bitcast33 = bitcast i64 %left.bits30 to double
  %left.double34 = select i1 %is_float.left20, double %left.bitcast33, double %left.sitofp32
  %right.sitofp35 = sitofp i64 %right.bits31 to double
  %right.bitcast36 = bitcast i64 %right.bits31 to double
  %right.double37 = select i1 %is_float.right21, double %right.bitcast36, double %right.sitofp35
  %result.bits38 = bitcast double %left.double34 to i64
  %result.float.ptr39 = inttoptr i64 %result.bits38 to ptr
  %rcc.val.heap40 = call ptr @malloc(i64 16)
  %tag.ptr41 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap40, i32 0, i32 0
  store i64 2, ptr %tag.ptr41, align 4
  %payload.ptr42 = getelementptr inbounds nuw %RCCValue, ptr %rcc.val.heap40, i32 0, i32 1
  store ptr %result.float.ptr39, ptr %payload.ptr42, align 8
  store ptr %rcc.val.heap40, ptr %arith.result26, align 8
  br label %arith.merge25

arith.int24:                                      ; preds = %arith.int
  %payload.ptr43 = getelementptr inbounds nuw %RCCValue, ptr %p.load16, i32 0, i32 1
  %payload44 = load ptr, ptr %payload.ptr43, align 8
  %payload45 = load ptr, ptr getelementptr inbounds nuw (%RCCValue, ptr null, i32 0, i32 1), align 8
  %left.int46 = ptrtoint ptr %payload44 to i64
  %right.int47 = ptrtoint ptr %payload45 to i64
  %payload.ptr48 = getelementptr inbounds nuw %RCCValue, ptr %p.load16, i32 0, i32 1
  %payload49 = load ptr, ptr %payload.ptr48, align 8
  %call.sout50 = call ptr @sout(ptr null)
  %call. = call ptr @0()
  store ptr null, ptr %age, align 8
  %call.51 = call ptr @1()
  store ptr %call.51, ptr %age, align 8
  %age.load = load ptr, ptr %age, align 8
  %call.sout52 = call ptr @sout(ptr %age.load)
  ret ptr null

arith.merge25:                                    ; preds = %arith.float23
}

declare ptr @malloc(i64)

declare ptr @Person(ptr, ptr)

declare ptr @sout(ptr)

declare ptr @0()

declare ptr @1()
