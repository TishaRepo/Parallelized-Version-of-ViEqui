#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>

#define NUM_THREADS 1
#define LOOP_LIMIT 2

atomic_int count;

atomic_int mutexa;
atomic_int mutexb;

void* thr0(void* arg)
{
  int ctr = 0;
  while(ctr++ < LOOP_LIMIT)
  {
    int e = 0;
    if (atomic_compare_exchange_strong_explicit(&mutexa, &e, 1, memory_order_seq_cst, memory_order_seq_cst)) {
      assert(count >= -1);
      atomic_exchange_explicit(&mutexa, 0, memory_order_seq_cst);
    }
      
    e = 0;
    if (atomic_compare_exchange_strong_explicit(&mutexb, &e, 1, memory_order_seq_cst, memory_order_seq_cst)) {
      assert(count <= 1);
      atomic_exchange_explicit(&mutexb, 0, memory_order_seq_cst);
    }
  }
  return 0;
}

void* thr1(void* arg)
{
  int ctr = 0;
  while (ctr++ < LOOP_LIMIT) {
    int e = 0;
    if (atomic_compare_exchange_strong_explicit(&mutexa, &e, 1, memory_order_seq_cst, memory_order_seq_cst)) {
      atomic_fetch_add_explicit(&count, 1, memory_order_seq_cst);
      atomic_fetch_sub_explicit(&count, 1, memory_order_seq_cst);
      atomic_exchange_explicit(&mutexa, 0, memory_order_seq_cst);
    }
  }
  return 0;
}

void* thr2(void* arg)
{
  int ctr = 0;
  while (ctr++ < LOOP_LIMIT) {
    int e = 0;
    if (atomic_compare_exchange_strong_explicit(&mutexb, &e, 1, memory_order_seq_cst, memory_order_seq_cst)) {
      atomic_fetch_sub_explicit(&count, 1, memory_order_seq_cst);
      atomic_fetch_add_explicit(&count, 1, memory_order_seq_cst);
      atomic_exchange_explicit(&mutexb, 0, memory_order_seq_cst);
    }
  }
  return 0;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&count, 0);

  atomic_init(&mutexa, 0);
  atomic_init(&mutexb, 0);

  pthread_create(&t[0], NULL, thr0, NULL);

  for (int n = 1; n < NUM_THREADS; n+=2)
	  pthread_create(&t[n], NULL, thr1, NULL);

  for (int n = 2; n < NUM_THREADS; n+=2)
	  pthread_create(&t[n], NULL, thr2, NULL);
}