// RUN: %clang_dfisan -mllvm -protect-all -mllvm -no-check-unsafe-access %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Overflow from s.n1.str[8] to s.n2.l

struct Nested1 {
  char str[8];
  int i;
  float f;
};

struct Nested2 {
  long l;
  double d;
};

struct S {
  struct Nested1 n1;
  struct Nested2 n2;
};

int main(void) {
  struct S s = { { "", 100, 200 }, { 300, 400 } };

  for (int i = 0; i < 32; i++)
    s.n1.str[i] = 'a';
  
  s.n2.l;

  return 0;
}
