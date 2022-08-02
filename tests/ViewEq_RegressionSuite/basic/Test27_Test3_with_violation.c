#include <assert.h>
#include <pthread.h>

int x; 
int y; 
int a;
int b;

void *t0(void *arg){
  y = 1;

  return NULL;
}

void *t1(void *arg){
  x = 1;
  
  return NULL;
}

void *t2(void *arg){
  a = x;
  
  return NULL;
}

void *t3(void *arg){
  b = y;
  
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  x = 0;
  y = 0;
  a = 0;
  b = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  assert(!(a==1 && b==1));

  return 0;
}
