// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

void *deref(void *arg) {
  barrier_wait(&barrier);
  int **ptr = (int **)arg;
  *ptr;                     // nullptr dereference
  return NULL;
}

void *nullify(void *arg) {
  int **ptr = (int **)arg;
  *ptr = NULL;              // p = NULL
  barrier_wait(&barrier);
  return NULL;
}

int main(void) {
  int *p __attribute__((annotate("dfi_protection"))) = (int *)malloc(sizeof(int) * 100);

  barrier_init(&barrier, 2);
  pthread_t tid[2];

  pthread_create(&tid[0], NULL, deref, &p);
  pthread_create(&tid[1], NULL, nullify, &p);
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  return 0;
}
