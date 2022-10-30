// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect incorrect accesses to safe region.

#include <stdio.h>

int main(void) {
  int safe __attribute__((annotate("dfi_protection"))) = 100;
  int *unsafe_ptr = (int *)0x8100003c0;
  printf("safe = %d, &safe = %p, unsafe_ptr = %p\n", safe, (void *)&safe, (void *)unsafe_ptr);

  *unsafe_ptr = 200;
  printf("safe = %d, &safe = %p, unsafe_ptr = %p\n", safe, (void *)&safe, (void *)unsafe_ptr);

  return 0;
}
