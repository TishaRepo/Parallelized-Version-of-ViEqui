#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int a;

void *t0(void *arg){
  a = 1;

  return NULL;
}

void *t1(void *arg){
  int expected = 1;
  atomic_compare_exchange_strong_explicit(&a, &expected, 2, memory_order_seq_cst, memory_order_seq_cst);
  
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1;

  atomic_init(&a, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  return 0;
}
