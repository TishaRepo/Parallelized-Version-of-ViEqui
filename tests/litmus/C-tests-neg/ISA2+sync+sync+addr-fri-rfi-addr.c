/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_1_r1_1; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r4_0; 
atomic_int atom_2_r7_1; 
atomic_int atom_2_r9_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v24 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v24, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v5_r3 = v4_r1 ^ v4_r1;
  int v8_r4 = atomic_load_explicit(&vars[3+v5_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[3], 1, memory_order_seq_cst);
  int v10_r7 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v11_r8 = v10_r7 ^ v10_r7;
  int v14_r9 = atomic_load_explicit(&vars[0+v11_r8], memory_order_seq_cst);
  int v25 = (v4_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v25, memory_order_seq_cst);
  int v26 = (v8_r4 == 0);
  atomic_store_explicit(&atom_2_r4_0, v26, memory_order_seq_cst);
  int v27 = (v10_r7 == 1);
  atomic_store_explicit(&atom_2_r7_1, v27, memory_order_seq_cst);
  int v28 = (v14_r9 == 0);
  atomic_store_explicit(&atom_2_r9_0, v28, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r4_0, 0);
  atomic_init(&atom_2_r7_1, 0);
  atomic_init(&atom_2_r9_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_2_r4_0, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r7_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_2_r9_0, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  if ( !(v23_conj == 1) ) assert(0);
  return 0;
}
