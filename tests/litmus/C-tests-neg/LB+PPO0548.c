/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  atomic_store_explicit(&vars[1], v4_r3, memory_order_seq_cst);
  int v22 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v22, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);
  int v9_r6 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v10_r7 = v9_r6 ^ v9_r6;
  int v13_r8 = atomic_load_explicit(&vars[3+v10_r7], memory_order_seq_cst);
  int v15_r10 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v23 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v23, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v16 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v17 = (v16 == 2);
  int v18 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  if ( !(v21_conj == 1) ) assert(0);
  return 0;
}
