#include <assert.h>
#include <pthread.h>

#define Nthreads 8
#define LOOP 5

int a;
int b;
int c;
int temp;

int mutex;

void __VERIFIER_atomic_acquire()
{
	// assume(mutex==0);
	mutex = 1;
}

void __VERIFIER_atomic_release()
{
	// assume(mutex==1);
	mutex = 0;
}


void* thr2(void* arg)
{
  for(int i=0; i<LOOP; i++){
    // __VERIFIER_atomic_acquire();
    if (mutex != 0) continue;
    mutex = 1;
    temp = a;
    a = b;
    b = c;
    c = temp;
    __VERIFIER_atomic_release();
  }

  return 0;
}

void* thr1(void* arg)
{
  for(int i=0; i<LOOP; i++)
  {
    // __VERIFIER_atomic_acquire();
    if (mutex != 0) continue;
    mutex = 1;
    assert(a != b);
    __VERIFIER_atomic_release();
  }

  return 0;
}

int main() {
  pthread_t t[Nthreads];

  a = 1;
  b = 2;
  c = 3;
  temp = 0;

  mutex = 0;

  pthread_create(&t[0], 0, thr1, 0);
  for (int i=1; i<Nthreads; i++)
  {
    pthread_create(&t[i], 0, thr2, 0);
  }
}

