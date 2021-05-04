#include <assert.h>
#include <pthread.h>

int x;
int y;

void *t0(void *arg){
  int a = x;
  y = 1;
  return NULL;
}

void *t1(void *arg){
  int c = y;
  x = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0;
  pthread_t thr1;

  x = 0;
  y = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  return 0;
}