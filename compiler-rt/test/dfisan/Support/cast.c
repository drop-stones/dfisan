// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// TODO: Support cast by alias analysis

struct Bytes {
  char c1;
  char c2;
  char c3;
  char c4;
};

int main(void) {
  int i __attribute__((annotate("dfi_protection"))) = 100;

  struct Bytes *bytes = (struct Bytes *)&i;
  bytes->c1;
  bytes->c2;
  bytes->c3;
  bytes->c4;

  bytes->c1 = 'a';      // OK
  // bytes->c2 = 'b';   // NG
  // bytes->c3 = 'c';
  // bytes->c4 = 'd';
  printf("i = %d\n", i);

  return 0;
}
