// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2011-2020 The SV-Benchmarks community
// SPDX-FileCopyrightText: The CSeq project
//
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <pthread.h>

#define SIGMA 20


int array[SIGMA];
int array_index;


void *thread(void * arg)
{
	int ai = *((int*)(arg));
	array_index = ai;
	array[array_index] = 1;
	return NULL;
}


int main()
{
	int tid=0, sum=0;
	pthread_t t[SIGMA];

	array_index = -1;
	for (int i=0; i<SIGMA; i++) {
		array[i] = 0;
	}

	for (tid=0; tid<SIGMA; tid++) {
		pthread_create(&t[tid], 0, thread, (void*)&tid);
	}

	for (tid=0; tid<SIGMA; tid++) {
		pthread_join(t[tid], 0);
	}

	for (tid=sum=0; tid<SIGMA; tid++) {
		sum += array[tid];
	}


	return 0;
}

