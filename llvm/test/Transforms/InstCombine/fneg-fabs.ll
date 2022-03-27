; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instcombine -S | FileCheck %s

; (X <  +/-0.0) ? X : -X --> -fabs(X)
; (X <= +/-0.0) ? X : -X --> -fabs(X)
; One negative test with no fmf
define double @select_noFMF_nfabs_lt(double %x) {
; CHECK-LABEL: @select_noFMF_nfabs_lt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp olt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp olt double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select i1 %cmp, double %x, double %negX
  ret double %sel
}

; One test where the neg has fmfs.
define double @select_nsz_nfabs_lt_fmfProp(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_lt_fmfProp(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp olt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg fast double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp olt double %x, 0.000000e+00
  %negX = fneg fast double %x
  %sel = select nsz i1 %cmp, double %x, double %negX
  ret double %sel
}

; Tests with various predicate types.
define double @select_nsz_nfabs_olt(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_olt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp olt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp olt double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %x, double %negX
  ret double %sel
}

define double @select_nsz_nfabs_ult(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_ult(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ult double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ult double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %x, double %negX
  ret double %sel
}

define double @select_nsz_nfabs_ole(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_ole(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ole double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ole double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %x, double %negX
  ret double %sel
}

define double @select_nsz_nfabs_ule(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_ule(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ule double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ule double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %x, double %negX
  ret double %sel
}

; (X >  +/-0.0) ? -X : X --> -fabs(X)
; (X >= +/-0.0) ? -X : X --> -fabs(X)
; One negative test with no fmf
define double @select_noFMF_nfabs_gt(double %x) {
; CHECK-LABEL: @select_noFMF_nfabs_gt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ogt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], double [[NEGX]], double [[X]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ogt double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select i1 %cmp, double %negX, double %x
  ret double %sel
}

; One test where the neg has fmfs.
define double @select_nsz_nfabs_gt_fmfProp(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_gt_fmfProp(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ogt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg fast double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[NEGX]], double [[X]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ogt double %x, 0.000000e+00
  %negX = fneg fast double %x
  %sel = select nsz i1 %cmp, double %negX, double %x
  ret double %sel
}

; Tests with various predicate types.
define double @select_nsz_nfabs_ogt(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_ogt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ogt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[NEGX]], double [[X]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ogt double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %negX, double %x
  ret double %sel
}

define double @select_nsz_nfabs_ugt(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_ugt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ugt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[NEGX]], double [[X]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp ugt double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %negX, double %x
  ret double %sel
}

define double @select_nsz_nfabs_oge(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_oge(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp oge double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[NEGX]], double [[X]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp oge double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %negX, double %x
  ret double %sel
}

define double @select_nsz_nfabs_uge(double %x) {
; CHECK-LABEL: @select_nsz_nfabs_uge(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp uge double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg double [[X]]
; CHECK-NEXT:    [[SEL:%.*]] = select nsz i1 [[CMP]], double [[NEGX]], double [[X]]
; CHECK-NEXT:    ret double [[SEL]]
;
  %cmp = fcmp uge double %x, 0.000000e+00
  %negX = fneg double %x
  %sel = select nsz i1 %cmp, double %negX, double %x
  ret double %sel
}

; (X < +/-0.0) ? X : (0.0 - X) --> (0.0 - fabs(X))
; One negative test with <=.
define double @select_noFMF_fsubfabs_le(double %x) {
; CHECK-LABEL: @select_noFMF_fsubfabs_le(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ole double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[SUB:%.*]] = fsub double 0.000000e+00, [[X]]
; CHECK-NEXT:    [[RETVAL_0:%.*]] = select i1 [[CMP]], double [[X]], double [[SUB]]
; CHECK-NEXT:    ret double [[RETVAL_0]]
;
  %cmp = fcmp ole double %x, 0.000000e+00
  %sub = fsub double 0.000000e+00, %x
  %retval.0 = select i1 %cmp, double %x, double %sub
  ret double %retval.0
}

define double @select_noFMF_fsubfabs_olt(double %x) {
; CHECK-LABEL: @select_noFMF_fsubfabs_olt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp olt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[SUB:%.*]] = fsub double 0.000000e+00, [[X]]
; CHECK-NEXT:    [[RETVAL_0:%.*]] = select i1 [[CMP]], double [[X]], double [[SUB]]
; CHECK-NEXT:    ret double [[RETVAL_0]]
;
  %cmp = fcmp olt double %x, 0.000000e+00
  %sub = fsub double 0.000000e+00, %x
  %retval.0 = select i1 %cmp, double %x, double %sub
  ret double %retval.0
}

define double @select_noFMF_fsubfabs_ult(double %x) {
; CHECK-LABEL: @select_noFMF_fsubfabs_ult(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ult double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[SUB:%.*]] = fsub double 0.000000e+00, [[X]]
; CHECK-NEXT:    [[RETVAL_0:%.*]] = select i1 [[CMP]], double [[X]], double [[SUB]]
; CHECK-NEXT:    ret double [[RETVAL_0]]
;
  %cmp = fcmp ult double %x, 0.000000e+00
  %sub = fsub double 0.000000e+00, %x
  %retval.0 = select i1 %cmp, double %x, double %sub
  ret double %retval.0
}


; With nsz:
; (X < +/-0.0) ? X : -X --> -fabs(X)
define double @select_nsz_fnegfabs_olt(double %x) {
; CHECK-LABEL: @select_nsz_fnegfabs_olt(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp olt double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg nsz double [[X]]
; CHECK-NEXT:    [[RETVAL_0:%.*]] = select i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[RETVAL_0]]
;
  %cmp = fcmp olt double %x, 0.000000e+00
  %negX = fneg nsz double %x
  %retval.0 = select i1 %cmp, double %x, double %negX
  ret double %retval.0
}

define double @select_nsz_fnegfabs_ult(double %x) {
; CHECK-LABEL: @select_nsz_fnegfabs_ult(
; CHECK-NEXT:    [[CMP:%.*]] = fcmp ult double [[X:%.*]], 0.000000e+00
; CHECK-NEXT:    [[NEGX:%.*]] = fneg nsz double [[X]]
; CHECK-NEXT:    [[RETVAL_0:%.*]] = select i1 [[CMP]], double [[X]], double [[NEGX]]
; CHECK-NEXT:    ret double [[RETVAL_0]]
;
  %cmp = fcmp ult double %x, 0.000000e+00
  %negX = fneg nsz double %x
  %retval.0 = select i1 %cmp, double %x, double %negX
  ret double %retval.0
}
