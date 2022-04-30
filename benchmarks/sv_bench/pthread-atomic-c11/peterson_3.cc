 

#include <threads.h>
#include <stdatomic.h>
#include "librace.h"
#include "model-assert.h"

#define LOOP 1
atomic_int level0 , level1 , level2 ;
atomic_int waiting0 , waiting1 ;
atomic_int _cc_x;
atomic_int __fence_var;

void t0(void *)
{
  int rwaiting = -1, rlevel1 = -1, rlevel2 = -1, rx = -1;
    for(int i=0;i<LOOP;i++) {
    atomic_store_explicit(__FILE__, __LINE__, &level0, 0, memory_order_seq_cst); //memory_order_release);
    atomic_store_explicit(__FILE__, __LINE__, &waiting0, 0, memory_order_seq_cst); //memory_order_release);
    rwaiting = atomic_load_explicit(__FILE__, __LINE__, &waiting0, memory_order_seq_cst); //memory_order_acquire);
    rlevel1 = atomic_load_explicit(__FILE__, __LINE__, &level1, memory_order_seq_cst); //memory_order_acquire);
    rlevel2 = atomic_load_explicit(__FILE__, __LINE__, &level2, memory_order_seq_cst); //memory_order_acquire);
    if(!(rwaiting != 0 || (rlevel1 < 0 && rlevel2 < 0))) return;
    atomic_store_explicit(__FILE__, __LINE__, &level0, 1, memory_order_seq_cst); //memory_order_release);
    atomic_store_explicit(__FILE__, __LINE__, &waiting1, 0, memory_order_seq_cst); //memory_order_release);
    rwaiting = atomic_load_explicit(__FILE__, __LINE__, &waiting1, memory_order_seq_cst); //memory_order_acquire);
    rlevel1 = atomic_load_explicit(__FILE__, __LINE__, &level1, memory_order_seq_cst); //memory_order_acquire);
    rlevel2 = atomic_load_explicit(__FILE__, __LINE__, &level2, memory_order_seq_cst); //memory_order_acquire);
    if(!(rwaiting != 0 || (rlevel1 < 1 && rlevel2 < 1))) return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 2, memory_order_seq_cst); //memory_order_release);
    rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_seq_cst); //memory_order_acquire);
    MODEL_ASSERT(rx==2);
    atomic_store_explicit(__FILE__, __LINE__, &level0, 0, memory_order_seq_cst); //memory_order_release);
  }
}

void t1(void *)
{
  int rwaiting = -1, rlevel0 = -1, rlevel2 = -1, rx = -1;
    for(int i=0;i<LOOP;i++) {
    atomic_store_explicit(__FILE__, __LINE__, &level1, 0, memory_order_seq_cst); //memory_order_release);
    atomic_store_explicit(__FILE__, __LINE__, &waiting0, 1, memory_order_seq_cst);
    rwaiting = atomic_load_explicit(__FILE__, __LINE__, &waiting0, memory_order_acquire);
    rlevel2 = atomic_load_explicit(__FILE__, __LINE__, &level2, memory_order_relaxed); //memory_order_acquire);
    rlevel0 = atomic_load_explicit(__FILE__, __LINE__, &level0, memory_order_relaxed); //memory_order_acquire);
    if(!(rwaiting != 1 || (rlevel2 < 0 && rlevel0 < 0))) return;
    atomic_store_explicit(__FILE__, __LINE__, &level1, 1, memory_order_seq_cst); //memory_order_release);
    atomic_store_explicit(__FILE__, __LINE__, &waiting1, 1, memory_order_seq_cst); //memory_order_release);
    rwaiting = atomic_load_explicit(__FILE__, __LINE__, &waiting1, memory_order_seq_cst); //memory_order_acquire);
    rlevel2 = atomic_load_explicit(__FILE__, __LINE__, &level2, memory_order_seq_cst); //memory_order_acquire);
    rlevel0 = atomic_load_explicit(__FILE__, __LINE__, &level0, memory_order_seq_cst);
    if(!(rwaiting != 1 || (rlevel2 < 1 && rlevel0 < 1))) return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 1, memory_order_seq_cst); //memory_order_release);
    rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_seq_cst); //memory_order_acquire);
    MODEL_ASSERT(rx==1);
    atomic_store_explicit(__FILE__, __LINE__, &level1, 0, memory_order_seq_cst); //memory_order_release);
  }
}


void t2(void * )
{
  int rwaiting = -1, rlevel0 = -1, rlevel1 = -1, rx = -1;
    for(int i=0;i<LOOP;i++) {
    atomic_store_explicit(__FILE__, __LINE__, &level2, 0, memory_order_seq_cst); //memory_order_release);
    atomic_store_explicit(__FILE__, __LINE__, &waiting0, 2, memory_order_seq_cst);
    rwaiting = atomic_load_explicit(__FILE__, __LINE__, &waiting0, memory_order_relaxed);
    rlevel1 = atomic_load_explicit(__FILE__, __LINE__, &level1, memory_order_relaxed); //memory_order_acquire);
    rlevel0 = atomic_load_explicit(__FILE__, __LINE__, &level0, memory_order_relaxed); //memory_order_acquire);
    if(!(rwaiting != 2 || (rlevel1 < 0 && rlevel0 < 0))) return;
    atomic_store_explicit(__FILE__, __LINE__, &level2, 1, memory_order_seq_cst); //memory_order_release);
    atomic_store_explicit(__FILE__, __LINE__, &waiting1, 2, memory_order_seq_cst); //memory_order_release);
    rwaiting = atomic_load_explicit(__FILE__, __LINE__, &waiting1, memory_order_seq_cst); //memory_order_acquire);
    rlevel1 = atomic_load_explicit(__FILE__, __LINE__, &level1, memory_order_seq_cst); //memory_order_acquire);
    rlevel0 = atomic_load_explicit(__FILE__, __LINE__, &level0, memory_order_seq_cst);
    if(!(rwaiting != 2 || (rlevel1 < 1 && rlevel0 < 1))) return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 3, memory_order_seq_cst); //memory_order_release);
    rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_seq_cst); //memory_order_acquire);
    MODEL_ASSERT(rx==3);
    atomic_store_explicit(__FILE__, __LINE__, &level2, 0, memory_order_seq_cst); //memory_order_release);
  }
}


int *propertyChecking(void* arg)
{
    return 0;
}

int user_main(int argc, char **argv) 
{
        thrd_t _t1, _t2, _t3;
        atomic_init(&level0, 0);
        atomic_init(&level1, 0);
        atomic_init(&level2, 0);
        atomic_init(&waiting1, 0);
        atomic_init(&waiting0, 0);
        atomic_init(&__fence_var, 0);
        atomic_init(&_cc_x, 0);
        thrd_create(&_t1, t0, 0);
        thrd_create(&_t2, t1, 0);
        thrd_create(&_t3, t2, 0);
    return 0;
}
