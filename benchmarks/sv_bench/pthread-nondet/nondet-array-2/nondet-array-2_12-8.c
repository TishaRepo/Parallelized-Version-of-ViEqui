#include <pthread.h>
#include "assert.h"

#define LOOP 8
#define SIZE 12
int a[SIZE] = {0};
int x;

void *thr(void* arg) {
  int t = x;
  a[t] = 1;
  x = t + 1;
}

int main(int argc, char* argv[]) {
  x = 0;
  for (int s = 0; s < SIZE; s++)
    a[s] = 0;
    
  pthread_t t[SIZE];
  int i;
  int n = LOOP;
  assert(n >= (SIZE/2) && n <= SIZE);
  for (i = 0; i < n; i++) {
    pthread_create(&t[i], 0, thr, 0);
  }
  for (i = 0; i < n; i++) {
    pthread_join(t[i], NULL);
  }
  int sum = 0;
  for (i = 0; i < n; i++) {
    sum += a[i];
  }
  assert(sum <= SIZE);
  return 0;
}
