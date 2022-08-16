// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage array arguments.

#define SIZE 100

struct Vec3D {
  int x, y, z;
};

static struct Vec3D *head;

void setHead(struct Vec3D arr[]) {
  head = &(arr[0]);
}

void setElement(int i) {
  head[i].x = head[i].y = head[i].z = i;
}

void readElement(struct Vec3D arr[], int i) {
  arr[i].x;
  arr[i].y;
  arr[i].z;
}

int main(void) {
  struct Vec3D *arr = (struct Vec3D *)malloc(sizeof(struct Vec3D) * SIZE);
  setHead(arr);
  for (int i = 0; i < SIZE; i++)
    setElement(i);
  
  for (int i = 0; i < SIZE; i++)
    readElement(arr, i);

  return 0;
}
