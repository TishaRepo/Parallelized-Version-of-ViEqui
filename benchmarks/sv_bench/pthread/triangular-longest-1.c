// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2011-2020 The SV-Benchmarks community
// SPDX-FileCopyrightText: 2018 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include <pthread.h>


#include <assert.h>

int i, j;

#define NUM 20
#define LIMIT (2*NUM+6)

void *t1(void *arg) {
  for (int k = 0; k < NUM; k++) {
    i = j + 1;
  }
  pthread_exit(NULL);
}

void *t2(void *arg) {
  for (int k = 0; k < NUM; k++) {
    j = i + 1;
  }
  pthread_exit(NULL);
}

int main(int argc, char **argv) {
  i = 3;
  j = 6;
  pthread_t id1, id2;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);

  int condI = i > LIMIT;

  int condJ = j > LIMIT;

  if (condI || condJ) {
    assert(0);
  }

  return 0;
}

