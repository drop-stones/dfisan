// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"
#include <pthread.h>

#define NPROC 4

int pid = 0;
int arr[NPROC][8] __attribute__((annotate("dfi_protection")));
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *Slave(void *arg) {
  pthread_mutex_lock(&m);
  int local_pid = pid;
  pid++;
  pthread_mutex_unlock(&m);

  for (int i = 0; i < 8; i++)
    arr[local_pid][i] = local_pid;
  
  return NULL;
}

int main(void) {
  pthread_t tid[NPROC];
  pthread_mutex_init(&m, NULL);

  for (int i = 0; i < NPROC-1; i++)
    pthread_create(&tid[i], NULL, Slave, NULL);

  Slave(NULL);

  for (int i = 0; i < NPROC-1; i++)
    pthread_join(tid[i], NULL);

  for (int i = 0; i < NPROC; i++)
    for (int j = 0; j < 8; j++)
      arr[i][j];
  
  return 0;
}
