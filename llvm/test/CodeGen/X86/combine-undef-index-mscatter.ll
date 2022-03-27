; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=avx512f | FileCheck %s

define void @main(<24 x float*> %x)
; CHECK-LABEL: main:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    vmovq %rcx, %xmm0
; CHECK-NEXT:    vmovq %rdx, %xmm1
; CHECK-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; CHECK-NEXT:    vmovq %rsi, %xmm1
; CHECK-NEXT:    vmovq %rdi, %xmm2
; CHECK-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm2[0],xmm1[0]
; CHECK-NEXT:    vinserti128 $1, %xmm0, %ymm1, %ymm0
; CHECK-NEXT:    vmovq %r9, %xmm1
; CHECK-NEXT:    vmovq %r8, %xmm2
; CHECK-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm2[0],xmm1[0]
; CHECK-NEXT:    vinserti128 $1, {{[0-9]+}}(%rsp), %ymm1, %ymm1
; CHECK-NEXT:    vinserti64x4 $1, %ymm1, %zmm0, %zmm0
; CHECK-NEXT:    vmovups {{[0-9]+}}(%rsp), %zmm1
; CHECK-NEXT:    vmovups {{[0-9]+}}(%rsp), %zmm2
; CHECK-NEXT:    kxnorw %k0, %k0, %k1
; CHECK-NEXT:    vbroadcastf128 {{.*#+}} ymm3 = [8.33005607E-1,8.435871E-1,1.69435993E-1,8.33005607E-1,8.33005607E-1,8.435871E-1,1.69435993E-1,8.33005607E-1]
; CHECK-NEXT:    # ymm3 = mem[0,1,0,1]
; CHECK-NEXT:    kxnorw %k0, %k0, %k2
; CHECK-NEXT:    vscatterqps %ymm3, (,%zmm0) {%k2}
; CHECK-NEXT:    kxnorw %k0, %k0, %k2
; CHECK-NEXT:    vscatterqps %ymm3, (,%zmm2) {%k2}
; CHECK-NEXT:    vscatterqps %ymm3, (,%zmm1) {%k1}
; CHECK-NEXT:    vzeroupper
; CHECK-NEXT:    retq
{
entry:
  call void @llvm.masked.scatter.v24f32.v24p0f32(<24 x float> <float 0x3FEAA7FB60000000, float 0x3FEAFEAA60000000, float 0x3FC5B01420000000, float 0x3FEAA7FB60000000, float 0x3FEAA7FB60000000, float 0x3FEAFEAA60000000, float 0x3FC5B01420000000, float 0x3FEAA7FB60000000, float 0x3FEAA7FB60000000, float 0x3FEAFEAA60000000, float 0x3FC5B01420000000, float 0x3FEAA7FB60000000, float 0x3FEAA7FB60000000, float 0x3FEAFEAA60000000, float 0x3FC5B01420000000, float 0x3FEAA7FB60000000, float 0x3FEAA7FB60000000, float 0x3FEAFEAA60000000, float 0x3FC5B01420000000, float 0x3FEAA7FB60000000, float 0x3FEAA7FB60000000, float 0x3FEAFEAA60000000, float 0x3FC5B01420000000, float 0x3FEAA7FB60000000>, <24 x float*> %x, i32 4, <24 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  ret void
}

declare void @llvm.masked.scatter.v24f32.v24p0f32(<24 x float>, <24 x float*>, i32 immarg, <24 x i1>)
