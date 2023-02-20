// RUN: %clang_dfisan -mllvm -protect-all -mllvm -no-check-unsafe-access %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Overread from s.n1.i1[0] to s.n2.i2[0]

struct Nested1 {
  int i1[1];
  int i2[1];
};

struct Nested2 {
  int i1[1];
  int i2[1];
};

struct S {
  int i[1];
  struct Nested1 n1;
  struct Nested2 n2;
};

int main(void) {
  struct S s;
  s.i[0]     = 100;
  s.n1.i1[0] = 200;
  s.n1.i2[0] = 300;
  s.n2.i1[0] = 400;
  s.n2.i2[0] = 500;

  s.n1.i1[3];   // Overread

  return 0;
}
