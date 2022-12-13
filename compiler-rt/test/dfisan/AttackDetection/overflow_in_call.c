// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can detect buffer overflow in another funcs.
// Limitations: dfisan cannot protect system data (e.g., return address, malloc metadata)

#include <stdio.h>

void nullify40bytes(char *str) {
  for (int i = 0; i < 40; i++)
    str[i] = 0;
}

int main(void) {
  char str[8] __attribute__((annotate("dfi_protection")));
  int x __attribute__((annotate("dfi_protection"))) = 100;

  printf("&x = %p\n", (void *)&x);
  printf("str = %p, &str[32] = %p\n", (void *)str, (void *)&str[32]);

  printf("Before nullify40bytes: x = %d\n", x);
  nullify40bytes(str);
  printf("After nullify40bytes : x = %d\n", x);

  return 0;
}
