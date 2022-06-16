// #include <threads.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
// #include "model-assert.h"
#define LOOP 20 

atomic_int flag1;
atomic_int flag2;
atomic_int _cc_x;
void* t0(void *)
{
int rflag2; rflag2 = -1;
int rx; rx = -1;
for(int l=0;l<LOOP;l++)
        {
atomic_store_explicit( &flag1, 0, memory_order_release);
atomic_store_explicit( &flag1, 1, memory_order_release);
rflag2 = atomic_load_explicit( &flag2, memory_order_acquire);
if(!(rflag2 != 1))return NULL;
;
atomic_store_explicit( &_cc_x, 0, memory_order_release);
rx = atomic_load_explicit( &_cc_x, memory_order_acquire);
assert(rx <= 0);
atomic_store_explicit( &flag1, 0, memory_order_release);
        }
        return NULL;
}
void* t1(void *)
{
int rflag1; rflag1 = -1;
int rx; rx = -1;
for(int l=0;l<LOOP;l++)
        {
atomic_store_explicit( &flag2, 0, memory_order_release);
rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
if(!(rflag1 != 1))return NULL;
;
atomic_store_explicit( &flag2, 1, memory_order_release);
rflag1 = atomic_load_explicit( &flag1, memory_order_acquire);
if(!(rflag1 != 1))return NULL;
;
atomic_store_explicit( &_cc_x, 1, memory_order_release);
rx = atomic_load_explicit( &_cc_x, memory_order_acquire);
assert(rx >= 1);
atomic_store_explicit( &flag2, 0, memory_order_release);
        }
        return NULL;
}
int *propertyChecking(void *arg)
{
return 0;
}
int main(int argc, char **argv)
{
        pthread_t _t1, _t2;

        atomic_init(&_cc_x, 0);
        atomic_init(&flag2, 0);
        atomic_init(&flag1, 0);

        pthread_create(&_t1, NULL, t0, NULL );
        pthread_create(&_t2, NULL, t1, NULL);
        pthread_join(_t1, 0);
        pthread_join(_t2, 0);
return 0;

}


