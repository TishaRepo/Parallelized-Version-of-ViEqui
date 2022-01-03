// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2011-2020 The SV-Benchmarks community
// SPDX-FileCopyrightText: The ESBMC project
//
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define iSet 4
#define iCheck 1

static int a;
static int b;

void *setThread(void *param);
void *checkThread(void *param);
void set();
int check();

int main(int argc, char *argv[]) {
    int i;

    a = 0;
    b = 0;

    pthread_t setPool[iSet];
    pthread_t checkPool[iCheck];

    for (i = 0; i < iSet; i++) {
        pthread_create(&setPool[i], ((void *)0), &setThread, ((void *)0));
    }

    for (i = 0; i < iCheck; i++) {
        pthread_create(&checkPool[i], ((void *)0), &checkThread, ((void *)0));
    }

    for (i = 0; i < iSet; i++) {
        pthread_join(setPool[i], ((void *)0));
    }

    for (i = 0; i < iCheck; i++) {
        pthread_join(checkPool[i], ((void *)0));
    }

    return 0;
}

void *setThread(void *param) {
    a = 1;
    b = -1;

    return ((void *)0);
}

void *checkThread(void *param) {
    if (! ((a == 0 && b == 0) || (a == 1 && b == -1))) {
        assert(0);
    }

    return ((void *)0);
}
