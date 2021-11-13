/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_1_r1_2; 
atomic_int atom_1_r2_3; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v2_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v8, memory_order_seq_cst);
  int v9 = (v4_r2 == 3);
  atomic_store_explicit(&atom_1_r2_3, v9, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 1);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_1_r2_3, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_1_r2_3, memory_order_seq_cst);
  int v7_conj = v5 & v6;
  if ( !(v7_conj == 1) ) assert(0);
  return 0;
}