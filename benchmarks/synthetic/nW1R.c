#include <assert.h>
#include <pthread.h>

#define N 15

int x;
int y;

void *t0(void *arg){
  x = 1;
  return NULL;
}

void *t1(void *arg){
  int a = x;  
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0[N];
  pthread_t thr1;
 
  x = 0;
 
  for (int i = 0; i < N; i++)
    pthread_create(&thr0[i], NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
 
  for (int i = 0; i < N; i++)
    pthread_join(thr0[i], NULL);
  pthread_join(thr1, NULL);
 
  return 0;
}
