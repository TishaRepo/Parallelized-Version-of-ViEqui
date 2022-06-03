// #include <threads.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
// #include "librace.h"
// #include "model-assert.h"
atomic_int c1;
atomic_int c2;

atomic_int n1;
atomic_int n2;
atomic_int _cc_x;

atomic_int barr;
#define LOOPt0 1
#define LOOPt1 1
void* t0(void *)
{
  int rn1 = -1;
  int rn2 = -1;
  int rc2 = -1;
  int rx = -1;
  for(int j=0;j<LOOPt0;j++){
   atomic_store_explicit(&c1, 1, memory_order_seq_cst);
    rn2 =atomic_load_explicit(&n2, memory_order_acquire);
    rn1 = rn2 + 1;
   atomic_store_explicit(&n1, rn1, memory_order_seq_cst); // _relaxed);
   atomic_store_explicit(&c1, 0, memory_order_seq_cst);
    rc2 = atomic_load_explicit(&c2, memory_order_seq_cst);
    if(!(rc2 == 0))
      return NULL;
    rn2 = atomic_load_explicit(&n2, memory_order_seq_cst);
    if(!(rn2 == 0 || rn1 < rn2))
      return NULL;
   atomic_store_explicit(&_cc_x, 2, memory_order_release);
    rx = atomic_load_explicit(&_cc_x, memory_order_acquire);
    assert(rx==2);
   atomic_store_explicit(&n1, 0, memory_order_seq_cst); // _relaxed);

  }
  return NULL;
}


void* t1(void *)
{
  int rn1 = -1;
  int rn2 = -1;
  int rc1 = -1;
  int rx = -1;
  for(int j=0;j<LOOPt1;j++){
   atomic_store_explicit(&c2, 1, memory_order_seq_cst);
    rn1=atomic_load_explicit( &n1, memory_order_seq_cst); // _relaxed);
    rn2 = rn1 + 1;
   atomic_store_explicit(&n2, rn2, memory_order_seq_cst); // _relaxed);
   atomic_store_explicit(&c2, 0, memory_order_seq_cst);
    rc1 =atomic_load_explicit(&c1, memory_order_seq_cst);
    if(!(rc1 == 0))
      return NULL;
    rn1=atomic_load_explicit(&n1, memory_order_seq_cst);
    if(!(rn1 == 0 || rn2 <= rn1))
      return NULL;
   atomic_store_explicit(&_cc_x, 1, memory_order_release); // _relaxed);
    rx =atomic_load_explicit(&_cc_x, memory_order_acquire); // _relaxed);
    assert(rx==1);
   atomic_store_explicit(&n2, 0, memory_order_seq_cst); // _relaxed);
  }
  return NULL;
}

int *propertyChecking(void* arg)
{
    return 0;
}

int main(int argc, char **argv)
{
        pthread_t _t1, _t2;
        atomic_init(&c1, 0);
        atomic_init(&c2, 0);
        atomic_init(&n1, 0);
        atomic_init(&n2, 0);
        atomic_init(&barr, 1);
        atomic_init(&_cc_x, 0);
        pthread_create(&_t1, NULL, t0, NULL );
        pthread_create(&_t2, NULL, t1, NULL);
        pthread_join(_t1, 0);
        pthread_join(_t2, 0);
      return 0;
}


















