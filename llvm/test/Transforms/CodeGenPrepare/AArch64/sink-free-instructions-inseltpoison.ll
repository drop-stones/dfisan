; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -codegenprepare -S | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown"

define <8 x i16> @sink_zext(<8 x i8> %a, <8 x i8> %b, i1 %c) {
; CHECK-LABEL: @sink_zext(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 [[C:%.*]], label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = zext <8 x i8> [[A:%.*]] to <8 x i16>
; CHECK-NEXT:    [[ZB_1:%.*]] = zext <8 x i8> [[B:%.*]] to <8 x i16>
; CHECK-NEXT:    [[RES_1:%.*]] = add <8 x i16> [[TMP0]], [[ZB_1]]
; CHECK-NEXT:    ret <8 x i16> [[RES_1]]
; CHECK:       if.else:
; CHECK-NEXT:    [[TMP1:%.*]] = zext <8 x i8> [[A]] to <8 x i16>
; CHECK-NEXT:    [[ZB_2:%.*]] = zext <8 x i8> [[B]] to <8 x i16>
; CHECK-NEXT:    [[RES_2:%.*]] = sub <8 x i16> [[TMP1]], [[ZB_2]]
; CHECK-NEXT:    ret <8 x i16> [[RES_2]]
;
entry:
  %za = zext <8 x i8> %a to <8 x i16>
  br i1 %c, label %if.then, label %if.else

if.then:
  %zb.1 = zext <8 x i8> %b to <8 x i16>
  %res.1 = add <8 x i16> %za, %zb.1
  ret <8 x i16> %res.1

if.else:
  %zb.2 = zext <8 x i8> %b to <8 x i16>
  %res.2 = sub <8 x i16> %za, %zb.2
  ret <8 x i16> %res.2
}

define <8 x i16> @sink_sext(<8 x i8> %a, <8 x i8> %b, i1 %c) {
; CHECK-LABEL: @sink_sext(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 [[C:%.*]], label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = sext <8 x i8> [[A:%.*]] to <8 x i16>
; CHECK-NEXT:    [[ZB_1:%.*]] = sext <8 x i8> [[B:%.*]] to <8 x i16>
; CHECK-NEXT:    [[RES_1:%.*]] = add <8 x i16> [[TMP0]], [[ZB_1]]
; CHECK-NEXT:    ret <8 x i16> [[RES_1]]
; CHECK:       if.else:
; CHECK-NEXT:    [[TMP1:%.*]] = sext <8 x i8> [[A]] to <8 x i16>
; CHECK-NEXT:    [[ZB_2:%.*]] = sext <8 x i8> [[B]] to <8 x i16>
; CHECK-NEXT:    [[RES_2:%.*]] = sub <8 x i16> [[TMP1]], [[ZB_2]]
; CHECK-NEXT:    ret <8 x i16> [[RES_2]]
;
entry:
  %za = sext <8 x i8> %a to <8 x i16>
  br i1 %c, label %if.then, label %if.else

if.then:
  %zb.1 = sext <8 x i8> %b to <8 x i16>
  %res.1 = add <8 x i16> %za, %zb.1
  ret <8 x i16> %res.1

if.else:
  %zb.2 = sext <8 x i8> %b to <8 x i16>
  %res.2 = sub <8 x i16> %za, %zb.2
  ret <8 x i16> %res.2
}

define <8 x i16> @do_not_sink_nonfree_zext(<8 x i8> %a, <8 x i8> %b, i1 %c) {
; CHECK-LABEL: @do_not_sink_nonfree_zext(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 [[C:%.*]], label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = sext <8 x i8> [[A:%.*]] to <8 x i16>
; CHECK-NEXT:    [[ZB_1:%.*]] = sext <8 x i8> [[B:%.*]] to <8 x i16>
; CHECK-NEXT:    [[RES_1:%.*]] = add <8 x i16> [[TMP0]], [[ZB_1]]
; CHECK-NEXT:    ret <8 x i16> [[RES_1]]
; CHECK:       if.else:
; CHECK-NEXT:    [[ZB_2:%.*]] = sext <8 x i8> [[B]] to <8 x i16>
; CHECK-NEXT:    ret <8 x i16> [[ZB_2]]
;
entry:
  %za = sext <8 x i8> %a to <8 x i16>
  br i1 %c, label %if.then, label %if.else

if.then:
  %zb.1 = sext <8 x i8> %b to <8 x i16>
  %res.1 = add <8 x i16> %za, %zb.1
  ret <8 x i16> %res.1

if.else:
  %zb.2 = sext <8 x i8> %b to <8 x i16>
  ret <8 x i16> %zb.2
}

define <8 x i16> @do_not_sink_nonfree_sext(<8 x i8> %a, <8 x i8> %b, i1 %c) {
; CHECK-LABEL: @do_not_sink_nonfree_sext(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 [[C:%.*]], label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = sext <8 x i8> [[A:%.*]] to <8 x i16>
; CHECK-NEXT:    [[ZB_1:%.*]] = sext <8 x i8> [[B:%.*]] to <8 x i16>
; CHECK-NEXT:    [[RES_1:%.*]] = add <8 x i16> [[TMP0]], [[ZB_1]]
; CHECK-NEXT:    ret <8 x i16> [[RES_1]]
; CHECK:       if.else:
; CHECK-NEXT:    [[ZB_2:%.*]] = sext <8 x i8> [[B]] to <8 x i16>
; CHECK-NEXT:    ret <8 x i16> [[ZB_2]]
;
entry:
  %za = sext <8 x i8> %a to <8 x i16>
  br i1 %c, label %if.then, label %if.else

if.then:
  %zb.1 = sext <8 x i8> %b to <8 x i16>
  %res.1 = add <8 x i16> %za, %zb.1
  ret <8 x i16> %res.1

if.else:
  %zb.2 = sext <8 x i8> %b to <8 x i16>
  ret <8 x i16> %zb.2
}

; The masks used are suitable for umull, sink shufflevector to users.
define <8 x i16> @sink_shufflevector_umull(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: @sink_shufflevector_umull(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 undef, label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = shufflevector <16 x i8> [[A:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[S2:%.*]] = shufflevector <16 x i8> [[B:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[VMULL0:%.*]] = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> [[TMP0]], <8 x i8> [[S2]])
; CHECK-NEXT:    ret <8 x i16> [[VMULL0]]
; CHECK:       if.else:
; CHECK-NEXT:    [[TMP1:%.*]] = shufflevector <16 x i8> [[A]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[S4:%.*]] = shufflevector <16 x i8> [[B]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[VMULL1:%.*]] = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> [[TMP1]], <8 x i8> [[S4]])
; CHECK-NEXT:    ret <8 x i16> [[VMULL1]]
;
entry:
  %s1 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %s3 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  br i1 undef, label %if.then, label %if.else

if.then:
  %s2 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %vmull0 = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> %s1, <8 x i8> %s2) #3
  ret <8 x i16> %vmull0

if.else:
  %s4 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %vmull1 = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> %s3, <8 x i8> %s4) #3
  ret <8 x i16> %vmull1
}

; Both exts and their shufflevector operands can be sunk.
define <8 x i16> @sink_shufflevector_ext_subadd(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: @sink_shufflevector_ext_subadd(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br i1 undef, label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = shufflevector <16 x i8> [[A:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[TMP1:%.*]] = zext <8 x i8> [[TMP0]] to <8 x i16>
; CHECK-NEXT:    [[S2:%.*]] = shufflevector <16 x i8> [[B:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[Z2:%.*]] = zext <8 x i8> [[S2]] to <8 x i16>
; CHECK-NEXT:    [[RES1:%.*]] = add <8 x i16> [[TMP1]], [[Z2]]
; CHECK-NEXT:    ret <8 x i16> [[RES1]]
; CHECK:       if.else:
; CHECK-NEXT:    [[TMP2:%.*]] = shufflevector <16 x i8> [[A]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[TMP3:%.*]] = sext <8 x i8> [[TMP2]] to <8 x i16>
; CHECK-NEXT:    [[S4:%.*]] = shufflevector <16 x i8> [[B]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[Z4:%.*]] = sext <8 x i8> [[S4]] to <8 x i16>
; CHECK-NEXT:    [[RES2:%.*]] = sub <8 x i16> [[TMP3]], [[Z4]]
; CHECK-NEXT:    ret <8 x i16> [[RES2]]
;
entry:
  %s1 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %z1 = zext <8 x i8> %s1 to <8 x i16>
  %s3 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %z3 = sext <8 x i8> %s3 to <8 x i16>
  br i1 undef, label %if.then, label %if.else

if.then:
  %s2 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %z2 = zext <8 x i8> %s2 to <8 x i16>
  %res1 = add <8 x i16> %z1, %z2
  ret <8 x i16> %res1

if.else:
  %s4 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %z4 = sext <8 x i8> %s4 to <8 x i16>
  %res2 = sub <8 x i16> %z3, %z4
  ret <8 x i16> %res2
}


declare void @user1(<8 x i16>)

; Both exts and their shufflevector operands can be sunk.
define <8 x i16> @sink_shufflevector_ext_subadd_multiuse(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: @sink_shufflevector_ext_subadd_multiuse(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[S3:%.*]] = shufflevector <16 x i8> [[A:%.*]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[Z3:%.*]] = sext <8 x i8> [[S3]] to <8 x i16>
; CHECK-NEXT:    call void @user1(<8 x i16> [[Z3]])
; CHECK-NEXT:    br i1 undef, label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[TMP0:%.*]] = shufflevector <16 x i8> [[A]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[TMP1:%.*]] = zext <8 x i8> [[TMP0]] to <8 x i16>
; CHECK-NEXT:    [[S2:%.*]] = shufflevector <16 x i8> [[B:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[Z2:%.*]] = zext <8 x i8> [[S2]] to <8 x i16>
; CHECK-NEXT:    [[RES1:%.*]] = add <8 x i16> [[TMP1]], [[Z2]]
; CHECK-NEXT:    ret <8 x i16> [[RES1]]
; CHECK:       if.else:
; CHECK-NEXT:    [[TMP2:%.*]] = shufflevector <16 x i8> [[A]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[TMP3:%.*]] = sext <8 x i8> [[TMP2]] to <8 x i16>
; CHECK-NEXT:    [[S4:%.*]] = shufflevector <16 x i8> [[B]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[Z4:%.*]] = sext <8 x i8> [[S4]] to <8 x i16>
; CHECK-NEXT:    [[RES2:%.*]] = sub <8 x i16> [[TMP3]], [[Z4]]
; CHECK-NEXT:    ret <8 x i16> [[RES2]]
;
entry:
  %s1 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %z1 = zext <8 x i8> %s1 to <8 x i16>
  %s3 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %z3 = sext <8 x i8> %s3 to <8 x i16>
  call void @user1(<8 x i16> %z3)
  br i1 undef, label %if.then, label %if.else

if.then:
  %s2 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %z2 = zext <8 x i8> %s2 to <8 x i16>
  %res1 = add <8 x i16> %z1, %z2
  ret <8 x i16> %res1

if.else:
  %s4 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %z4 = sext <8 x i8> %s4 to <8 x i16>
  %res2 = sub <8 x i16> %z3, %z4
  ret <8 x i16> %res2
}


; The masks used are not suitable for umull, do not sink.
define <8 x i16> @no_sink_shufflevector_umull(<16 x i8> %a, <16 x i8> %b) {
; CHECK-LABEL: @no_sink_shufflevector_umull(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[S1:%.*]] = shufflevector <16 x i8> [[A:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 1, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[S3:%.*]] = shufflevector <16 x i8> [[A]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    br i1 undef, label [[IF_THEN:%.*]], label [[IF_ELSE:%.*]]
; CHECK:       if.then:
; CHECK-NEXT:    [[S2:%.*]] = shufflevector <16 x i8> [[B:%.*]], <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[VMULL0:%.*]] = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> [[S1]], <8 x i8> [[S2]])
; CHECK-NEXT:    ret <8 x i16> [[VMULL0]]
; CHECK:       if.else:
; CHECK-NEXT:    [[S4:%.*]] = shufflevector <16 x i8> [[B]], <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 10, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:    [[VMULL1:%.*]] = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> [[S3]], <8 x i8> [[S4]])
; CHECK-NEXT:    ret <8 x i16> [[VMULL1]]
;
entry:
  %s1 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 1, i32 5, i32 6, i32 7>
  %s3 = shufflevector <16 x i8> %a, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  br i1 undef, label %if.then, label %if.else

if.then:
  %s2 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %vmull0 = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> %s1, <8 x i8> %s2) #3
  ret <8 x i16> %vmull0

if.else:
  %s4 = shufflevector <16 x i8> %b, <16 x i8> poison, <8 x i32> <i32 8, i32 9, i32 10, i32 10, i32 12, i32 13, i32 14, i32 15>
  %vmull1 = tail call <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8> %s3, <8 x i8> %s4) #3
  ret <8 x i16> %vmull1
}


; Function Attrs: nounwind readnone
declare <8 x i16> @llvm.aarch64.neon.umull.v8i16(<8 x i8>, <8 x i8>) #2
