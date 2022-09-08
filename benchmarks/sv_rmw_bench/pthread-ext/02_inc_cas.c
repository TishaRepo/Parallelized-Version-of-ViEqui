#include <assert.h>

//http://www.ibm.com/developerworks/java/library/j-jtp04186/index.html
//Listing 2. A nonblocking counter using CAS

#include <pthread.h>
#include <stdatomic.h>

#define NUM_THREADS 2
#define LOOP_LIMIT 5

atomic_uint value;

void* thr1(void* arg) {
	unsigned v,vn;
	int ctr = 0;

	do {
        v = value;

		if(v == 0u-1) {
			return 0;
		}

		vn = v + 1;

		if (atomic_compare_exchange_strong_explicit(&value,&v,vn,memory_order_seq_cst,memory_order_seq_cst)) {
			assert(value > v);
		}
	}
	while (++ctr < LOOP_LIMIT);

	return NULL;
}

int main(){
  pthread_t t[NUM_THREADS];
  atomic_init(&value, 0);

  for (int n = 0; n < NUM_THREADS; n++)
	pthread_create(&t[n], NULL, thr1, NULL);
}

