// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2011-2020 The SV-Benchmarks community
// SPDX-FileCopyrightText: The ESBMC project
//
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define iSet 10000
#define iCheck 10000

static int a;
static int b;

void *setThread(void *param);
void *checkThread(void *param);
void set();
int check();

int main(int argc, char *argv[]) {
    a = 0;
    b = 0;

    // if (argc != 1) {
    //     if (argc != 3) {
    //         fprintf(stderr, USAGE);
    //         exit(-1);
    //     } else {
    //         sscanf(argv[1], "%d", &iSet);
    //         sscanf(argv[2], "%d", &iCheck);
    //     }
    // }

    //printf("iSet = %d\niCheck = %d\n", iSet, iCheck);

    pthread_t setPool[iSet];
    pthread_t checkPool[iCheck];

    for (int i = 0; i < iSet; i++) {
        pthread_create(&setPool[i], NULL, &setThread, NULL);
    }

    for (int i = 0; i < iCheck; i++) {
        pthread_create(&checkPool[i], NULL, &checkThread, NULL);
    }

    for (int i = 0; i < iSet; i++) {
        pthread_join(setPool[i], NULL);
    }

    for (int i = 0; i < iCheck; i++) {
        pthread_join(checkPool[i], NULL);
    }

    return 0;
}
        
void *setThread(void *param) {
    a = 1;
    b = -1;

    return NULL;
}

void *checkThread(void *param) {
    if (! ((a == 0 && b == 0) || (a == 1 && b == -1))) {
        assert(0);
    }

    return NULL;
}

