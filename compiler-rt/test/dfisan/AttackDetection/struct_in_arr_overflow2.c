// RUN: %clang_dfisan -mllvm -protect-all -mllvm -no-check-unsafe-access %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Overflow from arr[i].i1 to arr[i+1].i1
// but dfisan cannot detect the error because of element insensitivity

#include <string.h>

struct Element {
  int i1[1];
  int i2[1];
};

int main(void) {
  struct Element arr[8];
  for (int i = 0; i < 7; i++) {
    arr[i].i1[2] = i;   // Overflow
    arr[i].i2[2] = i;   // Overflow
  }

  arr[1].i1[0];

  return 0;
}
