// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Data-race in Libsafe found by OWL[DNS'18] (Figure 1)

#include <string.h>
#include "test.h"

int dying __attribute__((annotate("dfi_protection"))) = 0;

uint stack_check(char *str) {
  // ...
  barrier_wait(&barrier);
  if (dying)
    return 0;
  // check overflow
  return 1;
}

void libsafe_die() {
  // ...
  dying = 1;
  barrier_wait(&barrier);
}

char *libsafe_strcpy(char *dst, char *src) {
  // ...
  if (stack_check(dst) == 0)
    return strcpy(dst, src);
  else
    exit(1);  // error
}

void *Thread1(void *arg) {
  libsafe_die();
  return NULL;
}

int main(void) {
  barrier_init(&barrier, 2);
  pthread_t tid;

  char src[16] = "Hello World\n", dst[16];

  pthread_create(&tid, NULL, Thread1, NULL);

  libsafe_strcpy(src, dst);

  pthread_join(tid, NULL);

  return 0;
}
