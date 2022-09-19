#include <assert.h>

//http://www.ibm.com/developerworks/java/library/j-jtp04186/index.html
//Listing 2. A counter using locks

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#define NUM_THREADS 30	
#define LOOP_LIMIT 30

volatile unsigned value;
atomic_uint m;

bool acquire()
{
	unsigned int e = 0, v = 1;
	return atomic_compare_exchange_strong_explicit(&m, &e, v, memory_order_seq_cst, memory_order_seq_cst);
}

bool release()
{
	int e = 1, v = 0;
	return atomic_exchange_explicit(&m, v, memory_order_seq_cst);
}

/*helpers for the property*/
volatile unsigned inc_flag;
volatile unsigned dec_flag;

unsigned inc() {
	unsigned inc_v = 0;
	int ctr = 0;

	while (!acquire()) {
		if (++ctr < LOOP_LIMIT) continue;
		return 0;
	}
	
	if(value == 0u-1) {
		release();
		return 0;
	}else{
		inc_v = value;
		inc_flag = 1, value = inc_v + 1; /*set flag, then update*/
		release();

		assert(dec_flag || value > inc_v);

		return inc_v + 1;
	}
}

unsigned dec() {
	unsigned dec_v;

	int ctr = 0;

	while (!acquire()) {
		if (++ctr < LOOP_LIMIT) continue;
		return 0;
	}
	if(value == 0) {
		release();

		return 0u-1; /*decrement failed, return max*/
	}else{
		dec_v = value;
		dec_flag = 1, value = dec_v - 1; /*set flag, then update*/
		release();

		assert(inc_flag || value < dec_v);

		return dec_v - 1;
	}
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

  volatile unsigned value = 0;
  atomic_init(&m,0);

  inc_flag = 0;
  dec_flag = 0;

  for (int n = 0; n < NUM_THREADS; n+=2)
	pthread_create(&t[n], NULL, thr1, NULL);

  for (int n = 1; n < NUM_THREADS; n+=2)
	pthread_create(&t[n], NULL, thr2, NULL);
}

