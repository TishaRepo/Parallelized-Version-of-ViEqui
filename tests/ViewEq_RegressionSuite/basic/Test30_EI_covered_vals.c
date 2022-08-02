#include <assert.h>
#include <pthread.h>

int w,x,y,z;

void *t0(void *arg){
  int a = x;
  int b = z;
  return NULL;
}

void *t1(void *arg){
  x = 1;
  int a = y;
  return NULL;
}

void *t2(void *arg){
  z = 1;
  int a = w;
  y = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0;
  pthread_t thr1;
  pthread_t thr2;
 
  w = 0;
  x = 0;
  y = 0;
  z = 0;
  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  
  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  
  return 0;
}