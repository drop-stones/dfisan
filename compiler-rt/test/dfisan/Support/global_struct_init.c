// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage struct initialization in local.
// TODO: Support global struct zeroinitializer

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

struct S0 g0;
struct S1 g1 = { 100, 200 };
struct S2 g2_0;
struct S2 g2_1 = { {'a', 300}, {400, 500}, 1.23, 4.56 };

int main(void) {
  g0.c;
  g0.s;

  g1.i;
  g1.l;

  // We cannot check g0 correctly
  g2_0.s0.c;
  g2_0.s0.s;
  g2_0.s1.i;
  g2_0.s1.l;
  g2_0.f;
  g2_0.d;

  g2_1.s0.c;
  g2_1.s0.s;
  g2_1.s1.i;
  g2_1.s1.l;
  g2_1.f;
  g2_1.d;

  return 0;
}