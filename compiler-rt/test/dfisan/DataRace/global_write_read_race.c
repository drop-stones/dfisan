// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Test to check whether data race of global variables can be detected by dfisan.

#include <stdio.h>
#include "test.h"

int shared __attribute__((annotate("dfi_protection")));   // DEF(1)

void *write_shared() {
  shared = 100;   // DEF(2)
  barrier_wait(&barrier);
  return NULL;
}  

void *read_shared() {
  barrier_wait(&barrier);
  shared;         // USE: { 1 }, Error occured because DEF(2) is data race.
  return NULL;
}

int main(void) {
  barrier_init(&barrier, 2);
  pthread_t tid1, tid2;
  printf("Before thread\n"); 

  pthread_create(&tid1, NULL, write_shared, NULL);
  pthread_create(&tid2, NULL, read_shared,  NULL);

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  printf("After thread\n");

  return 0;
}

