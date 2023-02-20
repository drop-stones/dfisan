// RUN: %clang_dfisan -mllvm -protect-all -mllvm -no-check-unsafe-access %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Overread from arr[0].i1[1] to arr[1].i1[1]
// but dfisan cannot detect the error because of element insensitivity

#include <string.h>

struct Element {
  int i1[1];
  int i2[1];
};

int main(void) {
  struct Element arr[8];
  for (int i = 0; i < 8; i++) {
    arr[i].i1[0] = i;
    arr[i].i2[0] = i;
  }

  arr[0].i1[2];   // Overread

  return 0;
}
