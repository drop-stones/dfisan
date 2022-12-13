// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// No-false alarm

#include <stdio.h>
#include <string.h>
#include "test.h"

struct Ptr {
  char name[8];
  int *ptr;
};

int flag = 0;

void *Nullify(void *arg) {
  struct Ptr *p = (struct Ptr *)arg;
  if (flag)
    p->ptr = NULL;
  barrier_wait(&barrier);
  return NULL;
}

void InputName(char *str) {
  for (int i = 0; i < 16; i++)
    str[i] = 'a';
}

int main(void) {
  int x = 100;
  struct Ptr p __attribute__((annotate("dfi_protection")));
  p.ptr = &x;
  // InputName(&p.name[0]);  // buffer overflow!!
  memcpy((void *)p.name, "no err", 7);

  barrier_init(&barrier, 2);
  pthread_t tid;
  pthread_create(&tid, NULL, Nullify, &p);

  barrier_wait(&barrier);
  *p.ptr; // Detect write-read race!!

  pthread_join(tid, NULL);

  return 0;
}
