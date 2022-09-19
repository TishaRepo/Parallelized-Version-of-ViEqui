#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS 5
#define LOOP_LIMIT 5

/*
to correctly model the cv_broadcast(COND) statement "b1_COND := 1;" must be manually changed to "b1_COND$ := 1;" in the abstract BP
*/

atomic_int MTX;
int COND;

bool acquire()
{
	int e = 0, v = 1;
	return atomic_compare_exchange_strong_explicit(&MTX, &e, v, memory_order_seq_cst, memory_order_seq_cst);
}

bool release()
{
	int e = 1, v = 0;
	return atomic_exchange_explicit(&MTX, v, memory_order_seq_cst);
}

bool cv_wait(){
  int ctr = 0;
  COND = 0;
  release();
  while (ctr++ < LOOP_LIMIT) {
    if (COND == 0)
      continue;

    return acquire(); 
  }

  return false;
}

void cv_broadcast() {
  COND = 1; //overapproximates semantics (for threader)
}

int LOADED;
int LOADING;

void space_map_contains(){
	assert(MTX == 1);
  // assert(1);
}

void space_map_walk(){
	assert(MTX == 1);
  // assert(1);
}

void space_map_load_wait(){
	assert(MTX == 1);
	while (LOADING) {
		assert(!LOADED);
		cv_wait();
		assert(COND); }
      	acquire();
  // assert(1);
}

void space_map_load(int k){
	int ctr = 0;

	assert(MTX == 1);
	assert(!LOADED);
	assert(!LOADING);
	LOADING = 1;
	release();
	acquire();
	for (;k && ctr++ < LOOP_LIMIT;) {
		release();
		acquire();
		if (k)
			break; 
	}
	if (k)
		LOADED = 1;
	LOADING = 0;
	cv_broadcast();
  // assert(1);
}

void space_map_unload(){
	assert(MTX == 1);
	LOADED = 0;
	assert(MTX == 1);
  // assert(1);
}

int space_map_alloc(int k){
	if (k)
		assert(MTX == 1);
  // assert(1);
	return k;
}

void space_map_sync(int k){
	int ctr = 0;
	assert(MTX == 1);
	if (k)
		return;
	while (k && ctr++ < LOOP_LIMIT) {
		while (k && ctr++ < LOOP_LIMIT) {
			if (k) {
				release();
				acquire(); }}}
	if (k) {
		release();
		acquire(); }
  // assert(1);
}

void space_map_ref_generate_map(){
	assert(MTX == 1);
  // assert(1);
}

void* thr1(void* arg){
	int k = *((int *)arg);
	acquire();
	switch((k+0)%9){
		case 1: space_map_contains(); break;
		case 2: space_map_walk(); break;
		case 3: if(LOADING)
				space_map_load_wait();
			else if(!LOADED)
				space_map_load(k);
			else
				space_map_unload(); break;
			break;
		case 6: space_map_alloc(k); break;
		case 7: space_map_sync(k); break;
		case 8: space_map_ref_generate_map(); break; }
	assert(MTX == 1);
	release();
  // assert(1);

  return 0;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&MTX, 0);
  COND = 0;

  LOADED = 0;
  LOADING = 0;

  int arg[NUM_THREADS];
  for (int n = 0; n < NUM_THREADS; n++) {
	arg[n] = n;
    pthread_create(&t[n], 0, thr1, &arg[n]);
  }
}

