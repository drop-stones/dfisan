// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can call malloc() and free() correctly.

#include <stdlib.h>

#define SIZE 100

int main(int argc, char **argv) {
  int *arr = (int *)malloc(sizeof(int) * SIZE);
  for (int i = 0; i < SIZE; i++) 
    arr[i] = i;
  free(arr);

  return 0;
}
