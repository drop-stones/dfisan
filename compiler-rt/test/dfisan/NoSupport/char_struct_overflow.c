// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in char.
// TODO: Remove DefUse edges of out-of-bounds in DG.

struct CharSet {
  char c0[1];
  char c1;
};

int main(int argc, char **argv) {
  struct CharSet cs;
  cs.c1 = 'a';
  for (int i = 0; i < 8; i++)
    cs.c0[i] = 'b';

  cs.c1;
  return 0;
}