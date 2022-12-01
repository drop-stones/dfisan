// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Dfisan gives a false alarm because `shared` accesses is not protected by same locks.

#include "test.h"

int shared __attribute__((annotate("dfi_protection")));
int flag = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *Thread1(void *arg) {
  shared = 100; // DEF

  pthread_mutex_lock(&m);
  flag = 1;
  pthread_mutex_unlock(&m);

  barrier_wait(&barrier);
  return NULL;
}

void *Thread2(void *arg) {
  barrier_wait(&barrier);

  pthread_mutex_lock(&m);
  int f = flag;
  pthread_mutex_unlock(&m);

  if (f)
    shared = 200;

  return NULL;
}

int main(void) {
  pthread_t tid[2];
  barrier_init(&barrier, 2);
  pthread_mutex_init(&m, NULL);

  pthread_create(&tid[0], NULL, Thread1, NULL);
  pthread_create(&tid[1], NULL, Thread2, NULL);
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  pthread_mutex_destroy(&m);

  return 0;
}
