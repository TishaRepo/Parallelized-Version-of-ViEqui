#include <assert.h>
#include <pthread.h>

int arr[2];

void *t0(void *arg){
  arr[1] = 1;

  return NULL;
}

void *t1(void *arg){
  arr[0] = 1;
  
  return NULL;
}

void *t2(void *arg){
  int a = arr[0];
  
  return NULL;
}

void *t3(void *arg){
  int b = arr[1];
  
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  arr[0] = 0;
  arr[1] = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  return 0;
}
