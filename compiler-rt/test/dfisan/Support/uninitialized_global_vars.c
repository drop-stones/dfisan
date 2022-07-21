// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage uninitialized global variables correctly.

char  c = 'a';
short s = 100;
int   i;
long  l;
long double ld;
__float128 f128;

void accessGlobalVars() {
  // Load global vars
  c;
  s;    // ^ initialized
  i;    // v uninitialized
  l;
  ld;
  f128;

  // Write global vars
  c = 'b';
  s = 200;
  i = 300;
  l = 400;
  ld = 1.23;
  f128 = 4.56;
}

int main(int argc, char **argv) {
  c;
  s;    // ^ initialized
  i;    // v uninitialized
  l;
  ld;
  f128;

  accessGlobalVars();

  c;
  s;
  i;
  l;
  ld;
  f128;

  return 0;
}