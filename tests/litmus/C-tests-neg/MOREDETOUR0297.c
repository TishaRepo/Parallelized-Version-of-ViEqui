/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r5_5; 
atomic_int atom_1_r4_4; 
atomic_int atom_1_r1_2; 
atomic_int atom_2_r5_2; 
atomic_int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 4, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 5, memory_order_seq_cst);
  int v2_r5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_cmpeq = (v2_r5 == v2_r5);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v23 = (v2_r5 == 5);
  atomic_store_explicit(&atom_0_r5_5, v23, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);

  int v7_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v24 = (v7_r4 == 4);
  atomic_store_explicit(&atom_1_r4_4, v24, memory_order_seq_cst);
  int v25 = (v5_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v25, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v9_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v10_r3 = v9_r1 ^ v9_r1;
  int v11_r3 = v10_r3 + 1;
  atomic_store_explicit(&vars[0], v11_r3, memory_order_seq_cst);

  int v13_r5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v13_r5 == 2);
  atomic_store_explicit(&atom_2_r5_2, v26, memory_order_seq_cst);
  int v27 = (v9_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v27, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r5_5, 0);
  atomic_init(&atom_1_r4_4, 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_2_r5_2, 0);
  atomic_init(&atom_2_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v14 = atomic_load_explicit(&atom_0_r5_5, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r4_4, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_2_r5_2, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  if ( !(v22_conj == 1) ) assert(0);
  return 0;
}
