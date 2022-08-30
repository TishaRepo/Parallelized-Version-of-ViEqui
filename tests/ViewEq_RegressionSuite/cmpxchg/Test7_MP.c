#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x; 
atomic_int y; 

void *t0(void *arg){
  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&x, &expected, 1, memory_order_seq_cst, memory_order_seq_cst)) {
    y = 1;
    assert(y == 1);
    x = 0;
  }
    
  return NULL;
}

void *t1(void *arg){
  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&x, &expected, 1, memory_order_seq_cst, memory_order_seq_cst)) {
    y = 2;
    assert(y == 2);
    x = 0;
  }
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0;
  pthread_t thr1;

  atomic_init(&x, 0);
  atomic_init(&y, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  return 0;
}