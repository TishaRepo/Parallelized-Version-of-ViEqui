/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[5]; 
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
  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_r4 = v4_r3 ^ v4_r3;
  int v8_r5 = atomic_load_explicit(&vars[2+v5_r4], memory_order_seq_cst);
  int v9_r7 = v8_r5 ^ v8_r5;
  int v10_r7 = v9_r7 + 1;
  atomic_store_explicit(&vars[3], v10_r7, memory_order_seq_cst);
  int v12_r9 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v13_r10 = v12_r9 ^ v12_r9;
  int v16_r11 = atomic_load_explicit(&vars[4+v13_r10], memory_order_seq_cst);
  int v17_r13 = v16_r11 ^ v16_r11;
  int v18_r13 = v17_r13 + 1;
  atomic_store_explicit(&vars[0], v18_r13, memory_order_seq_cst);
  int v23 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v23, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[4], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v19 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v20 = (v19 == 2);
  int v21 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v22_conj = v20 & v21;
  if ( !(v22_conj == 1) ) assert(0);
  return 0;
}
