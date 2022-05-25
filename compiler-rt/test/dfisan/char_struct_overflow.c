// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in char.

struct CharSet {
  char c0[1];
  char c1;
};

int main(int argc, char **argv) {
  struct CharSet cs;
  cs.c1 = 'a';
  cs.c0[4] = 'b';

  cs.c1;
  return 0;
}