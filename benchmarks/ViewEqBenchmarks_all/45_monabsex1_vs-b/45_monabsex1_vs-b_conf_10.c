#include <assert.h>
#include <pthread.h>

#define NUM_THREADS 10

int s;

void* thr1(void* arg)
{
	int l = *((int *)arg);
  l = NUM_THREADS;
	s = l;
	assert(s == l);

  return NULL;
}

int main()
{
  pthread_t t[NUM_THREADS];

  s = 0;

  int arg[NUM_THREADS];
  for (unsigned i = 0; i < NUM_THREADS; ++i) {
    arg[i] = i;
    pthread_create(&t[i], NULL, thr1, &arg[i]);
  }
}

