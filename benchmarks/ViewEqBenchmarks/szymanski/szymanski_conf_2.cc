#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
// #include "model-assert.h"

#define LOOP 2


atomic_int flag1 ,flag2 ;
atomic_int _cc_x;



void* t0(void*)
{
  int rflag2 = -1, rx=-1;
  for(int j=0;j<LOOP;j++) {
    atomic_store_explicit( &flag1, 1, memory_order_release);
    rflag2 = atomic_load_explicit( &flag2, memory_order_acquire);
    if(!(rflag2 <3))return NULL;
    atomic_store_explicit( &flag1, 3, memory_order_release);
    rflag2 = atomic_load_explicit( &flag2, memory_order_acquire);
    if (rflag2 == 1)
    {
        atomic_store_explicit( &flag1, 2, memory_order_release);
        rflag2 = atomic_load_explicit( &flag2, memory_order_acquire);
        if(!(rflag2 == 4))return NULL;
    }
    atomic_store_explicit( &flag1, 4, memory_order_release);
    rflag2 = atomic_load_explicit( &flag2, memory_order_acquire);
    if(!(rflag2 < 2))return NULL;
    atomic_store_explicit( &_cc_x, 0, memory_order_release);
    rx = atomic_load_explicit( &_cc_x, memory_order_acquire);
    assert(rx<=0);
    rflag2 = atomic_load_explicit( &flag2, memory_order_acquire);
    if(!(2>rflag2 || rflag2 > 3))return NULL;
    atomic_store_explicit( &flag1, 0, memory_order_release);
  }
  return NULL;
}

void* t1(void*)
{
  int rflag1 = -1, rx=-1;
  for(int j=0;j<LOOP;j++) {
    atomic_store_explicit( &flag2, 1, memory_order_release);
    rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
    if(!(rflag1 < 3))return NULL;
    atomic_store_explicit( &flag2, 3, memory_order_release);
    rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
    if (rflag1 == 1)
    {
        atomic_store_explicit( &flag2, 2, memory_order_release);
        rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
        if(!(rflag1 == 4))return NULL;
    }
    atomic_store_explicit( &flag2, 4, memory_order_release);
    rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
    if(!(rflag1 < 2))return NULL;
    atomic_store_explicit( &_cc_x, 1, memory_order_release);
    rx = atomic_load_explicit( &_cc_x, memory_order_acquire);
    assert(rx>=1);
    rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
    if(!(2 > rflag1 || rflag1 > 3))return NULL;
    atomic_store_explicit( &flag2, 0, memory_order_release);
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
        atomic_init(&flag1, 0);
        atomic_init(&flag2, 0);
        atomic_init(&_cc_x, 0);
        pthread_create(&_t1, NULL, t0, NULL );
        pthread_create(&_t2, NULL, t1, NULL);
        pthread_join(_t1, 0);
        pthread_join(_t2, 0);
    return 0;
}

