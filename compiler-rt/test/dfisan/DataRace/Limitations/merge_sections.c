// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <string.h>
#include "test.h"
#include "../../safe_alloc.h"

typedef struct {
	char* word;
	int count;
} wc_count_t;

typedef struct
{
   int length1;
   int length2;
   int length_out_pos;
   wc_count_t *data1;
   wc_count_t *data2;
   wc_count_t *out;
} merge_data_t;

void *merge_sections(void *args_in) {
  merge_data_t *args = (merge_data_t*)args_in;
  // int cmp_ret;
  int curr1 = 0, curr2 = 0;
  int length_out = 0;

  // args->length1;
  // args->length2;
  // args->length_out_pos;
  // args->data1[0].word;
  // args->data2[0].word;

  // memcpy(&args->out[0], &args->data2[0], sizeof(wc_count_t));
  memcpy(&args->out[length_out], &args->data1[curr1], (args->length1 - curr1)*sizeof(wc_count_t));
  memcpy(&args->out[length_out], &args->data2[curr2], (args->length2 - curr2)*sizeof(wc_count_t));

  return NULL;
}

int main(void) {
  pthread_t tid;
  merge_data_t* m_args = (merge_data_t*)safe_calloc(sizeof(merge_data_t), 1);
  m_args->length1 = 1;
  m_args->length2 = 1;
  m_args->length_out_pos = 1;
  m_args->data1 = (wc_count_t*)calloc(sizeof(wc_count_t), 1);
  m_args->data2 = (wc_count_t*)calloc(sizeof(wc_count_t), 1);
  m_args->out   = (wc_count_t*)safe_calloc(sizeof(wc_count_t), 1);

  pthread_create(&tid, NULL, merge_sections, m_args);
  pthread_join(tid, NULL);
}
