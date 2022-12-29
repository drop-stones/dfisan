// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Write-read race detection

#include <stdio.h>
#include <string.h>
#include "test.h"

invisible_barrier_t barrier2;

struct Ptr {
  char name[8];
  int *ptr;
};

int flag = 1;

void *Nullify(void *arg) {
  barrier_wait(&barrier);
  struct Ptr *p = (struct Ptr *)arg;
  if (flag)
    p->ptr = NULL;    // Def p.ptr
  barrier_wait(&barrier2);
  return NULL;
}

void InputName(char *str) {
  memcpy((void *)str, "aaaaaaa", 7);
}

int main(void) {
  int x = 100;
  struct Ptr p __attribute__((annotate("dfi_protection")));
  p.ptr = &x;
  InputName(p.name);  // No overflow

  barrier_init(&barrier, 2);
  barrier_init(&barrier2, 2);
  pthread_t tid;
  pthread_create(&tid, NULL, Nullify, &p);

  if (p.ptr != NULL) {
    barrier_wait(&barrier);
    barrier_wait(&barrier2);
    *p.ptr; // Detect write-read race!!
  }

  pthread_join(tid, NULL);

  return 0;
}
