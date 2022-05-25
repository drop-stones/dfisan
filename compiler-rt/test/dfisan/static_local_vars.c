// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage static local variables correctly.

void static_vars() {
  static char c;      // uninitialized
  static int i = 100; // initialized

  c;
  i;

  c++;
  i++;
}

int main(int argc, char **argv) {
  static_vars();
  static_vars();
  static_vars();

  return 0;
}
