// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <stdio.h>
#include <string.h>

struct S {
  char str[8];
  int i;
};

struct S s __attribute__((annotate("dfi_protection")));

int main(void) {
  memset((void *)&s, 0, sizeof(struct S));
  strcpy(s.str, "hello");

  s.str[0];
  s.i;

  return 0;
}
