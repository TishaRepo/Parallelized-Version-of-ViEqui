/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r1_1; 
atomic_int atom_0_r4_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_cmpeq = (v2_r1 == 0);
  if (v3_cmpeq)  goto lbl_L00; else goto lbl_L00;
lbl_L00:;
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v5_r2 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v6_r10 = v5_r2 ^ v5_r2;

  int v9_r4 = atomic_load_explicit(&vars[1+v6_r10], memory_order_seq_cst);
  int v13 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v13, memory_order_seq_cst);
  int v14 = (v9_r4 == 0);
  atomic_store_explicit(&atom_0_r4_0, v14, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_0_r4_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_0_r4_0, memory_order_seq_cst);
  int v12_conj = v10 & v11;
  if ( !(v12_conj == 1) ) assert(0);
  return 0;
}
