/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[5]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r5_1; 
atomic_int atom_3_r1_1; 
atomic_int atom_3_r7_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v4_r5 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v23 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v23, memory_order_seq_cst);
  int v24 = (v4_r5 == 1);
  atomic_store_explicit(&atom_1_r5_1, v24, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[3], 1, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r1 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  atomic_store_explicit(&vars[4], 1, memory_order_seq_cst);
  int v8_r5 = atomic_load_explicit(&vars[4], memory_order_seq_cst);
  int v9_r6 = v8_r5 ^ v8_r5;
  int v12_r7 = atomic_load_explicit(&vars[0+v9_r6], memory_order_seq_cst);
  int v25 = (v6_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v25, memory_order_seq_cst);
  int v26 = (v12_r7 == 0);
  atomic_store_explicit(&atom_3_r7_0, v26, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[4], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r5_1, 0);
  atomic_init(&atom_3_r1_1, 0);
  atomic_init(&atom_3_r7_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v14 = (v13 == 2);
  int v15 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r5_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_3_r7_0, memory_order_seq_cst);
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  if (v22_conj == 1) assert(0);
  return 0;
}