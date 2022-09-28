// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect overread of heap object.

#include <stdio.h>
#include <stdlib.h>
#include "../safe_alloc.h"

int main(int argc, char **argv) {
  int *arr = (int *)safe_malloc(sizeof(int) * 10);   // 0x__3c0
  int *p = (int *)safe_malloc(sizeof(int));          // 0x__3f0

  arr[0] = 0;
  *p = 100;

  printf("p = %p, *p = %d\n", (void *)p, *p);
  printf("arr = %p, arr[0] = %d\n", (void *)arr, arr[0]);
  printf("arr[12] = %d\n", arr[12]);    // Error: read '*p' via 'arr'

  free(arr);
  free(p);

  return 0;
}
