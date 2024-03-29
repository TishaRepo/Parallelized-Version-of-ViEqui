/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r3_0; 
atomic_int atom_1_r3_1; 
atomic_int atom_1_r7_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);

  int v2_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v16 = (v2_r3 == 0);
  atomic_store_explicit(&atom_0_r3_0, v16, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_r4 = v4_r3 ^ v4_r3;
  atomic_store_explicit(&vars[0+v5_r4], 1, memory_order_seq_cst);
  int v7_r7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v17 = (v4_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v17, memory_order_seq_cst);
  int v18 = (v7_r7 == 1);
  atomic_store_explicit(&atom_1_r7_1, v18, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r3_0, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_1_r7_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v8 = atomic_load_explicit(&atom_0_r3_0, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v10 = (v9 == 2);
  int v11 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r7_1, memory_order_seq_cst);
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v8 & v14_conj;
  if ( !(v15_conj == 1) ) assert(0);
  return 0;
}
