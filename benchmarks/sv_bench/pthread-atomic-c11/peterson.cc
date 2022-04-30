

#include <threads.h>
#include <stdatomic.h>
#include "librace.h"
#include "model-assert.h"

#define LOOPt0 2
#define LOOPt1 4

atomic_int flag1;
atomic_int flag2;;
atomic_int turn;
atomic_int _cc_x;
atomic_int __fence_var;

void t0(void *)
{
    int rflag2 = -1, rturn = -1, rx = -1;
    for (int k = 0; k < LOOPt0; k++) {
        atomic_store_explicit(__FILE__, __LINE__, &flag1, 1, memory_order_release);
        atomic_store_explicit(__FILE__, __LINE__, &turn, 1, memory_order_release);
        rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_acquire); //memory_order_acquire);
        rturn = atomic_load_explicit(__FILE__, __LINE__, &turn, memory_order_acquire);
        if(!(rflag2!=1 || rturn!=1)) return;
        atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 2, memory_order_release); //memory_order_release);
        rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_relaxed); //memory_order_acquire);;
        MODEL_ASSERT(rx==2);
        atomic_store_explicit(__FILE__, __LINE__, &flag1, 0, memory_order_release); //memory_order_release);
    }
}

void t1(void *)
{
    int rflag1 = -1, rturn = -1, rx = -1;
    for (int k = 0; k < LOOPt1; k++) {
        atomic_store_explicit(__FILE__, __LINE__, &flag2, 1, memory_order_release);
        atomic_store_explicit(__FILE__, __LINE__, &turn, 0, memory_order_release);
        rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_acquire); //memory_order_acquire);
        rturn = atomic_load_explicit(__FILE__, __LINE__, &turn, memory_order_acquire);
        if (!(rflag1!=1 || rturn!=0)) return; 
        atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 1, memory_order_release); //memory_order_release);
        rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_relaxed); //memory_order_acquire);;
        MODEL_ASSERT(rx==1);
        atomic_store_explicit(__FILE__, __LINE__, &flag2, 0, memory_order_release); //memory_order_release);
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
        atomic_init(&turn, 0);
        atomic_init(&__fence_var, 0);
        atomic_init(&_cc_x, 0);
        thrd_create(&_t1, t0, 0);
        thrd_create(&_t2, t1, 0);
      return 0;
}


