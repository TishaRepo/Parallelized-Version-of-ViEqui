/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r8_1; 

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
  int v5_cmpeq = (v4_r4 == v4_r4);
  if (v5_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v7_r7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v9_r8 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v19 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v19, memory_order_seq_cst);
  int v20 = (v9_r8 == 1);
  atomic_store_explicit(&atom_1_r8_1, v20, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r8_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v11 = (v10 == 2);
  int v12 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v13 = (v12 == 2);
  int v14 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r8_1, memory_order_seq_cst);
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v11 & v17_conj;
  if ( !(v18_conj == 1) ) assert(0);
  return 0;
}
