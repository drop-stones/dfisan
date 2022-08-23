// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for selective dfisan attributes.

#include <stdlib.h>

int g1 __attribute__((annotate("dfi_protection")));
int g2 __attribute__((annotate("dfi_protection")));
int *g3 __attribute__((annotate("dfi_ptr_protection")));
int *g4 __attribute__((annotate("dfi_protection"))) __attribute__((annotate("dfi_ptr_protection")));

int g5;
int *g6;

int main(void) {
  int l1 __attribute__((annotate("dfi_protection")));
  int l2 __attribute__((annotate("dfi_protection")));
  int *p1 __attribute__((annotate("dfi_ptr_protection")));
  int *p2 __attribute__((annotate("dfi_protection"))) __attribute__((annotate("dfi_ptr_protection")));

  g3 = (int *)malloc(sizeof(int));
  g4 = (int *)malloc(sizeof(int));
  p1 = (int *)malloc(sizeof(int));
  p2 = (int *)malloc(sizeof(int));

  g1 = 100;
  g2 = 200;
  l1 = 300;
  l2 = 400;
  *g3 = 500;
  *g4 = 600;
  *p1 = 700;
  *p2 = 800;

  g1;
  g2;
  l1;
  l2;
  *g3;
  *g4;
  *p1;
  *p2;

  /// No checked
  int l3;
  int *p3 = (int *)malloc(sizeof(int));
  g6 = (int *)malloc(sizeof(int));

  g3 = 900;
  l3 = 1000;
  *g6 = 1100;
  *p3 = 1200;
  g3;
  l3;
  *g6;
  *p3;

  return 0;
}
