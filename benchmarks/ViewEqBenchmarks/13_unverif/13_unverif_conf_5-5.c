#include <assert.h>

//Symmetry-Aware Predicate Abstraction for Shared-Variable Concurrent Programs (Extended Technical Report). CoRR abs/1102.2330 (2011)

#include <stdatomic.h>
#include <pthread.h>

#define NUM_THREADS 5
#define LOOP_LIMIT 5

atomic_uint r;
atomic_uint s;

void* thr1(void* arg){
  unsigned int l = 0;
  int ctr = 0;

  atomic_fetch_add_explicit(&r, 1, memory_order_seq_cst);
  if(r == 1){
    while (ctr++ < LOOP_LIMIT) {
      L3: s = s + 1;
      l = l + 1;
      assert(s == l);
    }
  }

  return 0;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&r, 0);
  atomic_init(&s, 0);

  for (int n = 0; n < NUM_THREADS; n++)
	  pthread_create(&t[n], NULL, thr1, NULL);
}

