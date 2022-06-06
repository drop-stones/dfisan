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

struct S3 {
  unsigned int ui;
  struct S2 s2;
};

struct S0 g0;
struct S1 g1 = { 100, 200 };
struct S2 g2_0;
struct S2 g2_1 = { {'a', 300}, {400, 500}, 1.23, 4.56 };
struct S3 g3_0;
struct S3 g3_1 = { 600, { {'b', 700}, {800, 900}, 7.89, 10.11 } };

int main(void) {
  g0.c;
  g0.s;

  g1.i;
  g1.l;

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

  g3_0.ui;
  g3_0.s2.s0.c;
  g3_0.s2.s0.s;
  g3_0.s2.s1.i;
  g3_0.s2.s1.l;
  g3_0.s2.f;
  g3_0.s2.d;

  g3_1.ui;
  g3_1.s2.s0.c;
  g3_1.s2.s0.s;
  g3_1.s2.s1.i;
  g3_1.s2.s1.l;
  g3_1.s2.f;
  g3_1.s2.d;

  return 0;
}