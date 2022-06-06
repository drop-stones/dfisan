// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage array initialization in struct.

struct S0 {
  char str0[8];
  int i;
};

struct S1 {
  struct S0 s0;
  char str1[8];
};

struct S1 g0;
struct S1 g1 = { {"Chris", 200 }, "Dave" };

int main(void) {
  struct S1 s1 = { { "Alice", 100 }, "Bob" };
  s1.s0.str0[0];
  s1.s0.str0[1];
  s1.s0.i;
  s1.str1[0];
  s1.str1[1];

  g0.s0.str0[0];
  g0.s0.str0[4];
  g0.s0.i;
  g0.str1[0];
  g0.str1[4];

  g1.s0.str0[0];
  g1.s0.str0[4];
  g1.s0.i;
  g1.str1[0];
  g1.str1[4];

  return 0;
}