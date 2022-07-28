// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test from 401.bzip2 in SPEC CPU2006.

static __inline__
char mmed3(char a, char b, char c) {
  char t;
  if (a > b) { t = a; a = b; b = t; }
  if (b > c) {
    b = c;
    if (a > b) b = a;
  }
  return b;
}

int main(void) {
  int med = (int)mmed3('a', 'b', 'c');
  med;

  return 0;
}
