#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
// #include "model-assert.h"

atomic_int __fence_var;
atomic_int x;
atomic_int y ;
atomic_int _cc_x;
#define LOOP 115

void* t0(void *)
{
  int ry = -1;
  int rX  = -1;

  for(int i=0;i<LOOP;i++) {
    atomic_store_explicit( &x, 1, memory_order_release);
    ry = atomic_load_explicit( &y, memory_order_seq_cst);;     
    if(!(ry == 0)) return NULL;
    atomic_store_explicit( &_cc_x, 2, memory_order_release);
    rX = atomic_load_explicit( &_cc_x, memory_order_seq_cst);
    assert(rX==2);
    atomic_store_explicit( &x, 0, memory_order_seq_cst);
  }
  return NULL;
}



void* t1(void *)
{
  int rx = -1;
  int rX = -1;
  for(int i=0;i<LOOP;i++) {
    atomic_store_explicit( &y, 1, memory_order_seq_cst);
    rx = atomic_load_explicit( &x, memory_order_seq_cst);;
    if(!(rx == 0)) return NULL;
    atomic_store_explicit( &_cc_x, 1, memory_order_release);
    rX = atomic_load_explicit( &_cc_x, memory_order_seq_cst);
    assert(rX==1);
    atomic_store_explicit( &y, 0, memory_order_seq_cst);
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
        atomic_init(&x, 0);
        atomic_init(&y, 0);
        atomic_init(&__fence_var, 0);
        atomic_init(&_cc_x, 0);
        pthread_create(&_t1, NULL, t0, NULL );
        pthread_create(&_t2, NULL, t1, NULL);
        pthread_join(_t1, 0);
        pthread_join(_t2, 0);
    return 0;
}
