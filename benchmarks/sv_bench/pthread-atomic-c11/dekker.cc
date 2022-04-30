 

#include <threads.h>
#include <stdatomic.h>
#include "librace.h"
#include "model-assert.h"
// #define if(!e) if (!(e)) ERROR: MODEL_ASSERT(0)MODEL_ASSERT(0);
atomic_int flag1;
atomic_int flag2;;
atomic_int turn;
atomic_int _cc_x;
atomic_int __fence_var;

#define OLOOP 2
#define ILOOP 2

void t0(void *)
{
  int rflag2 = -1, rturn_1 = -1, rturn_2 = -1, rx = -1;

  // if(!0)MODEL_ASSERT(0);
  for(int i =0;i<OLOOP;i++){
    atomic_store_explicit(__FILE__, __LINE__, &flag1, 1, memory_order_release);
    rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_seq_cst);
    
	  int j = 0;
    while (rflag2 >= 1 && j<ILOOP)
    {
        rturn_1 = atomic_load_explicit(__FILE__, __LINE__, &turn, memory_order_seq_cst);;
        if (rturn_1 != 0)
        {
            atomic_store_explicit(__FILE__, __LINE__, &flag1, 0, memory_order_seq_cst);
            rturn_2 = atomic_load_explicit(__FILE__, __LINE__, &turn, memory_order_seq_cst);
            if(!(rturn_2 == 0))return;
            atomic_store_explicit(__FILE__, __LINE__, &flag1, 1, memory_order_seq_cst);
        }
        rflag2 = atomic_load_explicit(__FILE__, __LINE__, &flag2, memory_order_seq_cst);
        j=j+1;  
    }
    if(rflag2>=1)
        return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 2, memory_order_release);
    rx =  atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_seq_cst);
    if(rx!=2) MODEL_ASSERT(0);
    atomic_store_explicit(__FILE__, __LINE__, &turn, 1, memory_order_seq_cst);
    atomic_store_explicit(__FILE__, __LINE__, &flag1, 0, memory_order_seq_cst);
    i= i+1;
  }
}

void t1(void *)
{
  int rflag1 = -1, rturn_1 = -1, rturn_2 = -1, rx = -1;
    for(int i =0;i<OLOOP;i++){
    atomic_store_explicit(__FILE__, __LINE__, &flag2, 1, memory_order_seq_cst);
    rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_seq_cst);
    int j =0;
    while (rflag1 >= 1 && j<ILOOP)
    {
        rturn_1 = atomic_load_explicit(__FILE__, __LINE__, &turn, memory_order_seq_cst);;
        if (rturn_1 != 1)
        {
            atomic_store_explicit(__FILE__, __LINE__, &flag2, 0, memory_order_seq_cst);
            rturn_2 = atomic_load_explicit(__FILE__, __LINE__, &turn, memory_order_seq_cst);;
            if(!(rturn_2 == 1))return;
               atomic_store_explicit(__FILE__, __LINE__, &flag2, 1, memory_order_seq_cst);
        }
        rflag1 = atomic_load_explicit(__FILE__, __LINE__, &flag1, memory_order_seq_cst);
        j=j+1;
    }
    if(rflag1>=1)
        return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 1, memory_order_release);
    rx =  atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_seq_cst);
    if(rx!=1) MODEL_ASSERT(0);
    atomic_store_explicit(__FILE__, __LINE__, &turn, 0, memory_order_seq_cst);
    atomic_store_explicit(__FILE__, __LINE__, &flag2, 0, memory_order_seq_cst);

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





