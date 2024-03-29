/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r4_3; 
atomic_int atom_1_r5_2; 
atomic_int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v2_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r5 = v2_r4 ^ v2_r4;
  int v4_r5 = v3_r5 + 1;
  atomic_store_explicit(&vars[1], v4_r5, memory_order_seq_cst);
  int v16 = (v2_r4 == 3);
  atomic_store_explicit(&atom_0_r4_3, v16, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_r3 = v6_r1 ^ v6_r1;
  int v8_r3 = v7_r3 + 1;
  atomic_store_explicit(&vars[0], v8_r3, memory_order_seq_cst);

  int v10_r5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v17 = (v10_r5 == 2);
  atomic_store_explicit(&atom_1_r5_2, v17, memory_order_seq_cst);
  int v18 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v18, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r4_3, 0);
  atomic_init(&atom_1_r5_2, 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = atomic_load_explicit(&atom_0_r4_3, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r5_2, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  if ( !(v15_conj == 1) ) assert(0);
  return 0;
}
