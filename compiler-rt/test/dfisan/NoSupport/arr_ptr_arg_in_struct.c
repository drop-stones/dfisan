// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Limitations: dfisan cannot detect sub-object overflow in another funcs.
// TODO: Fix to detect them.

#include <stdio.h>
#include <string.h>

struct S {
  char str[8];
  int *ptr;
};

void nullify16bytes(char *str) {
  for (int i = 0; i < 16; i++)
    str[i] = 0;
}

int main(void) {
  struct S s __attribute__((annotate("dfi_protection")));
  int x;
  s.ptr = &x;

  printf("Before nullify16bytes: s.ptr = %p\n", (void *)s.ptr);
  nullify16bytes(s.str);
  // memcpy((void *)s.str, "aaaaaaaaaaaaaaaa", 16);
  printf("After nullify16bytes: s.ptr = %p\n", (void *)s.ptr);

  return 0;
}
