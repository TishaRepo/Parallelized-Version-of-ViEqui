/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v4_r5 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v5_r7 = v4_r5 ^ v4_r5;
  int v6_r7 = v5_r7 + 1;
  atomic_store_explicit(&vars[0], v6_r7, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v7 == 2);
  int v9 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v10 = (v9 == 2);
  int v11_conj = v8 & v10;
  if ( !(v11_conj == 1) ) assert(0);
  return 0;
}
