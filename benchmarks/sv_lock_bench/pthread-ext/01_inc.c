#include <assert.h>

//http://www.ibm.com/developerworks/java/library/j-jtp04186/index.html
//A counter using locks

#include <pthread.h>

#define Nthreads 5

unsigned value, m;

int __VERIFIER_atomic_acquire()
{
	if (m==0) {
		m = 1;
		return 1;
	}
	return 0;
}

void __VERIFIER_atomic_release()
{
	if (m==1) {
		m = 0;
	}
}

void * thr1(void* arg) {
	unsigned v = 0;

	if (__VERIFIER_atomic_acquire()) {
		if(value == 0u-1) {
			__VERIFIER_atomic_release();

		}else{

			v = value;
			value = v + 1;
			__VERIFIER_atomic_release();

			unsigned v_ = value;
			assert(v_ > v);
		}
	}

	return NULL;
}

int main(){
  pthread_t t[Nthreads];
  value = 0;
  m = 0;

	for (int  n = 0; n  < Nthreads; n++) { pthread_create(&t[n], 0, thr1, 0); }

	return 0;
}

