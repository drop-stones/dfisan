// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// TODO: Support union by alias analysis

struct Bytes {
  char c1;
  char c2;
  char c3;
  char c4;
};

union Int {
  int i;
  struct Bytes bytes;
};

int main(void) {
  union Int i __attribute__((annotate("dfi_protection")));

  // DEF
  i.i = 100;
  // USE
  i.i;
  i.bytes.c1;
  i.bytes.c2;
  i.bytes.c3;
  i.bytes.c4;

  // DEF
  i.bytes.c1 = 'a';
  // i.bytes.c2 = 'b';
  // i.bytes.c3 = 'c';
  // i.bytes.c4 = 'd';
  // USE
  i.i;
  i.bytes.c1;
  i.bytes.c2;
  i.bytes.c3;
  i.bytes.c4;

  return 0;
}
