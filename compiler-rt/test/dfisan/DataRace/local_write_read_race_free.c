// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Test to check whether data race of local variables can be detected by dfisan.

#include <stdio.h>
#include <pthread.h>

#define CNT 10000000

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *write_shared(void *arg) {
  int *shared_ptr = (int *)arg;
  for (int i = 0; i < CNT; i++) {
    pthread_mutex_lock(&m);
    *shared_ptr = 100;  // DEF(2)
    pthread_mutex_unlock(&m);
  }
  return NULL;
}  

void *read_shared(void *arg) {
  int *shared_ptr = (int *)arg;
  for (int i = 0; i < CNT; i++) {
    pthread_mutex_lock(&m);
    *shared_ptr;    // USE: { 1 }, Error occured because DEF(2) is data-race.
    pthread_mutex_unlock(&m);
  }
  return NULL;
}

int main(void) {
  pthread_t tid1, tid2;
  pthread_mutex_init(&m, NULL);
  int shared __attribute__((annotate("dfi_protection"))) = 0; // DEF(1)

  printf("Before thread\n"); 

  pthread_create(&tid1, NULL, write_shared, &shared);
  pthread_create(&tid2, NULL, read_shared,  &shared);

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  printf("After thread\n");

  pthread_mutex_destroy(&m);

  return 0;
}

