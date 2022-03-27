; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-darwin | FileCheck %s
;
; Test the add and load are folded into the store instruction.

@X = internal global i16 0              ; <i16*> [#uses=2]

define void @foo() nounwind {
; CHECK-LABEL: foo:
; CHECK:       ## %bb.0:
; CHECK-NEXT:    addw $329, _X ## imm = 0x149
; CHECK-NEXT:    retl
  %tmp.0 = load i16, i16* @X           ; <i16> [#uses=1]
  %tmp.3 = add i16 %tmp.0, 329            ; <i16> [#uses=1]
  store i16 %tmp.3, i16* @X
  ret void
}

; rdar://12838504
%struct.S2 = type { i64, i16, [2 x i8], i8, [3 x i8], [7 x i8], i8, [8 x i8] }
@s2 = external global %struct.S2, align 16
define void @test2() nounwind uwtable ssp {
; CHECK-LABEL: test2:
; CHECK:       ## %bb.0:
; CHECK-NEXT:    movl L_s2$non_lazy_ptr, %eax
; CHECK-NEXT:    andl $-262144, 20(%eax) ## imm = 0xFFFC0000
; CHECK-NEXT:    retl
  %bf.load35 = load i56, i56* bitcast ([7 x i8]* getelementptr inbounds (%struct.S2, %struct.S2* @s2, i32 0, i32 5) to i56*), align 16
  %bf.clear36 = and i56 %bf.load35, -1125895611875329
  store i56 %bf.clear36, i56* bitcast ([7 x i8]* getelementptr inbounds (%struct.S2, %struct.S2* @s2, i32 0, i32 5) to i56*), align 16
  ret void
}
