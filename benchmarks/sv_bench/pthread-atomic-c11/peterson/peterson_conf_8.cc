

#include <pthread.h>
#include <stdatomic.h>
// #include "librace.h"
#include <assert.h>

#define LOOP 8

atomic_int flag1;
atomic_int flag2;;
atomic_int turn;
atomic_int _cc_x;
atomic_int __fence_var;

void* t0(void *)
{
    int rflag2 = -1, rturn = -1, rx = -1;
    for (int k = 0; k < LOOP; k++) {
        atomic_store_explicit( &flag1, 1, memory_order_release);
        atomic_store_explicit( &turn, 1, memory_order_release);
        rflag2 = atomic_load_explicit( &flag2, memory_order_acquire); //memory_order_acquire);
        rturn = atomic_load_explicit( &turn, memory_order_acquire);
        if(!(rflag2!=1 || rturn!=1)) return NULL;
        atomic_store_explicit( &_cc_x, 2, memory_order_release); //memory_order_release);
        rx = atomic_load_explicit( &_cc_x, memory_order_relaxed); //memory_order_acquire);;
        assert(rx==2);
        atomic_store_explicit( &flag1, 0, memory_order_release); //memory_order_release);
    }
    return NULL;
}

void* t1(void *)
{
    int rflag1 = -1, rturn = -1, rx = -1;
    for (int k = 0; k < LOOP; k++) {
        atomic_store_explicit( &flag2, 1, memory_order_release);
        atomic_store_explicit( &turn, 0, memory_order_release);
        rflag1 = atomic_load_explicit( &flag1, memory_order_acquire); //memory_order_acquire);
        rturn = atomic_load_explicit( &turn, memory_order_acquire);
        if (!(rflag1!=1 || rturn!=0)) return NULL; 
        atomic_store_explicit( &_cc_x, 1, memory_order_release); //memory_order_release);
        rx = atomic_load_explicit( &_cc_x, memory_order_relaxed); //memory_order_acquire);;
        assert(rx==1);
        atomic_store_explicit( &flag2, 0, memory_order_release); //memory_order_release);
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
        atomic_init(&turn, 0);
        atomic_init(&__fence_var, 0);
        atomic_init(&_cc_x, 0);
        pthread_create(&_t1, NULL, t0, NULL );
        pthread_create(&_t2, NULL, t1, NULL);
        pthread_join(_t1, 0);
        pthread_join(_t2, 0);
      return 0;
}


