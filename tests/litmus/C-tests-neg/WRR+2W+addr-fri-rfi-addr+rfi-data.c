/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_1_r1_2; 
atomic_int atom_1_r4_0; 
atomic_int atom_1_r7_1; 
atomic_int atom_1_r9_0; 
atomic_int atom_2_r3_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = atomic_load_explicit(&vars[1+v3_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v8_r7 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r8 = v8_r7 ^ v8_r7;
  int v12_r9 = atomic_load_explicit(&vars[2+v9_r8], memory_order_seq_cst);
  int v29 = (v2_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v29, memory_order_seq_cst);
  int v30 = (v6_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v30, memory_order_seq_cst);
  int v31 = (v8_r7 == 1);
  atomic_store_explicit(&atom_1_r7_1, v31, memory_order_seq_cst);
  int v32 = (v12_r9 == 0);
  atomic_store_explicit(&atom_1_r9_0, v32, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v14_r3 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v15_r4 = v14_r3 ^ v14_r3;
  int v16_r4 = v15_r4 + 1;
  atomic_store_explicit(&vars[0], v16_r4, memory_order_seq_cst);
  int v33 = (v14_r3 == 1);
  atomic_store_explicit(&atom_2_r3_1, v33, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_1_r4_0, 0);
  atomic_init(&atom_1_r7_1, 0);
  atomic_init(&atom_1_r9_0, 0);
  atomic_init(&atom_2_r3_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v17 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v18 = (v17 == 2);
  int v19 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_1_r7_1, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_1_r9_0, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_2_r3_1, memory_order_seq_cst);
  int v24_conj = v22 & v23;
  int v25_conj = v21 & v24_conj;
  int v26_conj = v20 & v25_conj;
  int v27_conj = v19 & v26_conj;
  int v28_conj = v18 & v27_conj;
  if ( !(v28_conj == 1) ) assert(0);
  return 0;
}
