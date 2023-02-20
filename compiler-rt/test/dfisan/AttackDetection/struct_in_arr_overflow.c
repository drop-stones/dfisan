// RUN: %clang_dfisan -mllvm -protect-all -mllvm -no-check-unsafe-access %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Overflow from arr[i].str to arr[i].id

#include <string.h>

struct Element {
  char str[8];
  int id;
};

int main(void) {
  struct Element arr[8];
  for (int i = 0; i < 8; i++) {
    arr[i].id = i;
    memcpy(arr[i].str, "aaaaaaaaaa", 10);   // Overflow
  }

  arr[0].id;

  return 0;
}
