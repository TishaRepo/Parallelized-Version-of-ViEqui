/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r3_1; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r4_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  atomic_store_explicit(&vars[1], v4_r4, memory_order_seq_cst);
  int v19 = (v2_r3 == 1);
  atomic_store_explicit(&atom_0_r3_1, v19, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = atomic_load_explicit(&vars[0+v7_r3], memory_order_seq_cst);
  int v20 = (v6_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v20, memory_order_seq_cst);
  int v21 = (v10_r4 == 0);
  atomic_store_explicit(&atom_2_r4_0, v21, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r3_1, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r4_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v12 = (v11 == 2);
  int v13 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_2_r4_0, memory_order_seq_cst);
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  if ( !(v18_conj == 1) ) assert(0);
  return 0;
}
