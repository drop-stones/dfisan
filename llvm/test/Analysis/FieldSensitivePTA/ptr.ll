; RUN: opt %s -passes=print-field-sensitive-pta -disable-output 2>&1 | grep -e "FieldSensitivePTA/ptr.ll"

define dso_local i32 @foo() #0 {
  ret i32 5
}
