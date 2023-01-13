// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Data-race in Apache exploted by OWL[DNS'18] (Figure 8)
// ref. https://bz.apache.org/bugzilla/show_bug.cgi?id=46215

// dfisan cannot detect the data-race
// because dfisan don't handle def-use between the same instructions.

#include <pthread.h>
#include "test.h"
#include "../safe_alloc.h"

typedef struct {
  size_t busy;
} proxy_worker_stat;

typedef struct {
  proxy_worker_stat *s;
} proxy_worker;

void *proxy_balancer_post_request(void *arg) {
  proxy_worker *worker = (proxy_worker *)arg;

  if (worker && worker->s->busy) {
    barrier_wait(&barrier);
    worker->s->busy--;
  }

  return worker;
}

int main(void) {
  barrier_init(&barrier, 2);
  pthread_t tid[2];

  proxy_worker *worker = (proxy_worker *)safe_malloc(sizeof(proxy_worker));
  worker->s = (proxy_worker_stat *)safe_malloc(sizeof(proxy_worker_stat));
  worker->s->busy = 1;

  pthread_create(&tid[0], NULL, proxy_balancer_post_request, worker);
  pthread_create(&tid[1], NULL, proxy_balancer_post_request, worker);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  return 0;
}
