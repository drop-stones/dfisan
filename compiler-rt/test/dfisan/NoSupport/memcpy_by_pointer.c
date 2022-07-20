// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can support memcpy() by pointers.
// TODO: Use-Def Analysis supports memcpy() by callee.

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

void copyS(struct S *dst, struct S *src) {
  memcpy(dst, src, sizeof(struct S));
}

int main(void) {
  struct S sfrom = { 'a', 100, 200, 300 };
  // struct S sto = {'b', 400, 500, 600};  // Error occured!!
///*
  struct S sto;
  // struct S sto = {'b', 400, 500, 600};  // Error occured!!
  copy(&sto, &sfrom, sizeof(struct S));
//*/
  // No error
  //copyS(&sto, &sfrom);
  //memcpy(&sto, &sfrom, sizeof(struct S));

  sfrom.c;
  sfrom.s;
  sfrom.i;
  sfrom.l;
  sto.c;
  sto.s;    // No check
  sto.i;    //
  sto.l;    //

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
