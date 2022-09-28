// RUN: %clang_dfisan %s -o %t
// RUN: %run %t %s
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can support sscanf().

#include <stdio.h>

int main(void) {
  char str[] = "100 200 300";
  char format[] = "%d %d %d";

  int a __attribute__((annotate("dfi_protection")));
  int b __attribute__((annotate("dfi_protection")));
  int c __attribute__((annotate("dfi_protection")));
  sscanf(str, format, &a, &b, &c);

  a; b; c;

  return 0;
}
