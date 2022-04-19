// RUN: %clang_dfisan %s -o %t && ! %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in heap struct object.

#include <stdlib.h>

#define SIZE 8

struct Array {
  int arr[SIZE];
  int id;
};

int main(int argc, char **argv) {
  struct Array *a = (struct Array *)malloc(sizeof(struct Array));
  a->id = 100;
  for (int i = 0; i < SIZE + 1; i++)
    a->arr[i] = i;
  a->id;      // Error: read broken 'a->id'
  return 0;
}