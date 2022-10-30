// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage static local variables correctly.

#include <assert.h>

struct S {
  int i;
  long l;
};

void local_static_init() {
  static char c __attribute__((annotate("dfi_protection")));      // uninitialized
  static int i  __attribute__((annotate("dfi_protection"))) = 100; // initialized
  static int arr0[8] __attribute__((annotate("dfi_protection")));
  static int arr1[8] __attribute__((annotate("dfi_protection"))) = { 0, 1, 2, 3, 4, 5, 6, 7 };
  static struct S s  __attribute__((annotate("dfi_protection"))) = { 200, 300 };

  assert((c == 0 && i == 100) && "Int is not initialized");
  assert((s.i == 200 && s.l == 300) && "Struct is not initialized");
  for (int i = 0; i < 8; i++)
    assert(arr0[i] == 0);
  for (int i = 0; i < 8; i++)
    assert(arr1[i] == i && "Array is not initialized");
}

int main(int argc, char **argv) {
  local_static_init();

  return 0;
}
