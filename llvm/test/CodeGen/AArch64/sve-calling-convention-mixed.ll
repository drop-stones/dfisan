; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple aarch64-linux-gnu -mattr=+sve | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Make sure callers set up the arguments correctly - tests AArch64ISelLowering::LowerCALL

define float @foo1(double* %x0, double* %x1, double* %x2) nounwind {
; CHECK-LABEL: foo1:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    stp x29, x30, [sp, #-16]! // 16-byte Folded Spill
; CHECK-NEXT:    addvl sp, sp, #-4
; CHECK-NEXT:    ptrue p0.b
; CHECK-NEXT:    fmov s0, #1.00000000
; CHECK-NEXT:    ld4d { z1.d, z2.d, z3.d, z4.d }, p0/z, [x0]
; CHECK-NEXT:    ld4d { z16.d, z17.d, z18.d, z19.d }, p0/z, [x1]
; CHECK-NEXT:    ld1d { z5.d }, p0/z, [x2]
; CHECK-NEXT:    mov x0, sp
; CHECK-NEXT:    ptrue p0.d
; CHECK-NEXT:    st1d { z16.d }, p0, [sp]
; CHECK-NEXT:    st1d { z17.d }, p0, [sp, #1, mul vl]
; CHECK-NEXT:    st1d { z18.d }, p0, [sp, #2, mul vl]
; CHECK-NEXT:    st1d { z19.d }, p0, [sp, #3, mul vl]
; CHECK-NEXT:    bl callee1
; CHECK-NEXT:    addvl sp, sp, #4
; CHECK-NEXT:    ldp x29, x30, [sp], #16 // 16-byte Folded Reload
; CHECK-NEXT:    ret
entry:
  %0 = call <vscale x 16 x i1> @llvm.aarch64.sve.ptrue.nxv16i1(i32 31)
  %1 = call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> %0)
  %2 = call <vscale x 8 x double> @llvm.aarch64.sve.ld4.nxv8f64.nxv2i1(<vscale x 2 x i1> %1, double* %x0)
  %3 = call <vscale x 8 x double> @llvm.aarch64.sve.ld4.nxv8f64.nxv2i1(<vscale x 2 x i1> %1, double* %x1)
  %4 = call <vscale x 2 x double> @llvm.aarch64.sve.ld1.nxv2f64(<vscale x 2 x i1> %1, double* %x2)
  %call = call float @callee1(float 1.000000e+00, <vscale x 8 x double> %2, <vscale x 8 x double> %3, <vscale x 2 x double> %4)
  ret float %call
}

define float @foo2(double* %x0, double* %x1) nounwind {
; CHECK-LABEL: foo2:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    stp x29, x30, [sp, #-16]! // 16-byte Folded Spill
; CHECK-NEXT:    addvl sp, sp, #-4
; CHECK-NEXT:    sub sp, sp, #16
; CHECK-NEXT:    ptrue p0.b
; CHECK-NEXT:    add x9, sp, #16
; CHECK-NEXT:    ld4d { z1.d, z2.d, z3.d, z4.d }, p0/z, [x0]
; CHECK-NEXT:    ld4d { z16.d, z17.d, z18.d, z19.d }, p0/z, [x1]
; CHECK-NEXT:    ptrue p0.d
; CHECK-NEXT:    add x8, sp, #16
; CHECK-NEXT:    fmov s0, #1.00000000
; CHECK-NEXT:    mov w0, wzr
; CHECK-NEXT:    mov w1, #1
; CHECK-NEXT:    mov w2, #2
; CHECK-NEXT:    st1d { z16.d }, p0, [x9]
; CHECK-NEXT:    add x9, sp, #16
; CHECK-NEXT:    mov w3, #3
; CHECK-NEXT:    mov w4, #4
; CHECK-NEXT:    mov w5, #5
; CHECK-NEXT:    mov w6, #6
; CHECK-NEXT:    st1d { z17.d }, p0, [x9, #1, mul vl]
; CHECK-NEXT:    add x9, sp, #16
; CHECK-NEXT:    mov w7, #7
; CHECK-NEXT:    st1d { z18.d }, p0, [x9, #2, mul vl]
; CHECK-NEXT:    add x9, sp, #16
; CHECK-NEXT:    st1d { z19.d }, p0, [x9, #3, mul vl]
; CHECK-NEXT:    str x8, [sp]
; CHECK-NEXT:    bl callee2
; CHECK-NEXT:    addvl sp, sp, #4
; CHECK-NEXT:    add sp, sp, #16
; CHECK-NEXT:    ldp x29, x30, [sp], #16 // 16-byte Folded Reload
; CHECK-NEXT:    ret
entry:
  %0 = call <vscale x 16 x i1> @llvm.aarch64.sve.ptrue.nxv16i1(i32 31)
  %1 = call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> %0)
  %2 = call <vscale x 8 x double> @llvm.aarch64.sve.ld4.nxv8f64.nxv2i1(<vscale x 2 x i1> %1, double* %x0)
  %3 = call <vscale x 8 x double> @llvm.aarch64.sve.ld4.nxv8f64.nxv2i1(<vscale x 2 x i1> %1, double* %x1)
  %call = call float @callee2(i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, float 1.000000e+00, <vscale x 8 x double> %2, <vscale x 8 x double> %3)
  ret float %call
}

define float @foo3(double* %x0, double* %x1, double* %x2) nounwind {
; CHECK-LABEL: foo3:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    stp x29, x30, [sp, #-16]! // 16-byte Folded Spill
; CHECK-NEXT:    addvl sp, sp, #-3
; CHECK-NEXT:    ptrue p0.b
; CHECK-NEXT:    fmov s0, #1.00000000
; CHECK-NEXT:    ld4d { z2.d, z3.d, z4.d, z5.d }, p0/z, [x0]
; CHECK-NEXT:    ld3d { z16.d, z17.d, z18.d }, p0/z, [x1]
; CHECK-NEXT:    ld1d { z6.d }, p0/z, [x2]
; CHECK-NEXT:    fmov s1, #2.00000000
; CHECK-NEXT:    mov x0, sp
; CHECK-NEXT:    ptrue p0.d
; CHECK-NEXT:    st1d { z16.d }, p0, [sp]
; CHECK-NEXT:    st1d { z17.d }, p0, [sp, #1, mul vl]
; CHECK-NEXT:    st1d { z18.d }, p0, [sp, #2, mul vl]
; CHECK-NEXT:    bl callee3
; CHECK-NEXT:    addvl sp, sp, #3
; CHECK-NEXT:    ldp x29, x30, [sp], #16 // 16-byte Folded Reload
; CHECK-NEXT:    ret
entry:
  %0 = call <vscale x 16 x i1> @llvm.aarch64.sve.ptrue.nxv16i1(i32 31)
  %1 = call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> %0)
  %2 = call <vscale x 8 x double> @llvm.aarch64.sve.ld4.nxv8f64.nxv2i1(<vscale x 2 x i1> %1, double* %x0)
  %3 = call <vscale x 6 x double> @llvm.aarch64.sve.ld3.nxv6f64.nxv2i1(<vscale x 2 x i1> %1, double* %x1)
  %4 = call <vscale x 2 x double> @llvm.aarch64.sve.ld1.nxv2f64(<vscale x 2 x i1> %1, double* %x2)
  %call = call float @callee3(float 1.000000e+00, float 2.000000e+00, <vscale x 8 x double> %2, <vscale x 6 x double> %3, <vscale x 2 x double> %4)
  ret float %call
}

; Make sure callees read the arguments correctly - tests AArch64ISelLowering::LowerFormalArguments

define double @foo4(double %x0, double * %ptr1, double * %ptr2, double * %ptr3, <vscale x 8 x double> %x1, <vscale x 8 x double> %x2, <vscale x 2 x double> %x3) nounwind {
; CHECK-LABEL: foo4:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ptrue p0.d
; CHECK-NEXT:    ld1d { z6.d }, p0/z, [x3, #1, mul vl]
; CHECK-NEXT:    ld1d { z7.d }, p0/z, [x3]
; CHECK-NEXT:    ld1d { z24.d }, p0/z, [x3, #3, mul vl]
; CHECK-NEXT:    ld1d { z25.d }, p0/z, [x3, #2, mul vl]
; CHECK-NEXT:    st1d { z4.d }, p0, [x0, #3, mul vl]
; CHECK-NEXT:    st1d { z3.d }, p0, [x0, #2, mul vl]
; CHECK-NEXT:    st1d { z2.d }, p0, [x0, #1, mul vl]
; CHECK-NEXT:    st1d { z1.d }, p0, [x0]
; CHECK-NEXT:    st1d { z25.d }, p0, [x1, #2, mul vl]
; CHECK-NEXT:    st1d { z24.d }, p0, [x1, #3, mul vl]
; CHECK-NEXT:    st1d { z7.d }, p0, [x1]
; CHECK-NEXT:    st1d { z6.d }, p0, [x1, #1, mul vl]
; CHECK-NEXT:    st1d { z5.d }, p0, [x2]
; CHECK-NEXT:    ret
entry:
  %ptr1.bc = bitcast double * %ptr1 to <vscale x 8 x double> *
  store volatile <vscale x 8 x double> %x1, <vscale x 8 x double>* %ptr1.bc
  %ptr2.bc = bitcast double * %ptr2 to <vscale x 8 x double> *
  store volatile <vscale x 8 x double> %x2, <vscale x 8 x double>* %ptr2.bc
  %ptr3.bc = bitcast double * %ptr3 to <vscale x 2 x double> *
  store volatile <vscale x 2 x double> %x3, <vscale x 2 x double>* %ptr3.bc
  ret double %x0
}

define double @foo5(i32 %i0, i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, double * %ptr1, double * %ptr2, double %x0, <vscale x 8 x double> %x1, <vscale x 8 x double> %x2) nounwind {
; CHECK-LABEL: foo5:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ldr x8, [sp]
; CHECK-NEXT:    ptrue p0.d
; CHECK-NEXT:    ld1d { z5.d }, p0/z, [x8, #1, mul vl]
; CHECK-NEXT:    ld1d { z6.d }, p0/z, [x8]
; CHECK-NEXT:    ld1d { z7.d }, p0/z, [x8, #3, mul vl]
; CHECK-NEXT:    ld1d { z24.d }, p0/z, [x8, #2, mul vl]
; CHECK-NEXT:    st1d { z4.d }, p0, [x6, #3, mul vl]
; CHECK-NEXT:    st1d { z3.d }, p0, [x6, #2, mul vl]
; CHECK-NEXT:    st1d { z2.d }, p0, [x6, #1, mul vl]
; CHECK-NEXT:    st1d { z1.d }, p0, [x6]
; CHECK-NEXT:    st1d { z24.d }, p0, [x7, #2, mul vl]
; CHECK-NEXT:    st1d { z7.d }, p0, [x7, #3, mul vl]
; CHECK-NEXT:    st1d { z6.d }, p0, [x7]
; CHECK-NEXT:    st1d { z5.d }, p0, [x7, #1, mul vl]
; CHECK-NEXT:    ret
entry:
  %ptr1.bc = bitcast double * %ptr1 to <vscale x 8 x double> *
  store volatile <vscale x 8 x double> %x1, <vscale x 8 x double>* %ptr1.bc
  %ptr2.bc = bitcast double * %ptr2 to <vscale x 8 x double> *
  store volatile <vscale x 8 x double> %x2, <vscale x 8 x double>* %ptr2.bc
  ret double %x0
}

define double @foo6(double %x0, double %x1, double * %ptr1, double * %ptr2, <vscale x 8 x double> %x2, <vscale x 6 x double> %x3) nounwind {
; CHECK-LABEL: foo6:
; CHECK:       // %bb.0: // %entry
; CHECK-NEXT:    ptrue p0.d
; CHECK-NEXT:    ld1d { z1.d }, p0/z, [x2]
; CHECK-NEXT:    ld1d { z6.d }, p0/z, [x2, #2, mul vl]
; CHECK-NEXT:    ld1d { z7.d }, p0/z, [x2, #1, mul vl]
; CHECK-NEXT:    st1d { z5.d }, p0, [x0, #3, mul vl]
; CHECK-NEXT:    st1d { z4.d }, p0, [x0, #2, mul vl]
; CHECK-NEXT:    st1d { z3.d }, p0, [x0, #1, mul vl]
; CHECK-NEXT:    st1d { z2.d }, p0, [x0]
; CHECK-NEXT:    st1d { z7.d }, p0, [x1, #1, mul vl]
; CHECK-NEXT:    st1d { z6.d }, p0, [x1, #2, mul vl]
; CHECK-NEXT:    st1d { z1.d }, p0, [x1]
; CHECK-NEXT:    ret
entry:
  %ptr1.bc = bitcast double * %ptr1 to <vscale x 8 x double> *
  store volatile <vscale x 8 x double> %x2, <vscale x 8 x double>* %ptr1.bc
  %ptr2.bc = bitcast double * %ptr2 to <vscale x 6 x double> *
  store volatile <vscale x 6 x double> %x3, <vscale x 6 x double>* %ptr2.bc
  ret double %x0
}

declare float @callee1(float, <vscale x 8 x double>, <vscale x 8 x double>, <vscale x 2 x double>)
declare float @callee2(i32, i32, i32, i32, i32, i32, i32, i32, float, <vscale x 8 x double>, <vscale x 8 x double>)
declare float @callee3(float, float, <vscale x 8 x double>, <vscale x 6 x double>, <vscale x 2 x double>)

declare <vscale x 16 x i1> @llvm.aarch64.sve.ptrue.nxv16i1(i32 immarg)
declare <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1>)
declare <vscale x 8 x double> @llvm.aarch64.sve.ld4.nxv8f64.nxv2i1(<vscale x 2 x i1>, double*)
declare <vscale x 6 x double> @llvm.aarch64.sve.ld3.nxv6f64.nxv2i1(<vscale x 2 x i1>, double*)
declare <vscale x 2 x double> @llvm.aarch64.sve.ld1.nxv2f64(<vscale x 2 x i1>, double*)
declare double @llvm.aarch64.sve.faddv.nxv2f64(<vscale x 2 x i1>, <vscale x 2 x double>)
declare <vscale x 2 x double> @llvm.aarch64.sve.tuple.get.nxv2f64.nxv8f64(<vscale x 8 x double>, i32 immarg)
declare <vscale x 2 x double> @llvm.aarch64.sve.tuple.get.nxv2f64.nxv6f64(<vscale x 6 x double>, i32 immarg)
