/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_2; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v17 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v17, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v18 = (v4_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v18, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v6 = (v5 == 2);
  int v7 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v8 = (v7 == 2);
  int v9 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v10 = (v9 == 2);
  int v11 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v8 & v14_conj;
  int v16_conj = v6 & v15_conj;
  if ( !(v16_conj == 1) ) assert(0);
  return 0;
}
