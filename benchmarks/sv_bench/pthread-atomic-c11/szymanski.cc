#include <threads.h>
#include <stdatomic.h>
#include "librace.h"
#include "model-assert.h"

#define LOOP 2


atomic_int flag1 ,flag2 ;
atomic_int _cc_x;



void t0(void*)
{
  int rflag2 = -1, rx=-1;
  for(int j=0;j<LOOP;j++) {
    atomic_store_explicit(__FILE__, __LINE__, &flag1, 1, memory_order_release);
    rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_acquire);
    if(!(rflag2 <3))return;
    atomic_store_explicit(__FILE__, __LINE__, &flag1, 3, memory_order_release);
    rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_acquire);
    if (rflag2 == 1)
    {
        atomic_store_explicit(__FILE__, __LINE__, &flag1, 2, memory_order_release);
        rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_acquire);
        if(!(rflag2 == 4))return;
    }
    atomic_store_explicit(__FILE__, __LINE__, &flag1, 4, memory_order_release);
    rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_acquire);
    if(!(rflag2 < 2))return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 0, memory_order_release);
    rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_acquire);
    MODEL_ASSERT(rx<=0);
    rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_acquire);
    if(!(2>rflag2 || rflag2 > 3))return;
    atomic_store_explicit(__FILE__, __LINE__, &flag1, 0, memory_order_release);
  }
}

void t1(void*)
{
  int rflag1 = -1, rx=-1;
  for(int j=0;j<LOOP;j++) {
    atomic_store_explicit(__FILE__, __LINE__, &flag2, 1, memory_order_release);
    rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_acquire);
    if(!(rflag1 < 3))return;
    atomic_store_explicit(__FILE__, __LINE__, &flag2, 3, memory_order_release);
    rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_acquire);
    if (rflag1 == 1)
    {
        atomic_store_explicit(__FILE__, __LINE__, &flag2, 2, memory_order_release);
        rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_acquire);
        if(!(rflag1 == 4))return;
    }
    atomic_store_explicit(__FILE__, __LINE__, &flag2, 4, memory_order_release);
    rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_acquire);
    if(!(rflag1 < 2))return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 1, memory_order_release);
    rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_acquire);
    MODEL_ASSERT(rx>=1);
    rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_acquire);
    if(!(2 > rflag1 || rflag1 > 3))return;
    atomic_store_explicit(__FILE__, __LINE__, &flag2, 0, memory_order_release);
  }
}


int *propertyChecking(void* arg)
{
    return 0;
}

int user_main(int argc, char **argv)
{
       thrd_t _t1, _t2;
        atomic_init(&flag1, 0);
        atomic_init(&flag2, 0);
        atomic_init(&_cc_x, 0);
        thrd_create(&_t1, t0, 0);
        thrd_create(&_t2, t1, 0);
    return 0;
}

