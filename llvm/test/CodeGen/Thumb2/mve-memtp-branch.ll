; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=thumbv8.1m.main-none-none-eabi -mattr=+mve.fp -verify-machineinstrs -arm-memtransfer-tploop=force-enabled %s -o - | FileCheck %s

; In this test, the successors of various blocks were becoming invalid after
; ifcvt as the blocks did not properly fall through to the successor after a
; WhileLoopStart

@arr_183 = external dso_local local_unnamed_addr global [20 x [23 x [19 x i8]]], align 1
define i32 @a(i8 zeroext %b, [3 x i8]* nocapture readonly %c, [3 x i32]* nocapture readonly %d) {
; CHECK-LABEL: a:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    .save {r4, r5, r7, lr}
; CHECK-NEXT:    push {r4, r5, r7, lr}
; CHECK-NEXT:    cmp r0, #1
; CHECK-NEXT:    bls.w .LBB0_12
; CHECK-NEXT:  @ %bb.1: @ %for.body.us.preheader
; CHECK-NEXT:    movw r5, :lower16:arr_183
; CHECK-NEXT:    movs r3, #0
; CHECK-NEXT:    movt r5, :upper16:arr_183
; CHECK-NEXT:    mov.w r12, #19
; CHECK-NEXT:    vmov.i32 q0, #0x0
; CHECK-NEXT:    vmov.i32 q1, #0x0
; CHECK-NEXT:    vmov.i32 q2, #0x0
; CHECK-NEXT:    vmov.i32 q3, #0x0
; CHECK-NEXT:    b .LBB0_3
; CHECK-NEXT:  .LBB0_2: @ %land.end.us.3
; CHECK-NEXT:    @ in Loop: Header=BB0_3 Depth=1
; CHECK-NEXT:    movs r3, #1
; CHECK-NEXT:  .LBB0_3: @ %for.body.us
; CHECK-NEXT:    @ =>This Loop Header: Depth=1
; CHECK-NEXT:    @ Child Loop BB0_4 Depth 2
; CHECK-NEXT:    @ Child Loop BB0_6 Depth 2
; CHECK-NEXT:    @ Child Loop BB0_8 Depth 2
; CHECK-NEXT:    @ Child Loop BB0_11 Depth 2
; CHECK-NEXT:    ldr.w r0, [r2, r3, lsl #2]
; CHECK-NEXT:    cmp r0, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r0, [r1, r3]
; CHECK-NEXT:    moveq r0, #0
; CHECK-NEXT:    mla r3, r3, r12, r5
; CHECK-NEXT:    add r3, r0
; CHECK-NEXT:    rsb.w r0, r0, #108
; CHECK-NEXT:    wlstp.8 lr, r0, .LBB0_5
; CHECK-NEXT:  .LBB0_4: @ Parent Loop BB0_3 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q0, [r3], #16
; CHECK-NEXT:    letp lr, .LBB0_4
; CHECK-NEXT:  .LBB0_5: @ %land.end.us
; CHECK-NEXT:    @ in Loop: Header=BB0_3 Depth=1
; CHECK-NEXT:    ldr r0, [r2, #4]
; CHECK-NEXT:    cmp r0, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r0, [r1, #1]
; CHECK-NEXT:    moveq r0, #0
; CHECK-NEXT:    adds r3, r5, r0
; CHECK-NEXT:    rsb.w r0, r0, #108
; CHECK-NEXT:    adds r3, #19
; CHECK-NEXT:    wlstp.8 lr, r0, .LBB0_7
; CHECK-NEXT:  .LBB0_6: @ Parent Loop BB0_3 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q1, [r3], #16
; CHECK-NEXT:    letp lr, .LBB0_6
; CHECK-NEXT:  .LBB0_7: @ %land.end.us.1
; CHECK-NEXT:    @ in Loop: Header=BB0_3 Depth=1
; CHECK-NEXT:    ldr r0, [r2, #4]
; CHECK-NEXT:    cmp r0, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r0, [r1, #1]
; CHECK-NEXT:    moveq r0, #0
; CHECK-NEXT:    adds r3, r5, r0
; CHECK-NEXT:    rsb.w r0, r0, #108
; CHECK-NEXT:    adds r3, #19
; CHECK-NEXT:    wlstp.8 lr, r0, .LBB0_9
; CHECK-NEXT:  .LBB0_8: @ Parent Loop BB0_3 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q2, [r3], #16
; CHECK-NEXT:    letp lr, .LBB0_8
; CHECK-NEXT:  .LBB0_9: @ %land.end.us.2
; CHECK-NEXT:    @ in Loop: Header=BB0_3 Depth=1
; CHECK-NEXT:    ldr r0, [r2, #4]
; CHECK-NEXT:    cmp r0, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r0, [r1, #1]
; CHECK-NEXT:    moveq r0, #0
; CHECK-NEXT:    adds r3, r5, r0
; CHECK-NEXT:    rsb.w r0, r0, #108
; CHECK-NEXT:    add.w r4, r0, #15
; CHECK-NEXT:    adds r3, #19
; CHECK-NEXT:    lsrs r4, r4, #4
; CHECK-NEXT:    cmp.w r4, #0
; CHECK-NEXT:    beq .LBB0_2
; CHECK-NEXT:  @ %bb.10: @ %land.end.us.2
; CHECK-NEXT:    @ in Loop: Header=BB0_3 Depth=1
; CHECK-NEXT:    dlstp.8 lr, r0
; CHECK-NEXT:  .LBB0_11: @ Parent Loop BB0_3 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q3, [r3], #16
; CHECK-NEXT:    letp lr, .LBB0_11
; CHECK-NEXT:    b .LBB0_2
; CHECK-NEXT:  .LBB0_12:
; CHECK-NEXT:    movw r12, :lower16:arr_183
; CHECK-NEXT:    vmov.i32 q0, #0x0
; CHECK-NEXT:    movt r12, :upper16:arr_183
; CHECK-NEXT:    vmov.i32 q1, #0x0
; CHECK-NEXT:    vmov.i32 q2, #0x0
; CHECK-NEXT:    vmov.i32 q3, #0x0
; CHECK-NEXT:    b .LBB0_14
; CHECK-NEXT:  .LBB0_13: @ %for.body.lr.ph.3
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    ldr r3, [r2, #4]
; CHECK-NEXT:    cmp r3, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r3, [r1, #1]
; CHECK-NEXT:    moveq r3, #0
; CHECK-NEXT:    add.w r5, r12, r3
; CHECK-NEXT:    rsb.w r3, r3, #108
; CHECK-NEXT:    add.w r4, r5, #19
; CHECK-NEXT:    wlstp.8 lr, r3, .LBB0_14
; CHECK-NEXT:    b .LBB0_24
; CHECK-NEXT:  .LBB0_14: @ %for.cond
; CHECK-NEXT:    @ =>This Loop Header: Depth=1
; CHECK-NEXT:    @ Child Loop BB0_16 Depth 2
; CHECK-NEXT:    @ Child Loop BB0_19 Depth 2
; CHECK-NEXT:    @ Child Loop BB0_22 Depth 2
; CHECK-NEXT:    @ Child Loop BB0_24 Depth 2
; CHECK-NEXT:    cmp r0, #2
; CHECK-NEXT:    blo .LBB0_17
; CHECK-NEXT:  @ %bb.15: @ %for.body.lr.ph
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    ldr r3, [r2, #4]
; CHECK-NEXT:    cmp r3, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r3, [r1, #1]
; CHECK-NEXT:    moveq r3, #0
; CHECK-NEXT:    add.w r5, r12, r3
; CHECK-NEXT:    rsb.w r3, r3, #108
; CHECK-NEXT:    add.w r4, r5, #19
; CHECK-NEXT:    wlstp.8 lr, r3, .LBB0_17
; CHECK-NEXT:  .LBB0_16: @ Parent Loop BB0_14 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q0, [r4], #16
; CHECK-NEXT:    letp lr, .LBB0_16
; CHECK-NEXT:  .LBB0_17: @ %for.cond.backedge
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    cmp r0, #2
; CHECK-NEXT:    blo .LBB0_20
; CHECK-NEXT:  @ %bb.18: @ %for.body.lr.ph.1
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    ldr r3, [r2, #4]
; CHECK-NEXT:    cmp r3, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r3, [r1, #1]
; CHECK-NEXT:    moveq r3, #0
; CHECK-NEXT:    add.w r5, r12, r3
; CHECK-NEXT:    rsb.w r3, r3, #108
; CHECK-NEXT:    add.w r4, r5, #19
; CHECK-NEXT:    wlstp.8 lr, r3, .LBB0_20
; CHECK-NEXT:  .LBB0_19: @ Parent Loop BB0_14 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q1, [r4], #16
; CHECK-NEXT:    letp lr, .LBB0_19
; CHECK-NEXT:  .LBB0_20: @ %for.cond.backedge.1
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    cmp r0, #2
; CHECK-NEXT:    blo .LBB0_23
; CHECK-NEXT:  @ %bb.21: @ %for.body.lr.ph.2
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    ldr r3, [r2, #4]
; CHECK-NEXT:    cmp r3, #0
; CHECK-NEXT:    ite ne
; CHECK-NEXT:    ldrbne r3, [r1, #1]
; CHECK-NEXT:    moveq r3, #0
; CHECK-NEXT:    add.w r5, r12, r3
; CHECK-NEXT:    rsb.w r3, r3, #108
; CHECK-NEXT:    add.w r4, r5, #19
; CHECK-NEXT:    wlstp.8 lr, r3, .LBB0_23
; CHECK-NEXT:  .LBB0_22: @ Parent Loop BB0_14 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q2, [r4], #16
; CHECK-NEXT:    letp lr, .LBB0_22
; CHECK-NEXT:  .LBB0_23: @ %for.cond.backedge.2
; CHECK-NEXT:    @ in Loop: Header=BB0_14 Depth=1
; CHECK-NEXT:    cmp r0, #2
; CHECK-NEXT:    blo .LBB0_14
; CHECK-NEXT:    b .LBB0_13
; CHECK-NEXT:  .LBB0_24: @ Parent Loop BB0_14 Depth=1
; CHECK-NEXT:    @ => This Inner Loop Header: Depth=2
; CHECK-NEXT:    vstrb.8 q3, [r4], #16
; CHECK-NEXT:    letp lr, .LBB0_24
; CHECK-NEXT:    b .LBB0_14
entry:
  %cmp = icmp ugt i8 %b, 1
  br i1 %cmp, label %for.body.us.preheader, label %for.cond.preheader

for.cond.preheader:                               ; preds = %entry
  %cmp43 = icmp ugt i8 %b, 1
  %arrayidx6 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  %cmp43.1 = icmp ugt i8 %b, 1
  %arrayidx6.1 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12.1 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  %cmp43.2 = icmp ugt i8 %b, 1
  %arrayidx6.2 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12.2 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  %cmp43.3 = icmp ugt i8 %b, 1
  %arrayidx6.3 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12.3 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  br label %for.cond

for.body.us.preheader:                            ; preds = %entry
  %arrayidx6.us.1 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12.us.1 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  %arrayidx6.us.2 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12.us.2 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  %arrayidx6.us.3 = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 1
  %arrayidx12.us.3 = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 1
  br label %for.body.us

for.cond:                                         ; preds = %for.cond.backedge.3, %for.cond.preheader
  br i1 %cmp43, label %for.body.lr.ph, label %for.cond.backedge

for.body.lr.ph:                                   ; preds = %for.cond
  %0 = load i32, i32* %arrayidx6, align 4
  %tobool7.not = icmp eq i32 %0, 0
  br i1 %tobool7.not, label %land.end, label %land.rhs

for.body.us:                                      ; preds = %land.end.us.3, %for.body.us.preheader
  %conv44.us = phi i32 [ 0, %for.body.us.preheader ], [ 1, %land.end.us.3 ]
  %arrayidx6.us = getelementptr inbounds [3 x i32], [3 x i32]* %d, i32 0, i32 %conv44.us
  %1 = load i32, i32* %arrayidx6.us, align 4
  %tobool7.not.us = icmp eq i32 %1, 0
  br i1 %tobool7.not.us, label %land.end.us, label %land.rhs.us

land.rhs.us:                                      ; preds = %for.body.us
  %arrayidx12.us = getelementptr inbounds [3 x i8], [3 x i8]* %c, i32 0, i32 %conv44.us
  %2 = load i8, i8* %arrayidx12.us, align 1
  %tobool13.us = zext i8 %2 to i32
  br label %land.end.us

land.end.us:                                      ; preds = %land.rhs.us, %for.body.us
  %3 = phi i32 [ 0, %for.body.us ], [ %tobool13.us, %land.rhs.us ]
  %scevgep45 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 %conv44.us, i32 %3
  %4 = sub nuw nsw i32 108, %3
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep45, i8 0, i32 %4, i1 false)
  %5 = load i32, i32* %arrayidx6.us.1, align 4
  %tobool7.not.us.1 = icmp eq i32 %5, 0
  br i1 %tobool7.not.us.1, label %land.end.us.1, label %land.rhs.us.1

land.rhs:                                         ; preds = %for.body.lr.ph
  %6 = load i8, i8* %arrayidx12, align 1
  %tobool13 = zext i8 %6 to i32
  br label %land.end

land.end:                                         ; preds = %land.rhs, %for.body.lr.ph
  %7 = phi i32 [ 0, %for.body.lr.ph ], [ %tobool13, %land.rhs ]
  %scevgep = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %7
  %8 = sub nuw nsw i32 108, %7
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep, i8 0, i32 %8, i1 false)
  br label %for.cond.backedge

for.cond.backedge:                                ; preds = %land.end, %for.cond
  br i1 %cmp43.1, label %for.body.lr.ph.1, label %for.cond.backedge.1

for.body.lr.ph.1:                                 ; preds = %for.cond.backedge
  %9 = load i32, i32* %arrayidx6.1, align 4
  %tobool7.not.1 = icmp eq i32 %9, 0
  br i1 %tobool7.not.1, label %land.end.1, label %land.rhs.1

land.rhs.1:                                       ; preds = %for.body.lr.ph.1
  %10 = load i8, i8* %arrayidx12.1, align 1
  %tobool13.1 = zext i8 %10 to i32
  br label %land.end.1

land.end.1:                                       ; preds = %land.rhs.1, %for.body.lr.ph.1
  %11 = phi i32 [ 0, %for.body.lr.ph.1 ], [ %tobool13.1, %land.rhs.1 ]
  %scevgep.1 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %11
  %12 = sub nuw nsw i32 108, %11
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep.1, i8 0, i32 %12, i1 false)
  br label %for.cond.backedge.1

for.cond.backedge.1:                              ; preds = %land.end.1, %for.cond.backedge
  br i1 %cmp43.2, label %for.body.lr.ph.2, label %for.cond.backedge.2

for.body.lr.ph.2:                                 ; preds = %for.cond.backedge.1
  %13 = load i32, i32* %arrayidx6.2, align 4
  %tobool7.not.2 = icmp eq i32 %13, 0
  br i1 %tobool7.not.2, label %land.end.2, label %land.rhs.2

land.rhs.2:                                       ; preds = %for.body.lr.ph.2
  %14 = load i8, i8* %arrayidx12.2, align 1
  %tobool13.2 = zext i8 %14 to i32
  br label %land.end.2

land.end.2:                                       ; preds = %land.rhs.2, %for.body.lr.ph.2
  %15 = phi i32 [ 0, %for.body.lr.ph.2 ], [ %tobool13.2, %land.rhs.2 ]
  %scevgep.2 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %15
  %16 = sub nuw nsw i32 108, %15
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep.2, i8 0, i32 %16, i1 false)
  br label %for.cond.backedge.2

for.cond.backedge.2:                              ; preds = %land.end.2, %for.cond.backedge.1
  br i1 %cmp43.3, label %for.body.lr.ph.3, label %for.cond.backedge.3

for.body.lr.ph.3:                                 ; preds = %for.cond.backedge.2
  %17 = load i32, i32* %arrayidx6.3, align 4
  %tobool7.not.3 = icmp eq i32 %17, 0
  br i1 %tobool7.not.3, label %land.end.3, label %land.rhs.3

land.rhs.3:                                       ; preds = %for.body.lr.ph.3
  %18 = load i8, i8* %arrayidx12.3, align 1
  %tobool13.3 = zext i8 %18 to i32
  br label %land.end.3

land.end.3:                                       ; preds = %land.rhs.3, %for.body.lr.ph.3
  %19 = phi i32 [ 0, %for.body.lr.ph.3 ], [ %tobool13.3, %land.rhs.3 ]
  %scevgep.3 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %19
  %20 = sub nuw nsw i32 108, %19
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep.3, i8 0, i32 %20, i1 false)
  br label %for.cond.backedge.3

for.cond.backedge.3:                              ; preds = %land.end.3, %for.cond.backedge.2
  br label %for.cond

land.rhs.us.1:                                    ; preds = %land.end.us
  %21 = load i8, i8* %arrayidx12.us.1, align 1
  %tobool13.us.1 = zext i8 %21 to i32
  br label %land.end.us.1

land.end.us.1:                                    ; preds = %land.rhs.us.1, %land.end.us
  %22 = phi i32 [ 0, %land.end.us ], [ %tobool13.us.1, %land.rhs.us.1 ]
  %scevgep45.1 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %22
  %23 = sub nuw nsw i32 108, %22
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep45.1, i8 0, i32 %23, i1 false)
  %24 = load i32, i32* %arrayidx6.us.2, align 4
  %tobool7.not.us.2 = icmp eq i32 %24, 0
  br i1 %tobool7.not.us.2, label %land.end.us.2, label %land.rhs.us.2

land.rhs.us.2:                                    ; preds = %land.end.us.1
  %25 = load i8, i8* %arrayidx12.us.2, align 1
  %tobool13.us.2 = zext i8 %25 to i32
  br label %land.end.us.2

land.end.us.2:                                    ; preds = %land.rhs.us.2, %land.end.us.1
  %26 = phi i32 [ 0, %land.end.us.1 ], [ %tobool13.us.2, %land.rhs.us.2 ]
  %scevgep45.2 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %26
  %27 = sub nuw nsw i32 108, %26
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep45.2, i8 0, i32 %27, i1 false)
  %28 = load i32, i32* %arrayidx6.us.3, align 4
  %tobool7.not.us.3 = icmp eq i32 %28, 0
  br i1 %tobool7.not.us.3, label %land.end.us.3, label %land.rhs.us.3

land.rhs.us.3:                                    ; preds = %land.end.us.2
  %29 = load i8, i8* %arrayidx12.us.3, align 1
  %tobool13.us.3 = zext i8 %29 to i32
  br label %land.end.us.3

land.end.us.3:                                    ; preds = %land.rhs.us.3, %land.end.us.2
  %30 = phi i32 [ 0, %land.end.us.2 ], [ %tobool13.us.3, %land.rhs.us.3 ]
  %scevgep45.3 = getelementptr [20 x [23 x [19 x i8]]], [20 x [23 x [19 x i8]]]* @arr_183, i32 0, i32 0, i32 1, i32 %30
  %31 = sub nuw nsw i32 108, %30
  call void @llvm.memset.p0i8.i32(i8* align 1 %scevgep45.3, i8 0, i32 %31, i1 false)
  br label %for.body.us
}

declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i1)
