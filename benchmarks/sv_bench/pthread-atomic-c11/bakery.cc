#include <threads.h>
#include <stdatomic.h>
#include "librace.h"
#include "model-assert.h"
 
atomic_int c1;
atomic_int c2;

atomic_int n1;
atomic_int n2;
atomic_int _cc_x;

atomic_int barr;
#define LOOPt0 3
#define LOOPt1 2
void t0(void *)
{
  int rn1 = -1;
  int rn2 = -1;
  int rc2 = -1;
  int rx = -1;
  for(int j=0;j<LOOPt0;j++){
    atomic_store_explicit(__FILE__, __LINE__, &c1, 1, memory_order_seq_cst);
    rn2 = atomic_load_explicit(__FILE__, __LINE__, &n2, memory_order_acquire);
    rn1 = rn2 + 1;
    atomic_store_explicit(__FILE__, __LINE__, &n1, rn1, memory_order_seq_cst); // _relaxed);
    atomic_store_explicit(__FILE__, __LINE__, &c1, 0, memory_order_seq_cst);
    rc2 =  atomic_load_explicit(__FILE__, __LINE__, &c2, memory_order_seq_cst);
    if(!(rc2 == 0))
      return;
    rn2 =  atomic_load_explicit(__FILE__, __LINE__, &n2, memory_order_seq_cst);
    if(!(rn2 == 0 || rn1 < rn2))
      return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 2, memory_order_release);
    rx =  atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_acquire);
    MODEL_ASSERT(rx==2);
    atomic_store_explicit(__FILE__, __LINE__, &n1, 0, memory_order_seq_cst); // _relaxed);

  }
}



void t1(void *)
{
  int rn1 = -1;
  int rn2 = -1;
  int rc1 = -1;
  int rx = -1;
  for(int j=0;j<LOOPt1;j++){
    atomic_store_explicit(__FILE__, __LINE__, &c2, 1, memory_order_seq_cst);
    rn1=atomic_load_explicit(__FILE__, __LINE__, &n1, memory_order_seq_cst); // _relaxed);
    rn2 = rn1 + 1;
    atomic_store_explicit(__FILE__, __LINE__, &n2, rn2, memory_order_seq_cst); // _relaxed);
    atomic_store_explicit(__FILE__, __LINE__, &c2, 0, memory_order_seq_cst);
    rc1 = atomic_load_explicit(__FILE__, __LINE__, &c1, memory_order_seq_cst);
    if(!(rc1 == 0))
      return;
    rn1= atomic_load_explicit(__FILE__, __LINE__, &n1, memory_order_seq_cst);
    if(!(rn1 == 0 || rn2 <= rn1))
      return;
    atomic_store_explicit(__FILE__, __LINE__, &_cc_x, 1, memory_order_release); // _relaxed);
    rx = atomic_load_explicit(__FILE__, __LINE__, &_cc_x, memory_order_acquire); // _relaxed);
    MODEL_ASSERT(rx==1);
    atomic_store_explicit(__FILE__, __LINE__, &n2, 0, memory_order_seq_cst); // _relaxed);
  }
}

int *propertyChecking(void* arg)
{
    return 0;
}

int user_main(int argc, char **argv)
{
        thrd_t _t1, _t2;
        atomic_init(&c1, 0);
        atomic_init(&c2, 0);
        atomic_init(&n1, 0);
        atomic_init(&n2, 0);
        atomic_init(&barr, 1);
        atomic_init(&_cc_x, 0);
        thrd_create(&_t1, t0, 0);
        thrd_create(&_t2, t1, 0);
      return 0;
}


















