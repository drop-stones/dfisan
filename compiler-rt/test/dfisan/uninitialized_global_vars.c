// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage uninitialized global variables correctly.

char  c = 'a';
short s = 100;
int   i;
long  l;

int main(int argc, char **argv) {
  c;
  s;
  i;
  l;

  return 0;
}