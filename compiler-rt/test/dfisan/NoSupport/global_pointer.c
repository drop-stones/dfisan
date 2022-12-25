// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// dfisan cannot handle global pointer because SVF cannot calculate value-flow of them.

#include "../safe_alloc.h"

int *global;

void initGlobal(void) {
  global[10];
  global[10] = 200;
}

int main(void) {
  global = (int *)safe_malloc(sizeof(int) * 100);
  global[10] = 100;

  initGlobal();

  global[10];

  return 0;
}
