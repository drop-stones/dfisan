// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for selective dfisan annotations.

#include "runtime_info.h"
#include <assert.h>
#include <stdio.h>

// global
int gsafe __attribute__((annotate("dfi_protection")));
int gunsafe;

int main(void) {
  // global
  assert(AddrIsInSafeAlignedGlobal(&gsafe) && "gsafe is not in safe aligned region");
  assert(AddrIsInUnsafeRegion(&gunsafe) && "gunsafe is not in unsafe aligned region");
  printf("&gsafe = %zx\n", &gsafe);
  printf("&gunsafe = %zx\n", &gunsafe);

  // local
  int lsafe __attribute__((annotate("dfi_protection")));
  int lunsafe;
  assert(AddrIsInSafeAlignedHeap(&lsafe) && "lsafe is not in safe aligned region");
  assert(AddrIsInUnsafeRegion(&lunsafe) && "lunsafe is not in unsafe region");
  printf("&lsafe = %zx\n", &lsafe);
  printf("&lunsafe = %zx\n", &lunsafe);

  // heap
  int *hsafe = (int *)safe_malloc(sizeof(int));
  int *hunsafe = (int *)malloc(sizeof(int));
  assert(AddrIsInSafeAlignedHeap(hsafe) && "hsafe is not in safe aligned region");
  assert(AddrIsInUnsafeRegion(hunsafe) && "hunsafe is not in unsafe region");
  printf("hsafe = %zx\n", hsafe);
  printf("hunsafe = %zx\n", hunsafe);

  free(hsafe);
  free(hunsafe);

  return 0;
}
