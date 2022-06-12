// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can support memcpy() by pointers.

#include <string.h>

struct S {
  char c;
  short s;
  int i;
  long l;
};

void copy(void *dst, void *src, int size) {
  memcpy(dst, src, size);
}

int main(void) {
  struct S sfrom = { 'a', 100, 200, 300 };
  struct S sto;
  copy(&sto, &sfrom, sizeof(struct S));
  //memcpy(&sto, &sfrom, sizeof(struct S));

  sfrom.c;
  sfrom.s;
  sfrom.i;
  sfrom.l;
  sto.c;
  sto.s;
  sto.i;
  sto.l;

  char arrfrom[8] = "Bob";
  char arrto[8];
  copy(arrto, arrfrom, sizeof(char) * 8);
  //memcpy(arrto, arrfrom, sizeof(char) * 8);

  arrfrom[0];
  arrfrom[1];
  arrfrom[2];
  arrto[0];
  arrto[1];
  arrto[2];

  return 0;
}
