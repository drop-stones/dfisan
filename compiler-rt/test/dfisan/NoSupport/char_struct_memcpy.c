// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can ignore padding array in struct.
// TODO: Support memcpy().

struct S {
  char c0;
  char c1;
  char c2;
  char c3;
};

int main(void) {
  struct S from = {'a', 'b', 'c', 'd'};
  struct S to;

  memcpy(&to, &from, sizeof(struct S));

  from.c0;
  from.c1;
  from.c2;
  from.c3;
  to.c0;
  to.c1;
  to.c2;
  to.c3;

  return 0;
}