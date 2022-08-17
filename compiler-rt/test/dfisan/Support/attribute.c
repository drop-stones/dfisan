// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for selective dfisan attributes.

#include <stdlib.h>

int g1 __attribute__((annotate("dfi_protection")));
int g2 __attribute__((annotate("dfi_protection")));

int g3;

int main(void) {
  int l1 __attribute__((annotate("dfi_protection")));
  int l2 __attribute__((annotate("dfi_protection")));

  g1 = 100;
  g2 = 200;
  l1 = 300;
  l2 = 400;

  g1;
  g2;
  l1;
  l2;

  int l3;
  g3 = 500;
  l3 = 600;
  g3;       // no checked
  l3;       // no checked


  return 0;
}
