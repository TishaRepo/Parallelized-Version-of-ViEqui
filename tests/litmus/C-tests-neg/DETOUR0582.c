/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r3_0; 
atomic_int atom_1_r3_1; 
atomic_int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  int v2_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v12 = (v2_r3 == 0);
  atomic_store_explicit(&atom_0_r3_0, v12, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v6_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v13 = (v4_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v13, memory_order_seq_cst);
  int v14 = (v6_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v14, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r3_0, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_1_r4_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atomic_load_explicit(&atom_0_r3_0, memory_order_seq_cst);
  int v8 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v10_conj = v8 & v9;
  int v11_conj = v7 & v10_conj;
  if ( !(v11_conj == 1) ) assert(0);
  return 0;
}
