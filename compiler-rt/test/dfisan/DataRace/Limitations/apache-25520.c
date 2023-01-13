// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Data-race in Apache-2.0.48 exploted by OWL[DNS'18] (Figure 7)
// ref. https://bz.apache.org/bugzilla/show_bug.cgi?id=25520

#include <pthread.h>
#include "test.h"
#include "../../safe_alloc.h"

typedef int apr_file_t;
typedef int apr_size_t;
typedef pthread_mutex_t apr_anylock_t;
#define LOG_BUFSIZE 1024

typedef struct {
  apr_file_t *handle;
  apr_size_t outcnt;
  char outbuf[LOG_BUFSIZE];
  apr_anylock_t mutex;
} buffered_log;

void *ap_buffered_log_writer(void *handle) {
  char *s;
  int i, len = 100;
  buffered_log *buf = (buffered_log*)handle;

  i = 0;
  s = &buf->outbuf[buf->outcnt];
  barrier_wait(&barrier);
  for (; i < len; ++i) {
    // memcpy + move pointer
  }
  buf->outcnt += len;
  return buf;
}

int main(void) {
  barrier_init(&barrier, 2);
  pthread_t tid[2];

  buffered_log *log = (buffered_log *)safe_malloc(sizeof(buffered_log));
  log->outcnt = 0;

  pthread_create(&tid[0], NULL, ap_buffered_log_writer, log);
  pthread_create(&tid[1], NULL, ap_buffered_log_writer, log);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  return 0;
}
