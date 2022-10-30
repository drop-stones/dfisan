// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests for rename optimization.

int main(int argc, char **argv) {
  int x __attribute__((annotate("dfi_protection"))) = 100;
  if (argc == 0)
    x = 200;
  else if (argc == 1)
    x = 300;
  else
    x = 400;
  
  x;    // Use a single DefID because of rename optimization

  return 0;
}
