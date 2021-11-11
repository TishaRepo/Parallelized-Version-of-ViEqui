/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r5_2; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  atomic_store_explicit(&vars[1], v4_r3, memory_order_seq_cst);
  int v16 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v16, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v9_r5 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v10_cmpeq = (v9_r5 == v9_r5);
  if (v10_cmpeq)  goto lbl_LC01; else goto lbl_LC01;
lbl_LC01:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v17 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v17, memory_order_seq_cst);
  int v18 = (v9_r5 == 2);
  atomic_store_explicit(&atom_1_r5_2, v18, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r5_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_1_r5_2, memory_order_seq_cst);
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  if ( !(v15_conj == 1) ) assert(0);
  return 0;
}
