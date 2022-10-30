// RUN: %clang_dfisan %s -o %t -g
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage memcpy().

#include <string.h>
#include "../../safe_alloc.h"

struct S {
  char c;
  int i;
};

int garrFrom[4] __attribute__((annotate("dfi_protection"))) = {100, 200, 300, 400};
int garrTo[4] __attribute__((annotate("dfi_protection")));

// memcpy() of array.
void arrayMemcpy() {
  // local array init by memcpy().
  int larrFrom[4] __attribute__((annotate("dfi_protection"))) = {500, 600, 700, 800};
  int larrTo[4] __attribute__((annotate("dfi_protection")));

  // local struct array init by memcpy().
  struct S sarrFrom[2] __attribute__((annotate("dfi_protection"))) = { {'a', 1}, {'b', 2} };
  struct S sarrTo[2] __attribute__((annotate("dfi_protection")));

  memcpy((void *)garrTo, (void *)garrFrom, sizeof(int) * 4);
  memcpy((void *)larrTo, (void *)larrFrom, sizeof(int) * 4);
  memcpy((void *)sarrTo, (void *)sarrFrom, sizeof(struct S) * 2);

  // defined by memcpy().
  garrTo[0]; garrTo[1]; garrTo[2]; garrTo[3];
  larrTo[0]; larrTo[1]; larrTo[2]; larrTo[3];
  sarrTo[0].c; sarrTo[0].i; sarrTo[1].c; sarrTo[1].i;

  // defined by init.
  garrFrom[0]; garrFrom[1]; garrFrom[2]; garrFrom[3];
  larrFrom[0]; larrFrom[1]; larrFrom[2]; larrFrom[3];
  sarrFrom[0].c; sarrFrom[0].i; sarrFrom[1].c; sarrFrom[1].i;
}

struct S gFrom __attribute__((annotate("dfi_protection"))) = { 'c', 3 };
struct S gTo __attribute__((annotate("dfi_protection")));

// memcpy of struct.
void structMemcpy() {
  // local struct init by memcpy().
  struct S lFrom __attribute__((annotate("dfi_protection"))) = { 'd', 4 };
  struct S lTo __attribute__((annotate("dfi_protection")));

  // heap struct.
  struct S *hFrom = (struct S *)safe_malloc(sizeof(struct S));
  struct S *hTo = (struct S *)safe_malloc(sizeof(struct S));
  hFrom->c = 'e'; hFrom->i = 5;

  memcpy((void *)&gTo, (void *)&gFrom, sizeof(struct S));
  memcpy((void *)&lTo, (void *)&lFrom, sizeof(struct S));
  memcpy((void *)hTo, (void *)hFrom, sizeof(struct S));

  // defined by memcpy().
  gTo.c; gTo.i;
  lTo.c; lTo.i;
  hTo->c; hTo->i;

  // defined by init.
  gFrom.c; gFrom.i;
  lFrom.c; lFrom.i;
  hFrom->c; hFrom->i;
}

int main(void) {
  arrayMemcpy();
  structMemcpy();

  return 0;
}
