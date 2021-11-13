/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_cmpeq = (v2_r3 == v2_r3);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v5_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v10 = (v5_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v10, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r4_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v6 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7 = (v6 == 2);
  int v8 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v9_conj = v7 & v8;
  if ( !(v9_conj == 1) ) assert(0);
  return 0;
}