// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage array copy.

#include <string.h>

void localArrayCopy() {
  int from[4] = {100, 200, 300, 400};
  int to[4];

  from[0];
  from[3];

  memcpy((void *)to, (void *)from, sizeof(int) * 4);

  from[0];
  from[3];
  to[0];
  to[3];
}

int gfrom[4] = {500, 600, 700, 800};
int gto[4];

void globalArrayCopy() {
  gfrom[0];
  gfrom[3];
  gto[0];
  gto[3];

  memcpy((void *)gto, (void *)gfrom, sizeof(int) * 4);

  gfrom[0];
  gfrom[3];
  gto[0];
  gto[3];
}

struct S {
  char c;
  int i;
};

void structArrayCopy() {
  struct S sfrom[2] = { {'a', 900}, {'b', 1000} };
  struct S sto[2];

  sfrom[0].c;
  sfrom[0].i;
  sfrom[1].c;
  sfrom[1].i;

  memcpy((void *)sto, (void *)sfrom, sizeof(struct S) * 2);

  sfrom[0].c;
  sfrom[0].i;
  sfrom[1].c;
  sfrom[1].i;
  sto[0].c;
  sto[0].i;
  sto[1].c;
  sto[1].i;
}


int main(void) {
  localArrayCopy();
  globalArrayCopy();
  structArrayCopy();

  return 0;
}