; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -consthoist -consthoist-gep -S -o - %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"
target triple = "thumbv6m-none--musleabi"

%0 = type { [10 x i32], [10 x i16] }

@global = external local_unnamed_addr global %0, align 4

define void @test_inbounds() {
; CHECK-LABEL: @test_inbounds(
; CHECK-NEXT:  bb:
; CHECK-NEXT:    [[CONST:%.*]] = bitcast i16* getelementptr inbounds ([[TMP0:%.*]], %0* @global, i32 0, i32 1, i32 0) to i16*
; CHECK-NEXT:    store i16 undef, i16* [[CONST]], align 2
; CHECK-NEXT:    [[BASE_BITCAST:%.*]] = bitcast i16* [[CONST]] to i8*
; CHECK-NEXT:    [[MAT_GEP:%.*]] = getelementptr i8, i8* [[BASE_BITCAST]], i32 2
; CHECK-NEXT:    [[MAT_BITCAST:%.*]] = bitcast i8* [[MAT_GEP]] to i16*
; CHECK-NEXT:    store i16 undef, i16* [[MAT_BITCAST]], align 2
; CHECK-NEXT:    [[BASE_BITCAST1:%.*]] = bitcast i16* [[CONST]] to i8*
; CHECK-NEXT:    [[MAT_GEP2:%.*]] = getelementptr i8, i8* [[BASE_BITCAST1]], i32 20
; CHECK-NEXT:    [[MAT_BITCAST3:%.*]] = bitcast i8* [[MAT_GEP2]] to i16*
; CHECK-NEXT:    store i16 undef, i16* [[MAT_BITCAST3]], align 2
; CHECK-NEXT:    ret void
;
bb:
  store i16 undef, i16* getelementptr inbounds (%0, %0* @global, i32 0, i32 1, i32 0)
  store i16 undef, i16* getelementptr inbounds (%0, %0* @global, i32 0, i32 1, i32 1)
  store i16 undef, i16* getelementptr inbounds (%0, %0* @global, i32 0, i32 1, i32 10)
  ret void
}

define dso_local void @test_non_inbounds() {
; CHECK-LABEL: @test_non_inbounds(
; CHECK-NEXT:  bb:
; CHECK-NEXT:    store i16 undef, i16* getelementptr inbounds ([[TMP0:%.*]], %0* @global, i32 0, i32 1, i32 11), align 2
; CHECK-NEXT:    store i16 undef, i16* getelementptr ([[TMP0]], %0* @global, i32 0, i32 1, i32 12), align 2
; CHECK-NEXT:    ret void
;
bb:
  store i16 undef, i16* getelementptr inbounds (%0, %0* @global, i32 0, i32 1, i32 11)
  store i16 undef, i16* getelementptr (%0, %0* @global, i32 0, i32 1, i32 12)
  ret void
}

