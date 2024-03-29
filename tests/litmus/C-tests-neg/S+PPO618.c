/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v4_r4 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_r5 = v4_r4 ^ v4_r4;
  int v8_r6 = atomic_load_explicit(&vars[2+v5_r5], memory_order_seq_cst);
  int v10_r8 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v11_r9 = v10_r8 ^ v10_r8;
  int v14_r10 = atomic_load_explicit(&vars[3+v11_r9], memory_order_seq_cst);
  int v15_cmpeq = (v14_r10 == v14_r10);
  if (v15_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v23 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v23, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v16 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v17 = (v16 == 2);
  int v18 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v19 = (v18 == 2);
  int v20 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v21_conj = v19 & v20;
  int v22_conj = v17 & v21_conj;
  if ( !(v22_conj == 1) ) assert(0);
  return 0;
}
