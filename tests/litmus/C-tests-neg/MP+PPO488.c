/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r10_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = atomic_load_explicit(&vars[2+v3_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);
  int v8_r8 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v10_r9 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v11_cmpeq = (v10_r9 == v10_r9);
  if (v11_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v13_r10 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v20 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v20, memory_order_seq_cst);
  int v21 = (v13_r10 == 0);
  atomic_store_explicit(&atom_1_r10_0, v21, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r10_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v14 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v15 = (v14 == 2);
  int v16 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_1_r10_0, memory_order_seq_cst);
  int v18_conj = v16 & v17;
  int v19_conj = v15 & v18_conj;
  if ( !(v19_conj == 1) ) assert(0);
  return 0;
}
