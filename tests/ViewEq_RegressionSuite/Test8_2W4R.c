#include <assert.h>
#include <pthread.h>

int x;
int y;

void *t0(void *arg){
  x = 1;
  return NULL;
}

void *t1(void *arg){
  int b = y;
  int c = x;
  return NULL;
}

void *t2(void *arg){
  y = 1;
  return NULL;
}

void *t3(void *arg){
  int e = x;
  return NULL;
}

void *t4(void *arg){
  int f = x;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0;
  pthread_t thr1;
  pthread_t thr2;
  pthread_t thr3;
  pthread_t thr4;

  x = 0;
  y = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);
  pthread_create(&thr4, NULL, t4, NULL);
  pthread_create(&thr5, NULL, t5, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);
  pthread_join(thr4, NULL);
  pthread_join(thr5, NULL);

  return 0;
}
