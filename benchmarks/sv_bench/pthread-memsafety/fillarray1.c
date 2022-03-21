#include <pthread.h>
#include <stdlib.h>

#define SIZ 10
#define LOOP 10

int size;
int ind;
int j, i;
int x, y, z;

void *t1(void *arg) {
  x = 1;
  y = 1;
  int *a = (int *)arg;

  for (int l=0;l<LOOP;l++) {
    if (y == 1 && z) {}
    else {
      while (ind < size - 1) {
        ++ind;
        a[ind] = i;
      }
      x = 0;
      break;
    }
  }

  pthread_exit(NULL);
}

void *t2(void *arg) {
  z = 1;
  y = 0;
  int *a = (int *)arg;
  for (int l=0;l<LOOP;l++) {
    if (y == 0 && x) {}
    else {
      while (ind < size - 1) {
        ++ind;
        a[ind] = j;
      }
      z = 0;
      break;
    }
  }

  pthread_exit(NULL);
}

int main(int argc, char **argv) {
  j = 2; i = 1;
  size = SIZ;
  x = y = z = 0;

  if (size < 1 || size > 20) {
    return 0;
  }

  int *a = (int *)malloc(size);
  pthread_t id1, id2;

  ind = 0;

  pthread_create(&id1, NULL, t1, a);
  pthread_create(&id2, NULL, t2, a);

  free(a);
  return 0;
}
