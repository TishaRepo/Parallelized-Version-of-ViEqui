#include <assert.h>

//http://www.ibm.com/developerworks/java/library/j-jtp04186/index.html
//Listing 2. A nonblocking counter using CAS

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#define NUM_THREADS 5
#define LOOP_LIMIT 5

atomic_uint value;

/*helpers for the property*/
volatile unsigned inc_flag = 0;
volatile unsigned dec_flag = 0;

void assert1(unsigned inc__v)
{
	assert(dec_flag || value > inc__v);
}

unsigned inc() {
	unsigned inc__v, inc__vn;
	int ctr = 0;

	do {
		inc__v = value;

		if(inc__v == 0u-1) {
			return 0; /*increment failed, return min*/
		}

		inc__vn = inc__v + 1;

		if (atomic_compare_exchange_strong_explicit(&value,&inc__v,inc__vn,memory_order_seq_cst,memory_order_seq_cst)) {
			assert1(inc__v);
		};
	}
	while (++ctr < LOOP_LIMIT);

	return inc__vn;
}

void assert2(unsigned dec__v)
{
  assert(inc_flag || value < dec__v);
}

unsigned dec() {
	unsigned dec__v, dec__vn;
	int ctr = 0;

	do {
		dec__v = value;

		if(dec__v == 0) {
			return 0u-1; /*decrement failed, return max*/
		}

		dec__vn = dec__v - 1;

		if (atomic_compare_exchange_strong_explicit(&value,&dec__v,dec__vn,memory_order_seq_cst,memory_order_seq_cst)) {
			assert2(dec__v);
		};
	}
	while (++ctr < LOOP_LIMIT);

	return dec__vn;
}

void* thr1(void* arg){
	inc();

  return NULL;
}

void* thr2(void* arg){
	dec();

  return NULL;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&value,0);

  inc_flag = 0;
  dec_flag = 0;

  for (int n = 0; n < NUM_THREADS; n+=2)
	pthread_create(&t[n], NULL, thr1, NULL);

  for (int n = 1; n < NUM_THREADS; n+=2)
	pthread_create(&t[n], NULL, thr2, NULL);
}

