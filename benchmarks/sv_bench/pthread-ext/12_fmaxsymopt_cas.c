#include <assert.h>
#include <pthread.h>

#define LOOP 10
#define Nthreads 2
#define OFFSET 64
// assume(offset % WORKPERTHREAD == 0 && offset >= 0 && offset < WORKPERTHREAD*THREADSMAX);

void __VERIFIER_atomic_CAS(
  volatile int *v,
  int e,
  int u,
  int *r)
{
	if(*v == e)
	{
		*v = u, *r = 1;
	}
	else
	{
		*r = 0;
	}
}

#define WORKPERTHREAD 16
#define THREADSMAX 16
int max;

int storage[WORKPERTHREAD*THREADSMAX];

void findMax(int offset){
	int i;
	int e;
	int my_max = 0;
	int c; 
	int cret;

	for(i = offset; i < offset+WORKPERTHREAD; i++) {
		e = storage[i];
		if(e > my_max) {
			my_max = e;
		}
		assert(e <= my_max);
	}

	for (int l=0; l<LOOP; l++) {
		c = max;
		if(my_max > c){
			// __VERIFIER_atomic_CAS(&max,c,my_max,&cret);
			if (max == c) {
				max = my_max;
				cret = 1;
			}
			else cret = 0; 
			if(cret){
				break;
			}
		}else{
			break;
		}
	}

	assert(my_max <= max);
}

void* thr1(void* arg) {
	int offset=OFFSET;

	// assume(offset % WORKPERTHREAD == 0 && offset >= 0 && offset < WORKPERTHREAD*THREADSMAX);

	findMax(offset);

  return 0;
}

int main(){
  max = 0;
  for (int i=0; i<WORKPERTHREAD*THREADSMAX; i++)
	storage[i] = i;
  pthread_t t[Nthreads];

	for (int i=0; i<Nthreads; i++) { 
		pthread_create(&t[i], 0, thr1, 0); 
	}
}

