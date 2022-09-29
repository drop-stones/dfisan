// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage static local variables correctly.
// TODO: Initialize static local variables

struct S {
  int i;
  long l;
};

void static_vars() {
  static char c __attribute__((annotate("dfi_protection")));      // uninitialized
  static int i  __attribute__((annotate("dfi_protection"))) = 100; // initialized
  static int arr0[8] __attribute__((annotate("dfi_protection")));
  static int arr1[8] __attribute__((annotate("dfi_protection"))) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  static struct S s  __attribute__((annotate("dfi_protection"))) = { 200, 300 };

  c++;
  i++;
  arr0[4]++;
  arr1[4]++;
  s.i++;
  s.l++;
}

int main(int argc, char **argv) {
  static_vars();
  static_vars();
  static_vars();

  return 0;
}
