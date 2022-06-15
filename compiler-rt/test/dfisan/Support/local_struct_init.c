// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage struct initialization in local.

struct S0 {
  char c;
  short s;
};

struct S1 {
  int i;
  long l;
};

struct S2 {
  struct S0 s0;
  struct S1 s1;
  float f;
  double d;
};

int main(void) {
  struct S2 s2 = { {'a', 100}, {200, 300}, 1.23, 4.56 };

  s2.s0.c;
  s2.s0.s;
  s2.s1.i;
  s2.s1.l;
  s2.f;
  s2.d;

  return 0;
}