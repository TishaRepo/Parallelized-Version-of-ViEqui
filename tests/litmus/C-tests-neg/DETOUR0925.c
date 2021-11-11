/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_1_r8_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v3_r5 = v2_r3 ^ v2_r3;
  atomic_store_explicit(&vars[0+v3_r5], 1, memory_order_seq_cst);
  int v5_r8 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v13 = (v5_r8 == 2);
  atomic_store_explicit(&atom_1_r8_2, v13, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_1_r8_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v6 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7 = (v6 == 2);
  int v8 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v9 = (v8 == 3);
  int v10 = atomic_load_explicit(&atom_1_r8_2, memory_order_seq_cst);
  int v11_conj = v9 & v10;
  int v12_conj = v7 & v11_conj;
  if ( !(v12_conj == 1) ) assert(0);
  return 0;
}
