/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r14_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = atomic_load_explicit(&vars[2+v3_r3], memory_order_seq_cst);
  int v7_r6 = v6_r4 ^ v6_r4;
  atomic_store_explicit(&vars[3+v7_r6], 1, memory_order_seq_cst);
  int v9_r9 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v10_r10 = v9_r9 ^ v9_r9;
  atomic_store_explicit(&vars[0+v10_r10], 1, memory_order_seq_cst);
  int v12_r13 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v14_r14 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v21 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v21, memory_order_seq_cst);
  int v22 = (v14_r14 == 1);
  atomic_store_explicit(&atom_1_r14_1, v22, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r14_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v15 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v16 = (v15 == 2);
  int v17 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_1_r14_1, memory_order_seq_cst);
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  if ( !(v20_conj == 1) ) assert(0);
  return 0;
}
