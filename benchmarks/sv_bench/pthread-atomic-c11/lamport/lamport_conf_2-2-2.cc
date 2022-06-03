#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
// #include "model-assert.h"

#define OFLOOP 2 // outer for loop
#define IFLOOP 2 // inner for loop
#define WLOOP  2 // while loop

atomic_int x, y;
atomic_int b1, b2;
atomic_int _cc_X;

atomic_int __fence_var;

void* t0(void *)
{
  int ry1 = -1, ry2 = -1, ry3 = -1, ry4 = -1, rx = -1, rb2 = -1;
  for(int l1=0;l1<OFLOOP;l1++) {
    for(int l2=0;l2<IFLOOP;l2++) 
    {
        atomic_store_explicit( &b1, 1, memory_order_relaxed); // memory_order_release);
        atomic_store_explicit( &x, 1, memory_order_relaxed);
        ry1 = atomic_load_explicit( &y, memory_order_relaxed);;
        if (ry1 != 0)
        {
            atomic_store_explicit( &b1, 0, memory_order_relaxed); // memory_order_release);
            ry2 = atomic_load_explicit( &y, memory_order_relaxed);
            int l3 = 0;
            while (ry2 != 0 && l3<WLOOP)
            {
                ry2 = atomic_load_explicit( &y, memory_order_relaxed);;
                l3++;
            };
            if(ry2!=0)
                return NULL;
            continue;
        }
        atomic_store_explicit( &y, 1, memory_order_seq_cst); 
        rx = atomic_load_explicit( &x, memory_order_seq_cst);;
        if (rx != 1)
        {
            atomic_store_explicit( &b1, 0, memory_order_seq_cst); // memory_order_release);
            rb2 = atomic_load_explicit( &b2, memory_order_seq_cst); // memory_order_acquire);
            int l3 =0 ;
            while (rb2 >= 1 &&l3<WLOOP)
            {
                rb2 = atomic_load_explicit( &b2, memory_order_seq_cst); // memory_order_acquire);
                l3++;
            };
            if(rb2>=1)
                return NULL;
            ry3 = atomic_load_explicit( &y, memory_order_seq_cst);;
            if (ry3 != 1)
            {
                ry4 = atomic_load_explicit( &y, memory_order_seq_cst);
                l3 = 0;
                while (ry4 != 0 && l3<WLOOP)
                {
                    ry4 = atomic_load_explicit( &y, memory_order_seq_cst);;
                    l3++;
                };
                if(ry4!=0)
                    return NULL;
                continue;
            }
        }
        break;
    }
    atomic_store_explicit( &_cc_X, 2, memory_order_relaxed); // memory_order_release);
    rx = atomic_load_explicit( &_cc_X, memory_order_relaxed); // memory_order_acquire);;
    assert(rx == 2);
    atomic_store_explicit( &y, 0, memory_order_seq_cst); // memory_order_release); 
    atomic_store_explicit( &b1, 0, memory_order_seq_cst); // memory_order_release);
  }
  return NULL;
}

void* t1(void *)
{
  int ry1 = -1, ry2 = -1, ry3 = -1, ry4 = -1, rx = -1, rb1 = -1;
  
for(int l1=0;l1<OFLOOP;l1++) {
    for(int l2=0;l2<IFLOOP;l2++) 
    {
        atomic_store_explicit( &b2, 1, memory_order_relaxed); // memory_order_release);
        atomic_store_explicit( &x, 2, memory_order_relaxed);
        ry1 = atomic_load_explicit( &y, memory_order_relaxed);;
        if (ry1 != 0)
        {
            atomic_store_explicit( &b2, 0, memory_order_relaxed); // memory_order_release);
            ry2 = atomic_load_explicit( &y, memory_order_relaxed);
            int l3 =0;
            while (ry2 != 0 && l3 <WLOOP)
            {
                ry2 = atomic_load_explicit( &y, memory_order_relaxed);
                l3++;
            };
            if(ry2!=0) return NULL;
            continue;
        }
        atomic_store_explicit( &y, 2, memory_order_seq_cst); 
        rx = atomic_load_explicit( &x, memory_order_seq_cst);
        if (rx != 2)
        {
            atomic_store_explicit( &b2, 0, memory_order_seq_cst); // memory_order_release);
            rb1 = atomic_load_explicit( &b1, memory_order_seq_cst); // memory_order_acquire);
            int l3 =0 ;
            while (rb1 >= 1 && l3<WLOOP)
            {
                rb1 = atomic_load_explicit( &b1, memory_order_seq_cst); // memory_order_acquire);
                l3++;
            };
            if(rb1>=1) return NULL;
            ry3 = atomic_load_explicit( &y, memory_order_seq_cst);;
            if (ry3 != 2)
            {
                ry4 = atomic_load_explicit( &y, memory_order_seq_cst);
                l3 =0 ;
                while (ry4 != 0 &&l3<WLOOP)
                {
                    ry4 = atomic_load_explicit( &y, memory_order_seq_cst);
                    l3++;
                };
                if(ry4!=0)return NULL;
                continue;
            }
        }
        break;
    }
    atomic_store_explicit( &_cc_X, 1, memory_order_relaxed); // memory_order_release);
    rx = atomic_load_explicit( &_cc_X, memory_order_relaxed); // memory_order_acquire);;
    assert(rx >= 1);
    atomic_store_explicit( &y, 0, memory_order_seq_cst); // memory_order_release); 
    atomic_store_explicit( &b2, 0, memory_order_seq_cst); // memory_order_release);
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
        atomic_init(&b1, 0);
        atomic_init(&b2, 0);
        atomic_init(&x, 0);
        atomic_init(&y, 0);
        atomic_init(&__fence_var, 0);
        atomic_init(&_cc_X, 0);
        pthread_create(&_t1, NULL, t0, NULL );
        pthread_create(&_t2, NULL, t1, NULL);
        pthread_join(_t1, 0);
        pthread_join(_t2, 0);
      return 0;
}



