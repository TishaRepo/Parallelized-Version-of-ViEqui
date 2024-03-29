/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r5_2; 
atomic_int atom_1_r7_0; 
atomic_int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  int v2_r5 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v19 = (v2_r5 == 2);
  atomic_store_explicit(&atom_0_r5_2, v19, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 4, memory_order_seq_cst);
  int v4_r4 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v5_r6 = v4_r4 ^ v4_r4;
  int v8_r7 = atomic_load_explicit(&vars[0+v5_r6], memory_order_seq_cst);
  int v20 = (v8_r7 == 0);
  atomic_store_explicit(&atom_1_r7_0, v20, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 3, memory_order_seq_cst);
  int v21 = (v10_r1 == 2);
  atomic_store_explicit(&atom_2_r1_2, v21, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r5_2, 0);
  atomic_init(&atom_1_r7_0, 0);
  atomic_init(&atom_2_r1_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atomic_load_explicit(&atom_0_r5_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r7_0, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v14 = (v13 == 4);
  int v15 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
  int v16_conj = v14 & v15;
  int v17_conj = v12 & v16_conj;
  int v18_conj = v11 & v17_conj;
  if ( !(v18_conj == 1) ) assert(0);
  return 0;
}
