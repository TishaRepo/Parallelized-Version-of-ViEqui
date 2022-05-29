#include <assert.h>
#include <pthread.h>

#define Nthreads 2

#define OFFSET 4
// assume(offset % WORKPERTHREAD == 0 && offset >= 0 && offset < WORKPERTHREAD*THREADSMAX);
#define WORKPERTHREAD 2
#define THREADSMAX 3
int max, m;
int storage[WORKPERTHREAD*THREADSMAX];

// int __VERIFIER_atomic_acquire()
// {
// 	int m_ = m;
// 	// if (m_!=0) return 0; 
// 	m = 1;
// 	return 1;
// }

void __VERIFIER_atomic_release()
{
	// assume(m==1);
	m = 0;
}

void findMax()
{
	int i;
	int e;

	for(i = OFFSET; i < OFFSET+WORKPERTHREAD; i++) {
		e = storage[i];
		
		if (m != 0)
			continue;
		m = 1;
		if(e > max) {
			max = e;
		}
	
		__VERIFIER_atomic_release();
		assert(e <= max);
	}
}

void* thr1(void* arg) {
  
	// assume(offset % WORKPERTHREAD == 0 && offset >= 0 && offset < WORKPERTHREAD*THREADSMAX);
	
	findMax();

//   return NULL;
}

int main(){
  pthread_t t;
//   pthread_t t[Nthreads];

  max = 0;
  m = 0;
  for (int i=0; i<WORKPERTHREAD*THREADSMAX; i++)
	storage[i] = i;

	// for (int i=0; i<Nthreads; i++) { 
	// 	pthread_create(&t[i], 0, thr1, 0); 
		pthread_create(&t, 0, thr1, 0); 
	// }

	return 0;
}

