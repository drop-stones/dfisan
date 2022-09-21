// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test for selective dfisan annotations.

#include "runtime_info.h"
#include <assert.h>

// global
int gsafe __attribute__((annotate("dfi_protection")));
int gunsafe;

int main(void) {
  // TODO: global support
  // assert(AddrIsInSafeAligned(&gsafe) && "gsafe is not in safe aligned region");
  // assert(AddrIsInUnsafeRegion(&gunsafe) && "gunsafe is not in unsafe aligned region");

  // local
  int lsafe __attribute__((annotate("dfi_protection")));
  int lunsafe;
  assert(AddrIsInSafeAligned(&lsafe) && "lsafe is not in safe aligned region");
  assert(AddrIsInUnsafeRegion(&lunsafe) && "lunsafe is not in unsafe region");

  // heap
  int *hsafe = (int *)safe_malloc(sizeof(int));
  int *hunsafe = (int *)malloc(sizeof(int));
  assert(AddrIsInSafeAligned(hsafe) && "hsafe is not in safe aligned region");
  assert(AddrIsInUnsafeRegion(hunsafe) && "hunsafe is not in unsafe region");

  free(hsafe);
  free(hunsafe);

  return 0;
}
