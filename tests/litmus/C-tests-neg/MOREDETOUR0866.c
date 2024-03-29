/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r3_0; 
atomic_int atom_2_r3_2; 
atomic_int atom_3_r4_4; 
atomic_int atom_3_r1_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 5, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_cmpeq = (v2_r1 == v2_r1);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v5_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 4, memory_order_seq_cst);
  int v24 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v24, memory_order_seq_cst);
  int v25 = (v5_r3 == 0);
  atomic_store_explicit(&atom_1_r3_0, v25, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  int v7_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v7_r3 == 2);
  atomic_store_explicit(&atom_2_r3_2, v26, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v9_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);

  int v11_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v27 = (v11_r4 == 4);
  atomic_store_explicit(&atom_3_r4_4, v27, memory_order_seq_cst);
  int v28 = (v9_r1 == 2);
  atomic_store_explicit(&atom_3_r1_2, v28, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r3_0, 0);
  atomic_init(&atom_2_r3_2, 0);
  atomic_init(&atom_3_r4_4, 0);
  atomic_init(&atom_3_r1_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v12 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v13 = (v12 == 5);
  int v14 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r3_0, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_2_r3_2, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_3_r4_4, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  int v23_conj = v13 & v22_conj;
  if ( !(v23_conj == 1) ) assert(0);
  return 0;
}
