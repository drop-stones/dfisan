// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <pthread.h>
#include <stdio.h>
#include <string.h>

struct {
  int i;
  char c1, c2, c3, c4;
} S __attribute__((annotate("dfi_protection")));

int G;

void *Thread1(void *x) {
  G = S.c1 + S.c3;
  return NULL;
}

void *Thread2(void *x) {
  S.c2 = 1;
  return NULL;
}

int main() {
  pthread_t t[2];
  memset(&S, 123, sizeof(S));
  pthread_create(&t[0], NULL, Thread1, NULL);
  pthread_create(&t[1], NULL, Thread2, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
  fprintf(stderr, "PASS\n");
}
