// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in heap struct object.
// TODO: Remove out-of-bounds DefUse edges in DG.

#include <stdlib.h>

#define SIZE 8

struct Array {
  char arr[SIZE];
  int id;
};

int main(int argc, char **argv) {
  struct Array *a = (struct Array *)malloc(sizeof(struct Array));
  a->id = 100;
  for (int i = 0; i < SIZE + 1; i++)
    a->arr[i] = 'a';
  a->id;      // Error: read broken 'a->id'
  return 0;
}