// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2011-2020 The SV-Benchmarks community
// SPDX-FileCopyrightText: The CSeq project
//
// SPDX-License-Identifier: Apache-2.0

void assume_abort_if_not(int cond) {
  if(!cond) {abort();}
}
#include <assert.h>

#include <stdlib.h>
#include <pthread.h>
#include <string.h>


const int SIGMA = 16;

int *array;
int array_index;


void *thread(void * arg)
{
	array[array_index] = 1;
	return 0;
}


int main()
{
  array_index = -1;
	int tid, sum;
	pthread_t *t;

	t = (pthread_t *)malloc(sizeof(pthread_t) * SIGMA);
	array = (int *)malloc(sizeof(int) * SIGMA);

	assume_abort_if_not(t);
	assume_abort_if_not(array);

	for (tid=0; tid<SIGMA; tid++) {
		array_index++;
		pthread_create(&t[tid], 0, thread, 0);
	}

	for (tid=0; tid<SIGMA; tid++) {
		pthread_join(t[tid], 0);
	}

	for (tid=sum=0; tid<SIGMA; tid++) {
		sum += array[tid];
	}


	return 0;
}

