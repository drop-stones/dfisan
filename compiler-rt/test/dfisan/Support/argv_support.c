// RUN: %clang_dfisan %s -o %t
// RUN: %run %t 100 200 300 400
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage command line arguments.
// Status: skip argv[] checks in the current version.

#include <stdio.h>

int main(int argc, char **argv) {
  for (int idx = 0; idx < argc; idx++) {
    printf("argv[%d]: %s\n", idx, argv[idx]);
  }

  return 0;
}
