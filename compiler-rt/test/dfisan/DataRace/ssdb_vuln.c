// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Data-race in SSDB found by OWL[DNS'18] (Figure 6)
// ref. https://github.com/ruigulala/concurrency-exploits/tree/3af870a562ec3d108f4a059430f199de5d92bc96/ssdb-1.9.4

#include <string.h>
#include <stdint.h>
#include "test.h"
#include "../safe_alloc.h"

invisible_barrier_t barrier2;

struct DB {
  int id;
};

struct BinlogQueue {
  struct DB *db;
  uint64_t min_seq;
  uint64_t last_seq;
  uint64_t trans_seq;
  int capacity;
};

int del_range(struct BinlogQueue *logs, uint64_t start, uint64_t end) {
  while (start <= end) {
    // access to logs->db
    barrier_wait(&barrier2);
    *logs->db;
  }
}

void *log_clean_thread_func(void *arg) {
  struct BinlogQueue *logs = (struct BinlogQueue *)arg;
  while(1) {
    if (!logs->db)
      break;
    barrier_wait(&barrier);
    del_range(logs, 0, 100);
  }
  return logs;
}

void destructor(struct BinlogQueue *logs) {
  logs->db = NULL;
}

int main(void) {
  barrier_init(&barrier, 2);
  barrier_init(&barrier2, 2);
  pthread_t tid;

  struct DB *db = (struct DB *)safe_malloc(sizeof(struct DB));
  struct BinlogQueue *logs = (struct BinlogQueue *)safe_malloc(sizeof(struct BinlogQueue));
  logs->db = db;

  pthread_create(&tid, NULL, log_clean_thread_func, logs);

  barrier_wait(&barrier);
  destructor(logs);
  barrier_wait(&barrier2);

  pthread_join(tid, NULL);

  return 0;
}
