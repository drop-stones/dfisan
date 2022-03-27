; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -verify-machineinstrs -mtriple=powerpc64le-unknown-unknown < %s  | \
; RUN:   FileCheck %s

; With MemorySSA, everything is taken out of the loop by licm.
; Loads and stores to undef are treated as non-aliasing.
define void @ec_GFp_nistp256_points_mul() {
; CHECK-LABEL: ec_GFp_nistp256_points_mul:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    ld 3, 0(3)
; CHECK-NEXT:    li 4, 0
; CHECK-NEXT:    subfic 5, 3, 0
; CHECK-NEXT:    subfze 5, 4
; CHECK-NEXT:    sradi 5, 5, 63
; CHECK-NEXT:    subc 3, 5, 3
; CHECK-NEXT:    subfe 3, 4, 5
; CHECK-NEXT:    sradi 3, 3, 63
; CHECK-NEXT:    std 3, 0(3)
; CHECK-NEXT:    .p2align 4
; CHECK-NEXT:  .LBB0_1: # %fe_cmovznz.exit.i534.i.15
; CHECK-NEXT:    #
; CHECK-NEXT:    b .LBB0_1

entry:
  br label %fe_cmovznz.exit.i534.i.15

fe_cmovznz.exit.i534.i.15:                        ; preds = %fe_cmovznz.exit.i534.i.15, %entry
  %0 = load i64, i64* undef, align 8
  %1 = load i64, i64* undef, align 8
  %conv.i69.i.i = zext i64 %0 to i128
  %sub.i72.i.i = sub nsw i128 0, %conv.i69.i.i
  %conv.i63.i.i = zext i64 %1 to i128
  %add.neg.i.i.i = ashr i128 %sub.i72.i.i, 127
  %sub.i65.i.i = sub nsw i128 %add.neg.i.i.i, %conv.i63.i.i
  %sub.i65.lobit.i.i = ashr i128 %sub.i65.i.i, 127
  %conv1.i58.i.i = and i128 %sub.i65.lobit.i.i, 18446744073709551615
  %add3.i59.i.i = add nuw nsw i128 %conv1.i58.i.i, 0
  %conv4.i60.i.i = trunc i128 %add3.i59.i.i to i64
  store i64 %conv4.i60.i.i, i64* undef, align 16
  br label %fe_cmovznz.exit.i534.i.15
}

