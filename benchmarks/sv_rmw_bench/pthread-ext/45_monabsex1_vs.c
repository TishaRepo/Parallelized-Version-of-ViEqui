#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

int s;

void* thr1(void* arg)
{
    int l = 4;
    __VERIFIER_atomic_begin();
    s = l;
    __VERIFIER_atomic_end();
    __VERIFIER_atomic_begin();
    assert(s == l);
    __VERIFIER_atomic_end();

    return 0;
}

int main()
{
  s = __VERIFIER_nondet_int();

  pthread_t t;

  while(1) pthread_create(&t, 0, thr1, 0);
}

