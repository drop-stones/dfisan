// RUN: %clang_dfisan %s -o %t
// RUN: %run %t %s
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can support sscanf().

#include <stdio.h>

int main(void) {
  char str[] = "100 200 300";
  char format[] = "%d %d %d";

  int a, b, c;
  sscanf(str, format, &a, &b, &c);

  a;
  b;
  c;

  return 0;
}
