// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <stdio.h>
#include <string.h>

int main(void) {
  char src[16] __attribute__((annotate("dfi_protection"))) = "Hello world!";
  char dst[16] __attribute__((annotate("dfi_protection")));

  strncpy(src, dst, sizeof(src));

  src[4];
  dst[4];

  return 0;
}
