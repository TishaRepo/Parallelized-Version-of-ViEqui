/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r3_0; 
atomic_int atom_1_r6_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  int v2_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v13 = (v2_r3 == 0);
  atomic_store_explicit(&atom_0_r3_0, v13, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v6_r5 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v7_cmpeq = (v6_r5 == v6_r5);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v9_r6 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v14 = (v9_r6 == 0);
  atomic_store_explicit(&atom_1_r6_0, v14, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r3_0, 0);
  atomic_init(&atom_1_r6_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = atomic_load_explicit(&atom_0_r3_0, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_1_r6_0, memory_order_seq_cst);
  int v12_conj = v10 & v11;
  if ( !(v12_conj == 1) ) assert(0);
  return 0;
}