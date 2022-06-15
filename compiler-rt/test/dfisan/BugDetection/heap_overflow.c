// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in heap.

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int *arr = (int *)malloc(sizeof(int) * 10);   // 0x__300
  int *p = (int *)malloc(sizeof(int));          // 0x__2d0

  *p = 100;

  printf("p = %p, *p = %d\n", (void *)p, *p);
  printf("arr = %p, arr[0] = %d\n", (void *)arr, arr[0]);

  arr[12] = 200;  // overwrite '*p'

  printf("p = %p, *p = %d\n", (void *)p, *p);   // Error: read broken '*p'
  printf("arr = %p, arr[0] = %d\n", (void *)arr, arr[0]);

  free(p);
  free(arr);

  return 0;
}