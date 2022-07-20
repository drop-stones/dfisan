// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can support memset().
// TODO: Support memset().

#include <stdlib.h>

struct S {
  char c;
  short s;
  int i;
  long l;
};

char gstr[100];
struct S gs;

int main(void) {
  char str[100];
  struct S s;

  memset(str, 0, sizeof(str));
  memset(&s, 0, sizeof(struct S));

  str[0];
  str[50];
  s.c;
  s.s;
  s.i;
  s.l;

  memset(gstr, 0, sizeof(gstr));
  memset(&gs, 0, sizeof(gs));

  gstr[0];
  gstr[50];
  gs.c;
  gs.s;
  gs.i;
  gs.l;

  return 0;
}