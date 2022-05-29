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

struct S2 g0;
struct S2 g1 = { {'a', 100}, {200, 300}, 1.23, 4.56 };

int main(void) {
  g0.s0.c;
  g0.s0.s;
  g0.s1.i;
  g0.s1.l;
  g0.f;
  g0.d;

  g1.s0.c;
  g1.s0.s;
  g1.s1.i;
  g1.s1.l;
  g1.f;
  g1.d;

  return 0;
}