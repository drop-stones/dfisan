// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage global pointer initializations.
// TODO: Support global initializations.

struct S {
  int *p;
  long *q;
};

int i = 100;
long l = 200;

int *r = &i;
long *s = &l;
struct S st = { &i, &l };

int main(void) {
  i;
  l;
  r;
  s;
  *r;
  *s;
  st;
  st.p;
  st.q;
  *st.p;
  *st.q;

  return 0;
};