/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r5_2; 
atomic_int atom_1_r3_3; 
atomic_int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  int v2_r5 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v13 = (v2_r5 == 2);
  atomic_store_explicit(&atom_0_r5_2, v13, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_cmpeq = (v4_r3 == v4_r3);
  if (v5_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v7_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v14 = (v4_r3 == 3);
  atomic_store_explicit(&atom_1_r3_3, v14, memory_order_seq_cst);
  int v15 = (v7_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v15, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 3, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r5_2, 0);
  atomic_init(&atom_1_r3_3, 0);
  atomic_init(&atom_1_r4_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v8 = atomic_load_explicit(&atom_0_r5_2, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_1_r3_3, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v11_conj = v9 & v10;
  int v12_conj = v8 & v11_conj;
  if ( !(v12_conj == 1) ) assert(0);
  return 0;
}
