// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Race between an aligned access and an unaligned access, which
// touches the same memory region.

#include "test.h"
#include <stdint.h>

struct u_uint64_t {
  uint64_t val;
};

uint64_t Global[2];

void *Thread1(void *x) {
  Global[1]++;
  barrier_wait(&barrier);
  return NULL;
}

void *Thread2(void *x) {
  barrier_wait(&barrier);
  char *p1 = (char *)(&Global[0]);
  // struct __attribute__((packed, aligned(1))) u_uint64_t { uint64_t val; };
  struct u_uint64_t *p4 = (struct u_uint64_t *)(p1 + 1);
  (*p4).val++;
  return NULL;
}

int main() {
  barrier_init(&barrier, 2);
  pthread_t t[2];
  pthread_create(&t[0], NULL, Thread1, NULL);
  pthread_create(&t[1], NULL, Thread2, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
  fprintf(stderr, "Pass\n");
  return 0;
}
