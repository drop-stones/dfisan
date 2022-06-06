// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage function side effects.

struct S {
  int i;
  long l;
};

void side_effect(int *i, struct S *s, char *str, int size) {
  *i = 400;
  s->i = 500;
  s->l = 600;
  for (int i = 0; i < size; i++)
    str[i] = 'a';
}

int main(void) {
  int i = 100;
  struct S s = {200, 300};
  char arr[8] = "Bob";

  side_effect(&i, &s, arr, 8);

  i;
  s.i;
  s.l;
  arr[0];
  arr[7];

  return 0;
}