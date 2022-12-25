// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "../safe_alloc.h"

struct Global {
  int x, y;
};
struct Global *global;

void initGlobal(void) {
  global->x = 100;
  global->y = 200;
}

int main(void) {
  global = (struct Global *)safe_malloc(sizeof(struct Global));
  global->x = 0;
  global->y = 0;

  initGlobal();

  global->x;
  global->y;

  return 0;
}
