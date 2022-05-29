#include <assert.h>

//Simple test_and_set lock with exponential backoff
//
//From Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors, 1991 (Fig. 1).
//Also available as pseudo-code here: http://www.cs.rochester.edu/research/synchronization/pseudocode/ss.html#tas

#include <pthread.h>

#define N 5
#define LOOP 10

#define unlocked 0
#define locked 1
int lock;
int c;

// void __VERIFIER_atomic_TAS(int& v,int& o)
// {
// 	o = v;
// 	v = 1;
// }

int acquire_lock(){
	int cond;

	// __VERIFIER_atomic_TAS(lock,cond);
	cond = lock;
	lock = locked;
	for (int i = 0; i < LOOP; i++) {
		if (cond != locked){
			break;
		}
		// __VERIFIER_atomic_TAS(lock,cond);
		cond = lock;
		lock = locked;
	}

	if (cond != lock) return 1;
	return 0;
}

void release_lock(){
	// assert(lock != unlocked);
	lock = unlocked; 
}

void* thr1(void *arg){
	for (int i = 0; i < LOOP; i++) {
		if (acquire_lock() == 0)
			continue;
		c++; assert(c == 1); c--;
		release_lock();
	}
  return 0;
}

int main(){
  pthread_t t[N];

  lock = unlocked;
  c = 0;

	for (int i = 0; i < N; i++) { 
		pthread_create(&t[i], NULL, thr1, NULL); 
	}
	
  return 0;
}

