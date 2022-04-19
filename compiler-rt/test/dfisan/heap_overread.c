// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect overread of heap object.

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int *arr = (int *)malloc(sizeof(int) * 10);   // 0x__300
  int *p = (int *)malloc(sizeof(int));          // 0x__2d0

  *p = 100;
  arr[0] = 0;

  printf("p = %p, *p = %d\n", (void *)p, *p);
  printf("arr = %p, arr[0] = %d\n", (void *)arr, arr[0]);
  printf("arr[12] = %d\n", arr[12]);    // Error: read '*p' via 'arr'

  free(p);
  free(arr);

  return 0;
}