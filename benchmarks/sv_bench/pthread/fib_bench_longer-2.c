// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2011-2020 The SV-Benchmarks community
// SPDX-FileCopyrightText: The ESBMC project
//
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>


#include <pthread.h>

int i, j;

#define NUM 6

void *
t1(void* arg)
{
  int k = 0;

  for (k = 0; k < NUM; k++) {
    i+=j;
  }
  pthread_exit(NULL);
}

void *
t2(void* arg)
{
  int k = 0;

  for (k = 0; k < NUM; k++) {
    j+=i;
  }
  pthread_exit(NULL);
}

int
main(int argc, char **argv)
{
  pthread_t id1, id2;

  i = 1;
  j = 1;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);

  int condI = i >= 377;

  int condJ = j >= 377;

  if (condI || condJ) {
    assert(0);
  }

  return 0;
}
