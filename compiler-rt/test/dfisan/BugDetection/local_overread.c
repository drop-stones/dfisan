// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect overread of local variable.

#include <stdio.h>

int main(int argc, char **argv) {
  int x __attribute__((annotate("dfi_protection"))) = 100;
  int arr[8] __attribute__((annotate("dfi_protection")));

  for (int i = 0; i < 8; i++)
    arr[i] = i;
  
  printf("x = %d\n", x);              // OK!
  printf("arr[12] = %d\n", arr[12]);  // Error: read 'x'

  return 0;
}
