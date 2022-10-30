// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage calloc().

#include "../../safe_alloc.h"

#define SIZE 144

int main(void) {
  int *arr = (int *)safe_calloc(SIZE, sizeof(int)); // DEF
  arr[0] = 100;                                     // DEF
  for (int i = 0; i < SIZE; i++)
    arr[i];                                         // USE

  return 0;
}
