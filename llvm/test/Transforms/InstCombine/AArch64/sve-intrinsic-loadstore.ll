; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -instcombine < %s | FileCheck %s

target triple = "aarch64-unknown-linux-gnu"

define <vscale x 4 x i32> @combine_ld1(i32* %ptr) #0 {
; CHECK-LABEL: @combine_ld1(
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i32* [[PTR:%.*]] to <vscale x 4 x i32>*
; CHECK-NEXT:    [[TMP2:%.*]] = load <vscale x 4 x i32>, <vscale x 4 x i32>* [[TMP1]], align 16
; CHECK-NEXT:    ret <vscale x 4 x i32> [[TMP2]]
;
  %1 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 31)
  %2 = call <vscale x 4 x i32> @llvm.aarch64.sve.ld1.nxv4i32(<vscale x 4 x i1> %1, i32* %ptr)
  ret <vscale x 4 x i32> %2
}

define <vscale x 4 x i32> @combine_ld1_casted_predicate(i32* %ptr) #0 {
; CHECK-LABEL: @combine_ld1_casted_predicate(
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i32* [[PTR:%.*]] to <vscale x 4 x i32>*
; CHECK-NEXT:    [[TMP2:%.*]] = load <vscale x 4 x i32>, <vscale x 4 x i32>* [[TMP1]], align 16
; CHECK-NEXT:    ret <vscale x 4 x i32> [[TMP2]]
;
  %1 = tail call <vscale x 8 x i1> @llvm.aarch64.sve.ptrue.nxv8i1(i32 31)
  %2 = tail call <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv8i1(<vscale x 8 x i1> %1)
  %3 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> %2)
  %4 = call <vscale x 4 x i32> @llvm.aarch64.sve.ld1.nxv4i32(<vscale x 4 x i1> %3, i32* %ptr)
  ret <vscale x 4 x i32> %4
}

define <vscale x 4 x i32> @combine_ld1_masked(i32* %ptr) #0 {
; CHECK-LABEL: @combine_ld1_masked(
; CHECK-NEXT:    [[TMP1:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 16)
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i32* [[PTR:%.*]] to <vscale x 4 x i32>*
; CHECK-NEXT:    [[TMP3:%.*]] = call <vscale x 4 x i32> @llvm.masked.load.nxv4i32.p0nxv4i32(<vscale x 4 x i32>* [[TMP2]], i32 1, <vscale x 4 x i1> [[TMP1]], <vscale x 4 x i32> zeroinitializer)
; CHECK-NEXT:    ret <vscale x 4 x i32> [[TMP3]]
;
  %1 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 16)
  %2 = call <vscale x 4 x i32> @llvm.aarch64.sve.ld1.nxv4i32(<vscale x 4 x i1> %1, i32* %ptr)
  ret <vscale x 4 x i32> %2
}

define <vscale x 8 x i16> @combine_ld1_masked_casted_predicate(i16* %ptr) #0 {
; CHECK-LABEL: @combine_ld1_masked_casted_predicate(
; CHECK-NEXT:    [[TMP1:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 31)
; CHECK-NEXT:    [[TMP2:%.*]] = tail call <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv4i1(<vscale x 4 x i1> [[TMP1]])
; CHECK-NEXT:    [[TMP3:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[TMP2]])
; CHECK-NEXT:    [[TMP4:%.*]] = bitcast i16* [[PTR:%.*]] to <vscale x 8 x i16>*
; CHECK-NEXT:    [[TMP5:%.*]] = call <vscale x 8 x i16> @llvm.masked.load.nxv8i16.p0nxv8i16(<vscale x 8 x i16>* [[TMP4]], i32 1, <vscale x 8 x i1> [[TMP3]], <vscale x 8 x i16> zeroinitializer)
; CHECK-NEXT:    ret <vscale x 8 x i16> [[TMP5]]
;
  %1 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 31)
  %2 = tail call <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv4i1(<vscale x 4 x i1> %1)
  %3 = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> %2)
  %4 = call <vscale x 8 x i16> @llvm.aarch64.sve.ld1.nxv8i16(<vscale x 8 x i1> %3, i16* %ptr)
  ret <vscale x 8 x i16> %4
}

define void @combine_st1(<vscale x 4 x i32> %vec, i32* %ptr) #0 {
; CHECK-LABEL: @combine_st1(
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i32* [[PTR:%.*]] to <vscale x 4 x i32>*
; CHECK-NEXT:    store <vscale x 4 x i32> [[VEC:%.*]], <vscale x 4 x i32>* [[TMP1]], align 16
; CHECK-NEXT:    ret void
;
  %1 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 31)
  call void @llvm.aarch64.sve.st1.nxv4i32(<vscale x 4 x i32> %vec, <vscale x 4 x i1> %1, i32* %ptr)
  ret void
}

define void @combine_st1_casted_predicate(<vscale x 4 x i32> %vec, i32* %ptr) #0 {
; CHECK-LABEL: @combine_st1_casted_predicate(
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i32* [[PTR:%.*]] to <vscale x 4 x i32>*
; CHECK-NEXT:    store <vscale x 4 x i32> [[VEC:%.*]], <vscale x 4 x i32>* [[TMP1]], align 16
; CHECK-NEXT:    ret void
;
  %1 = tail call <vscale x 8 x i1> @llvm.aarch64.sve.ptrue.nxv8i1(i32 31)
  %2 = tail call <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv8i1(<vscale x 8 x i1> %1)
  %3 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> %2)
  call void @llvm.aarch64.sve.st1.nxv4i32(<vscale x 4 x i32> %vec, <vscale x 4 x i1> %3, i32* %ptr)
  ret void
}

define void @combine_st1_masked(<vscale x 4 x i32> %vec, i32* %ptr) #0 {
; CHECK-LABEL: @combine_st1_masked(
; CHECK-NEXT:    [[TMP1:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 16)
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i32* [[PTR:%.*]] to <vscale x 4 x i32>*
; CHECK-NEXT:    call void @llvm.masked.store.nxv4i32.p0nxv4i32(<vscale x 4 x i32> [[VEC:%.*]], <vscale x 4 x i32>* [[TMP2]], i32 1, <vscale x 4 x i1> [[TMP1]])
; CHECK-NEXT:    ret void
;
  %1 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 16)
  call void @llvm.aarch64.sve.st1.nxv4i32(<vscale x 4 x i32> %vec, <vscale x 4 x i1> %1, i32* %ptr)
  ret void
}

define void @combine_st1_masked_casted_predicate(<vscale x 8 x i16> %vec, i16* %ptr) #0 {
; CHECK-LABEL: @combine_st1_masked_casted_predicate(
; CHECK-NEXT:    [[TMP1:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 31)
; CHECK-NEXT:    [[TMP2:%.*]] = tail call <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv4i1(<vscale x 4 x i1> [[TMP1]])
; CHECK-NEXT:    [[TMP3:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[TMP2]])
; CHECK-NEXT:    [[TMP4:%.*]] = bitcast i16* [[PTR:%.*]] to <vscale x 8 x i16>*
; CHECK-NEXT:    call void @llvm.masked.store.nxv8i16.p0nxv8i16(<vscale x 8 x i16> [[VEC:%.*]], <vscale x 8 x i16>* [[TMP4]], i32 1, <vscale x 8 x i1> [[TMP3]])
; CHECK-NEXT:    ret void
;
  %1 = tail call <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32 31)
  %2 = tail call <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv4i1(<vscale x 4 x i1> %1)
  %3 = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> %2)
  call void @llvm.aarch64.sve.st1.nxv8i16(<vscale x 8 x i16> %vec, <vscale x 8 x i1> %3, i16* %ptr)
  ret void
}

declare <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1>)
declare <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1>)

declare <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv4i1(<vscale x 4 x i1>)
declare <vscale x 16 x i1> @llvm.aarch64.sve.convert.to.svbool.nxv8i1(<vscale x 8 x i1>)

declare <vscale x 4 x i32> @llvm.aarch64.sve.ld1.nxv4i32(<vscale x 4 x i1>, i32*)
declare <vscale x 8 x i16> @llvm.aarch64.sve.ld1.nxv8i16(<vscale x 8 x i1>, i16*)

declare <vscale x 4 x i1> @llvm.aarch64.sve.ptrue.nxv4i1(i32)
declare <vscale x 8 x i1> @llvm.aarch64.sve.ptrue.nxv8i1(i32)
declare <vscale x 16 x i1> @llvm.aarch64.sve.ptrue.nxv16i1(i32)

declare void @llvm.aarch64.sve.st1.nxv4i32(<vscale x 4 x i32>, <vscale x 4 x i1>, i32*)
declare void @llvm.aarch64.sve.st1.nxv8i16(<vscale x 8 x i16>, <vscale x 8 x i1>, i16*)

attributes #0 = { "target-features"="+sve" }
