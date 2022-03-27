; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mcpu=pwr9 -mtriple=powerpc64le-unknown-unknown -verify-machineinstrs \
; RUN:   -ppc-asm-full-reg-names -ppc-vsr-nums-as-vr < %s | FileCheck %s
; RUN: llc -mcpu=pwr9 -mtriple=powerpc64-unknown-unknown -verify-machineinstrs \
; RUN:   -ppc-asm-full-reg-names -ppc-vsr-nums-as-vr < %s | \
; RUN:   FileCheck %s --check-prefix=CHECK-BE
; RUN: llc -mcpu=pwr8 -mtriple=powerpc64le-unknown-unknown -verify-machineinstrs \
; RUN:   -ppc-asm-full-reg-names -ppc-vsr-nums-as-vr < %s | FileCheck %s \
; RUN:   -check-prefix=CHECK-P8

; Function Attrs: norecurse nounwind readnone
define i64 @getPart1(fp128 %in) local_unnamed_addr {
; CHECK-LABEL: getPart1:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    mfvsrld r3, v2
; CHECK-NEXT:    blr
;
; CHECK-BE-LABEL: getPart1:
; CHECK-BE:       # %bb.0: # %entry
; CHECK-BE-NEXT:    mfvsrld r3, v2
; CHECK-BE-NEXT:    blr
;
; CHECK-P8-LABEL: getPart1:
; CHECK-P8:       # %bb.0: # %entry
; CHECK-P8-NEXT:    xxswapd vs0, v2
; CHECK-P8-NEXT:    mffprd r3, f0
; CHECK-P8-NEXT:    blr
entry:
  %0 = bitcast fp128 %in to i128
  %a.sroa.0.0.extract.trunc = trunc i128 %0 to i64
  ret i64 %a.sroa.0.0.extract.trunc
}

; Function Attrs: norecurse nounwind readnone
define i64 @getPart2(fp128 %in) local_unnamed_addr {
; CHECK-LABEL: getPart2:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    mfvsrd r3, v2
; CHECK-NEXT:    blr
;
; CHECK-BE-LABEL: getPart2:
; CHECK-BE:       # %bb.0: # %entry
; CHECK-BE-NEXT:    mfvsrd r3, v2
; CHECK-BE-NEXT:    blr
;
; CHECK-P8-LABEL: getPart2:
; CHECK-P8:       # %bb.0: # %entry
; CHECK-P8-NEXT:    mfvsrd r3, v2
; CHECK-P8-NEXT:    blr
entry:
  %0 = bitcast fp128 %in to i128
  %a.sroa.0.8.extract.shift = lshr i128 %0, 64
  %a.sroa.0.8.extract.trunc = trunc i128 %a.sroa.0.8.extract.shift to i64
  ret i64 %a.sroa.0.8.extract.trunc
}

; Function Attrs: norecurse nounwind readnone
define i64 @checkBitcast(fp128 %in, <2 x i64> %in2, <2 x i64> *%out) local_unnamed_addr {
; CHECK-LABEL: checkBitcast:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    mfvsrld r3, v2
; CHECK-NEXT:    vaddudm v2, v2, v3
; CHECK-NEXT:    stxv v2, 0(r7)
; CHECK-NEXT:    blr
;
; CHECK-BE-LABEL: checkBitcast:
; CHECK-BE:       # %bb.0: # %entry
; CHECK-BE-NEXT:    mfvsrd r3, v2
; CHECK-BE-NEXT:    vaddudm v2, v2, v3
; CHECK-BE-NEXT:    stxv v2, 0(r7)
; CHECK-BE-NEXT:    blr
;
; CHECK-P8-LABEL: checkBitcast:
; CHECK-P8:       # %bb.0: # %entry
; CHECK-P8-NEXT:    xxswapd vs0, v2
; CHECK-P8-NEXT:    vaddudm v2, v2, v3
; CHECK-P8-NEXT:    mffprd r3, f0
; CHECK-P8-NEXT:    xxswapd vs0, v2
; CHECK-P8-NEXT:    stxvd2x vs0, 0, r7
; CHECK-P8-NEXT:    blr
entry:
  %0 = bitcast fp128 %in to <2 x i64>
  %1 = extractelement <2 x i64> %0, i64 0
  %2 = add <2 x i64> %0, %in2
  store <2 x i64> %2, <2 x i64> *%out, align 16
  ret i64 %1
}

