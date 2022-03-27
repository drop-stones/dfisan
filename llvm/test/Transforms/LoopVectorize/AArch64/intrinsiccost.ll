; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -loop-vectorize -instcombine -simplifycfg < %s -S -o - | FileCheck %s --check-prefix=CHECK
; RUN: opt -loop-vectorize -debug-only=loop-vectorize -disable-output < %s 2>&1 | FileCheck %s --check-prefix=CHECK-COST
; REQUIRES: asserts

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64--linux-gnu"

; CHECK-COST-LABEL: sadd
; CHECK-COST: Found an estimated cost of 6 for VF 1 For instruction:   %1 = tail call i16 @llvm.sadd.sat.i16(i16 %0, i16 %offset)
; CHECK-COST: Found an estimated cost of 4 for VF 2 For instruction:   %1 = tail call i16 @llvm.sadd.sat.i16(i16 %0, i16 %offset)
; CHECK-COST: Found an estimated cost of 1 for VF 4 For instruction:   %1 = tail call i16 @llvm.sadd.sat.i16(i16 %0, i16 %offset)
; CHECK-COST: Found an estimated cost of 1 for VF 8 For instruction:   %1 = tail call i16 @llvm.sadd.sat.i16(i16 %0, i16 %offset)

define void @saddsat(i16* nocapture readonly %pSrc, i16 signext %offset, i16* nocapture noalias %pDst, i32 %blockSize) #0 {
; CHECK-LABEL: @saddsat(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[CMP_NOT6:%.*]] = icmp eq i32 [[BLOCKSIZE:%.*]], 0
; CHECK-NEXT:    br i1 [[CMP_NOT6]], label [[WHILE_END:%.*]], label [[WHILE_BODY_PREHEADER:%.*]]
; CHECK:       while.body.preheader:
; CHECK-NEXT:    [[TMP0:%.*]] = add i32 [[BLOCKSIZE]], -1
; CHECK-NEXT:    [[TMP1:%.*]] = zext i32 [[TMP0]] to i64
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i64 [[TMP1]], 1
; CHECK-NEXT:    [[MIN_ITERS_CHECK:%.*]] = icmp ult i32 [[TMP0]], 15
; CHECK-NEXT:    br i1 [[MIN_ITERS_CHECK]], label [[SCALAR_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    [[N_VEC:%.*]] = and i64 [[TMP2]], 8589934576
; CHECK-NEXT:    [[CAST_CRD:%.*]] = trunc i64 [[N_VEC]] to i32
; CHECK-NEXT:    [[IND_END:%.*]] = sub i32 [[BLOCKSIZE]], [[CAST_CRD]]
; CHECK-NEXT:    [[IND_END2:%.*]] = getelementptr i16, i16* [[PSRC:%.*]], i64 [[N_VEC]]
; CHECK-NEXT:    [[IND_END4:%.*]] = getelementptr i16, i16* [[PDST:%.*]], i64 [[N_VEC]]
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT:%.*]] = insertelement <8 x i16> poison, i16 [[OFFSET:%.*]], i64 0
; CHECK-NEXT:    [[BROADCAST_SPLAT:%.*]] = shufflevector <8 x i16> [[BROADCAST_SPLATINSERT]], <8 x i16> poison, <8 x i32> zeroinitializer
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT9:%.*]] = insertelement <8 x i16> poison, i16 [[OFFSET]], i64 0
; CHECK-NEXT:    [[BROADCAST_SPLAT10:%.*]] = shufflevector <8 x i16> [[BROADCAST_SPLATINSERT9]], <8 x i16> poison, <8 x i32> zeroinitializer
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[VECTOR_BODY]] ]
; CHECK-NEXT:    [[NEXT_GEP:%.*]] = getelementptr i16, i16* [[PSRC]], i64 [[INDEX]]
; CHECK-NEXT:    [[NEXT_GEP6:%.*]] = getelementptr i16, i16* [[PDST]], i64 [[INDEX]]
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast i16* [[NEXT_GEP]] to <8 x i16>*
; CHECK-NEXT:    [[WIDE_LOAD:%.*]] = load <8 x i16>, <8 x i16>* [[TMP3]], align 2
; CHECK-NEXT:    [[TMP4:%.*]] = getelementptr i16, i16* [[NEXT_GEP]], i64 8
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast i16* [[TMP4]] to <8 x i16>*
; CHECK-NEXT:    [[WIDE_LOAD8:%.*]] = load <8 x i16>, <8 x i16>* [[TMP5]], align 2
; CHECK-NEXT:    [[TMP6:%.*]] = call <8 x i16> @llvm.sadd.sat.v8i16(<8 x i16> [[WIDE_LOAD]], <8 x i16> [[BROADCAST_SPLAT]])
; CHECK-NEXT:    [[TMP7:%.*]] = call <8 x i16> @llvm.sadd.sat.v8i16(<8 x i16> [[WIDE_LOAD8]], <8 x i16> [[BROADCAST_SPLAT10]])
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast i16* [[NEXT_GEP6]] to <8 x i16>*
; CHECK-NEXT:    store <8 x i16> [[TMP6]], <8 x i16>* [[TMP8]], align 2
; CHECK-NEXT:    [[TMP9:%.*]] = getelementptr i16, i16* [[NEXT_GEP6]], i64 8
; CHECK-NEXT:    [[TMP10:%.*]] = bitcast i16* [[TMP9]] to <8 x i16>*
; CHECK-NEXT:    store <8 x i16> [[TMP7]], <8 x i16>* [[TMP10]], align 2
; CHECK-NEXT:    [[INDEX_NEXT]] = add nuw i64 [[INDEX]], 16
; CHECK-NEXT:    [[TMP11:%.*]] = icmp eq i64 [[INDEX_NEXT]], [[N_VEC]]
; CHECK-NEXT:    br i1 [[TMP11]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop [[LOOP0:![0-9]+]]
; CHECK:       middle.block:
; CHECK-NEXT:    [[CMP_N:%.*]] = icmp eq i64 [[TMP2]], [[N_VEC]]
; CHECK-NEXT:    br i1 [[CMP_N]], label [[WHILE_END]], label [[SCALAR_PH]]
; CHECK:       scalar.ph:
; CHECK-NEXT:    [[BC_RESUME_VAL:%.*]] = phi i32 [ [[IND_END]], [[MIDDLE_BLOCK]] ], [ [[BLOCKSIZE]], [[WHILE_BODY_PREHEADER]] ]
; CHECK-NEXT:    [[BC_RESUME_VAL1:%.*]] = phi i16* [ [[IND_END2]], [[MIDDLE_BLOCK]] ], [ [[PSRC]], [[WHILE_BODY_PREHEADER]] ]
; CHECK-NEXT:    [[BC_RESUME_VAL3:%.*]] = phi i16* [ [[IND_END4]], [[MIDDLE_BLOCK]] ], [ [[PDST]], [[WHILE_BODY_PREHEADER]] ]
; CHECK-NEXT:    br label [[WHILE_BODY:%.*]]
; CHECK:       while.body:
; CHECK-NEXT:    [[BLKCNT_09:%.*]] = phi i32 [ [[DEC:%.*]], [[WHILE_BODY]] ], [ [[BC_RESUME_VAL]], [[SCALAR_PH]] ]
; CHECK-NEXT:    [[PSRC_ADDR_08:%.*]] = phi i16* [ [[INCDEC_PTR:%.*]], [[WHILE_BODY]] ], [ [[BC_RESUME_VAL1]], [[SCALAR_PH]] ]
; CHECK-NEXT:    [[PDST_ADDR_07:%.*]] = phi i16* [ [[INCDEC_PTR3:%.*]], [[WHILE_BODY]] ], [ [[BC_RESUME_VAL3]], [[SCALAR_PH]] ]
; CHECK-NEXT:    [[INCDEC_PTR]] = getelementptr inbounds i16, i16* [[PSRC_ADDR_08]], i64 1
; CHECK-NEXT:    [[TMP12:%.*]] = load i16, i16* [[PSRC_ADDR_08]], align 2
; CHECK-NEXT:    [[TMP13:%.*]] = tail call i16 @llvm.sadd.sat.i16(i16 [[TMP12]], i16 [[OFFSET]])
; CHECK-NEXT:    [[INCDEC_PTR3]] = getelementptr inbounds i16, i16* [[PDST_ADDR_07]], i64 1
; CHECK-NEXT:    store i16 [[TMP13]], i16* [[PDST_ADDR_07]], align 2
; CHECK-NEXT:    [[DEC]] = add i32 [[BLKCNT_09]], -1
; CHECK-NEXT:    [[CMP_NOT:%.*]] = icmp eq i32 [[DEC]], 0
; CHECK-NEXT:    br i1 [[CMP_NOT]], label [[WHILE_END]], label [[WHILE_BODY]], !llvm.loop [[LOOP2:![0-9]+]]
; CHECK:       while.end:
; CHECK-NEXT:    ret void
;
entry:
  %cmp.not6 = icmp eq i32 %blockSize, 0
  br i1 %cmp.not6, label %while.end, label %while.body

while.body:                                       ; preds = %entry, %while.body
  %blkCnt.09 = phi i32 [ %dec, %while.body ], [ %blockSize, %entry ]
  %pSrc.addr.08 = phi i16* [ %incdec.ptr, %while.body ], [ %pSrc, %entry ]
  %pDst.addr.07 = phi i16* [ %incdec.ptr3, %while.body ], [ %pDst, %entry ]
  %incdec.ptr = getelementptr inbounds i16, i16* %pSrc.addr.08, i32 1
  %0 = load i16, i16* %pSrc.addr.08, align 2
  %1 = tail call i16 @llvm.sadd.sat.i16(i16 %0, i16 %offset)
  %incdec.ptr3 = getelementptr inbounds i16, i16* %pDst.addr.07, i32 1
  store i16 %1, i16* %pDst.addr.07, align 2
  %dec = add i32 %blkCnt.09, -1
  %cmp.not = icmp eq i32 %dec, 0
  br i1 %cmp.not, label %while.end, label %while.body

while.end:                                        ; preds = %while.body, %entry
  ret void
}

; CHECK-COST-LABEL: umin
; CHECK-COST: Found an estimated cost of 2 for VF 1 For instruction:   %1 = tail call i8 @llvm.umin.i8(i8 %0, i8 %offset)
; CHECK-COST: Found an estimated cost of 1 for VF 2 For instruction:   %1 = tail call i8 @llvm.umin.i8(i8 %0, i8 %offset)
; CHECK-COST: Found an estimated cost of 1 for VF 4 For instruction:   %1 = tail call i8 @llvm.umin.i8(i8 %0, i8 %offset)
; CHECK-COST: Found an estimated cost of 1 for VF 8 For instruction:   %1 = tail call i8 @llvm.umin.i8(i8 %0, i8 %offset)
; CHECK-COST: Found an estimated cost of 1 for VF 16 For instruction:   %1 = tail call i8 @llvm.umin.i8(i8 %0, i8 %offset)

define void @umin(i8* nocapture readonly %pSrc, i8 signext %offset, i8* nocapture noalias %pDst, i32 %blockSize) #0 {
; CHECK-LABEL: @umin(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[CMP_NOT6:%.*]] = icmp eq i32 [[BLOCKSIZE:%.*]], 0
; CHECK-NEXT:    br i1 [[CMP_NOT6]], label [[WHILE_END:%.*]], label [[ITER_CHECK:%.*]]
; CHECK:       iter.check:
; CHECK-NEXT:    [[TMP0:%.*]] = add i32 [[BLOCKSIZE]], -1
; CHECK-NEXT:    [[TMP1:%.*]] = zext i32 [[TMP0]] to i64
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i64 [[TMP1]], 1
; CHECK-NEXT:    [[MIN_ITERS_CHECK:%.*]] = icmp ult i32 [[TMP0]], 7
; CHECK-NEXT:    br i1 [[MIN_ITERS_CHECK]], label [[VEC_EPILOG_SCALAR_PH:%.*]], label [[VECTOR_MAIN_LOOP_ITER_CHECK:%.*]]
; CHECK:       vector.main.loop.iter.check:
; CHECK-NEXT:    [[MIN_ITERS_CHECK1:%.*]] = icmp ult i32 [[TMP0]], 31
; CHECK-NEXT:    br i1 [[MIN_ITERS_CHECK1]], label [[VEC_EPILOG_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    [[N_VEC:%.*]] = and i64 [[TMP2]], 8589934560
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT:%.*]] = insertelement <16 x i8> poison, i8 [[OFFSET:%.*]], i64 0
; CHECK-NEXT:    [[BROADCAST_SPLAT:%.*]] = shufflevector <16 x i8> [[BROADCAST_SPLATINSERT]], <16 x i8> poison, <16 x i32> zeroinitializer
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT6:%.*]] = insertelement <16 x i8> poison, i8 [[OFFSET]], i64 0
; CHECK-NEXT:    [[BROADCAST_SPLAT7:%.*]] = shufflevector <16 x i8> [[BROADCAST_SPLATINSERT6]], <16 x i8> poison, <16 x i32> zeroinitializer
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[VECTOR_BODY]] ]
; CHECK-NEXT:    [[NEXT_GEP:%.*]] = getelementptr i8, i8* [[PSRC:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    [[NEXT_GEP3:%.*]] = getelementptr i8, i8* [[PDST:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast i8* [[NEXT_GEP]] to <16 x i8>*
; CHECK-NEXT:    [[WIDE_LOAD:%.*]] = load <16 x i8>, <16 x i8>* [[TMP3]], align 2
; CHECK-NEXT:    [[TMP4:%.*]] = getelementptr i8, i8* [[NEXT_GEP]], i64 16
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast i8* [[TMP4]] to <16 x i8>*
; CHECK-NEXT:    [[WIDE_LOAD5:%.*]] = load <16 x i8>, <16 x i8>* [[TMP5]], align 2
; CHECK-NEXT:    [[TMP6:%.*]] = call <16 x i8> @llvm.umin.v16i8(<16 x i8> [[WIDE_LOAD]], <16 x i8> [[BROADCAST_SPLAT]])
; CHECK-NEXT:    [[TMP7:%.*]] = call <16 x i8> @llvm.umin.v16i8(<16 x i8> [[WIDE_LOAD5]], <16 x i8> [[BROADCAST_SPLAT7]])
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast i8* [[NEXT_GEP3]] to <16 x i8>*
; CHECK-NEXT:    store <16 x i8> [[TMP6]], <16 x i8>* [[TMP8]], align 2
; CHECK-NEXT:    [[TMP9:%.*]] = getelementptr i8, i8* [[NEXT_GEP3]], i64 16
; CHECK-NEXT:    [[TMP10:%.*]] = bitcast i8* [[TMP9]] to <16 x i8>*
; CHECK-NEXT:    store <16 x i8> [[TMP7]], <16 x i8>* [[TMP10]], align 2
; CHECK-NEXT:    [[INDEX_NEXT]] = add nuw i64 [[INDEX]], 32
; CHECK-NEXT:    [[TMP11:%.*]] = icmp eq i64 [[INDEX_NEXT]], [[N_VEC]]
; CHECK-NEXT:    br i1 [[TMP11]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop [[LOOP4:![0-9]+]]
; CHECK:       middle.block:
; CHECK-NEXT:    [[CMP_N:%.*]] = icmp eq i64 [[TMP2]], [[N_VEC]]
; CHECK-NEXT:    br i1 [[CMP_N]], label [[WHILE_END]], label [[VEC_EPILOG_ITER_CHECK:%.*]]
; CHECK:       vec.epilog.iter.check:
; CHECK-NEXT:    [[IND_END19:%.*]] = getelementptr i8, i8* [[PDST]], i64 [[N_VEC]]
; CHECK-NEXT:    [[IND_END16:%.*]] = getelementptr i8, i8* [[PSRC]], i64 [[N_VEC]]
; CHECK-NEXT:    [[CAST_CRD12:%.*]] = trunc i64 [[N_VEC]] to i32
; CHECK-NEXT:    [[IND_END13:%.*]] = sub i32 [[BLOCKSIZE]], [[CAST_CRD12]]
; CHECK-NEXT:    [[N_VEC_REMAINING:%.*]] = and i64 [[TMP2]], 24
; CHECK-NEXT:    [[MIN_EPILOG_ITERS_CHECK:%.*]] = icmp eq i64 [[N_VEC_REMAINING]], 0
; CHECK-NEXT:    br i1 [[MIN_EPILOG_ITERS_CHECK]], label [[VEC_EPILOG_SCALAR_PH]], label [[VEC_EPILOG_PH]]
; CHECK:       vec.epilog.ph:
; CHECK-NEXT:    [[VEC_EPILOG_RESUME_VAL:%.*]] = phi i64 [ [[N_VEC]], [[VEC_EPILOG_ITER_CHECK]] ], [ 0, [[VECTOR_MAIN_LOOP_ITER_CHECK]] ]
; CHECK-NEXT:    [[TMP12:%.*]] = add i32 [[BLOCKSIZE]], -1
; CHECK-NEXT:    [[TMP13:%.*]] = zext i32 [[TMP12]] to i64
; CHECK-NEXT:    [[TMP14:%.*]] = add nuw nsw i64 [[TMP13]], 1
; CHECK-NEXT:    [[N_VEC9:%.*]] = and i64 [[TMP14]], 8589934584
; CHECK-NEXT:    [[CAST_CRD:%.*]] = trunc i64 [[N_VEC9]] to i32
; CHECK-NEXT:    [[IND_END:%.*]] = sub i32 [[BLOCKSIZE]], [[CAST_CRD]]
; CHECK-NEXT:    [[IND_END15:%.*]] = getelementptr i8, i8* [[PSRC]], i64 [[N_VEC9]]
; CHECK-NEXT:    [[IND_END18:%.*]] = getelementptr i8, i8* [[PDST]], i64 [[N_VEC9]]
; CHECK-NEXT:    [[BROADCAST_SPLATINSERT25:%.*]] = insertelement <8 x i8> poison, i8 [[OFFSET]], i64 0
; CHECK-NEXT:    [[BROADCAST_SPLAT26:%.*]] = shufflevector <8 x i8> [[BROADCAST_SPLATINSERT25]], <8 x i8> poison, <8 x i32> zeroinitializer
; CHECK-NEXT:    br label [[VEC_EPILOG_VECTOR_BODY:%.*]]
; CHECK:       vec.epilog.vector.body:
; CHECK-NEXT:    [[INDEX10:%.*]] = phi i64 [ [[VEC_EPILOG_RESUME_VAL]], [[VEC_EPILOG_PH]] ], [ [[INDEX_NEXT11:%.*]], [[VEC_EPILOG_VECTOR_BODY]] ]
; CHECK-NEXT:    [[NEXT_GEP22:%.*]] = getelementptr i8, i8* [[PSRC]], i64 [[INDEX10]]
; CHECK-NEXT:    [[NEXT_GEP23:%.*]] = getelementptr i8, i8* [[PDST]], i64 [[INDEX10]]
; CHECK-NEXT:    [[TMP15:%.*]] = bitcast i8* [[NEXT_GEP22]] to <8 x i8>*
; CHECK-NEXT:    [[WIDE_LOAD24:%.*]] = load <8 x i8>, <8 x i8>* [[TMP15]], align 2
; CHECK-NEXT:    [[TMP16:%.*]] = call <8 x i8> @llvm.umin.v8i8(<8 x i8> [[WIDE_LOAD24]], <8 x i8> [[BROADCAST_SPLAT26]])
; CHECK-NEXT:    [[TMP17:%.*]] = bitcast i8* [[NEXT_GEP23]] to <8 x i8>*
; CHECK-NEXT:    store <8 x i8> [[TMP16]], <8 x i8>* [[TMP17]], align 2
; CHECK-NEXT:    [[INDEX_NEXT11]] = add nuw i64 [[INDEX10]], 8
; CHECK-NEXT:    [[TMP18:%.*]] = icmp eq i64 [[INDEX_NEXT11]], [[N_VEC9]]
; CHECK-NEXT:    br i1 [[TMP18]], label [[VEC_EPILOG_MIDDLE_BLOCK:%.*]], label [[VEC_EPILOG_VECTOR_BODY]], !llvm.loop [[LOOP5:![0-9]+]]
; CHECK:       vec.epilog.middle.block:
; CHECK-NEXT:    [[CMP_N20:%.*]] = icmp eq i64 [[TMP14]], [[N_VEC9]]
; CHECK-NEXT:    br i1 [[CMP_N20]], label [[WHILE_END]], label [[VEC_EPILOG_SCALAR_PH]]
; CHECK:       vec.epilog.scalar.ph:
; CHECK-NEXT:    [[BC_RESUME_VAL:%.*]] = phi i32 [ [[IND_END]], [[VEC_EPILOG_MIDDLE_BLOCK]] ], [ [[IND_END13]], [[VEC_EPILOG_ITER_CHECK]] ], [ [[BLOCKSIZE]], [[ITER_CHECK]] ]
; CHECK-NEXT:    [[BC_RESUME_VAL14:%.*]] = phi i8* [ [[IND_END15]], [[VEC_EPILOG_MIDDLE_BLOCK]] ], [ [[IND_END16]], [[VEC_EPILOG_ITER_CHECK]] ], [ [[PSRC]], [[ITER_CHECK]] ]
; CHECK-NEXT:    [[BC_RESUME_VAL17:%.*]] = phi i8* [ [[IND_END18]], [[VEC_EPILOG_MIDDLE_BLOCK]] ], [ [[IND_END19]], [[VEC_EPILOG_ITER_CHECK]] ], [ [[PDST]], [[ITER_CHECK]] ]
; CHECK-NEXT:    br label [[WHILE_BODY:%.*]]
; CHECK:       while.body:
; CHECK-NEXT:    [[BLKCNT_09:%.*]] = phi i32 [ [[DEC:%.*]], [[WHILE_BODY]] ], [ [[BC_RESUME_VAL]], [[VEC_EPILOG_SCALAR_PH]] ]
; CHECK-NEXT:    [[PSRC_ADDR_08:%.*]] = phi i8* [ [[INCDEC_PTR:%.*]], [[WHILE_BODY]] ], [ [[BC_RESUME_VAL14]], [[VEC_EPILOG_SCALAR_PH]] ]
; CHECK-NEXT:    [[PDST_ADDR_07:%.*]] = phi i8* [ [[INCDEC_PTR3:%.*]], [[WHILE_BODY]] ], [ [[BC_RESUME_VAL17]], [[VEC_EPILOG_SCALAR_PH]] ]
; CHECK-NEXT:    [[INCDEC_PTR]] = getelementptr inbounds i8, i8* [[PSRC_ADDR_08]], i64 1
; CHECK-NEXT:    [[TMP19:%.*]] = load i8, i8* [[PSRC_ADDR_08]], align 2
; CHECK-NEXT:    [[TMP20:%.*]] = tail call i8 @llvm.umin.i8(i8 [[TMP19]], i8 [[OFFSET]])
; CHECK-NEXT:    [[INCDEC_PTR3]] = getelementptr inbounds i8, i8* [[PDST_ADDR_07]], i64 1
; CHECK-NEXT:    store i8 [[TMP20]], i8* [[PDST_ADDR_07]], align 2
; CHECK-NEXT:    [[DEC]] = add i32 [[BLKCNT_09]], -1
; CHECK-NEXT:    [[CMP_NOT:%.*]] = icmp eq i32 [[DEC]], 0
; CHECK-NEXT:    br i1 [[CMP_NOT]], label [[WHILE_END]], label [[WHILE_BODY]], !llvm.loop [[LOOP6:![0-9]+]]
; CHECK:       while.end:
; CHECK-NEXT:    ret void
;
entry:
  %cmp.not6 = icmp eq i32 %blockSize, 0
  br i1 %cmp.not6, label %while.end, label %while.body

while.body:                                       ; preds = %entry, %while.body
  %blkCnt.09 = phi i32 [ %dec, %while.body ], [ %blockSize, %entry ]
  %pSrc.addr.08 = phi i8* [ %incdec.ptr, %while.body ], [ %pSrc, %entry ]
  %pDst.addr.07 = phi i8* [ %incdec.ptr3, %while.body ], [ %pDst, %entry ]
  %incdec.ptr = getelementptr inbounds i8, i8* %pSrc.addr.08, i32 1
  %0 = load i8, i8* %pSrc.addr.08, align 2
  %1 = tail call i8 @llvm.umin.i8(i8 %0, i8 %offset)
  %incdec.ptr3 = getelementptr inbounds i8, i8* %pDst.addr.07, i32 1
  store i8 %1, i8* %pDst.addr.07, align 2
  %dec = add i32 %blkCnt.09, -1
  %cmp.not = icmp eq i32 %dec, 0
  br i1 %cmp.not, label %while.end, label %while.body

while.end:                                        ; preds = %while.body, %entry
  ret void
}

declare i16 @llvm.sadd.sat.i16(i16, i16)
declare i8 @llvm.umin.i8(i8, i8)

