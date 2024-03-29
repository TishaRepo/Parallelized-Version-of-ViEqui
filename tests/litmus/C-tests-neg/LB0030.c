/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  atomic_store_explicit(&vars[1], v4_r3, memory_order_seq_cst);
  int v18 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v18, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = atomic_load_explicit(&vars[2+v7_r3], memory_order_seq_cst);
  int v12_r6 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v13_r7 = v12_r6 ^ v12_r6;
  int v14_r7 = v13_r7 + 1;
  atomic_store_explicit(&vars[0], v14_r7, memory_order_seq_cst);
  int v19 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v19, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v15 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v17_conj = v15 & v16;
  if ( !(v17_conj == 1) ) assert(0);
  return 0;
}
