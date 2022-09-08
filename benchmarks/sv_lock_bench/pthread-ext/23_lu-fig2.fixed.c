#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>


int mThread=0;
int start_main=0;
atomic_int mStartLock=0;
int __COUNT__ =0;

bool acquire()
{
	unsigned int e = 0, v = 1;
	return atomic_compare_exchange_strong_explicit(&mStartLock, &e, v, memory_order_seq_cst, memory_order_seq_cst);
}

bool release()
{
	int e = 1, v = 0;
	return atomic_exchange_explicit(&mStartLock, v, memory_order_seq_cst);
}

void __VERIFIER_atomic_thr1(int PR_CreateThread__RES)
{
      if( __COUNT__ == 0 ) { // atomic check(0);
	mThread = PR_CreateThread__RES; 
	__COUNT__ = __COUNT__ + 1; 
      } else { assert(0); } 
}

void* thr1(void* arg) { //nsThread::Init (mozilla/xpcom/threads/nsThread.cpp 1.31)

  int PR_CreateThread__RES = 1;
  __VERIFIER_atomic_acquire();
  start_main=1;
  __VERIFIER_atomic_thr1(PR_CreateThread__RES);
  __VERIFIER_atomic_release();

  return 0;
}

void __VERIFIER_atomic_thr2(int self)
{
      if( __COUNT__ == 1 ) { // atomic check(1);
	//int rv = self; // self->RegisterThreadSelf();
	__COUNT__ = __COUNT__ + 1;
      } else { assert(0); } 
}

void* thr2(void* arg) { //nsThread::Main (mozilla/xpcom/threads/nsThread.cpp 1.31)

  int self = mThread;
  while (start_main==0);
  __VERIFIER_atomic_acquire();
  __VERIFIER_atomic_release();
  __VERIFIER_atomic_thr2(self);

  return 0;
}

int main()
{
  pthread_t t;

  pthread_create(&t, 0, thr1, 0);
  thr2(0);

  return 0;
}

