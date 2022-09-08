// nidhuggc: -sc -optimal -no-assume-await -unroll=4
/* Test based on the SafeStack benchmark from SCTBench. */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>

#define N 5 // 3,3,1 - to reproduce the bug
#define LIMIT 4

// const unsigned iters[] = {N};
unsigned iters[N];

// #define NUM_THREADS (sizeof(iters)/sizeof(*iters))
#define NUM_THREADS N
#define STACK_SIZE N // NUM_THREADS

void __VERIFIER_assume(int truth);

typedef struct SafeStackItem {
    atomic_int Value;
    atomic_int Next;
} SafeStackItem;

typedef struct SafeStack {
    SafeStackItem array[STACK_SIZE];
    atomic_int head;
    atomic_int count;
} SafeStack;

SafeStack stack;

void Init(int pushCount) {
    int i;
    atomic_init(&stack.count, pushCount);
    atomic_init(&stack.head, 0);
    for (i = 0; i < pushCount - 1; i++) {
	    atomic_init(&stack.array[i].Next, i + 1);
    }
    atomic_init(&stack.array[pushCount - 1].Next, -1);
}

int Pop(void) {
    int loop_iter = 0;
    while (atomic_load(&stack.count) > 1 && loop_iter++ < LIMIT) {
        int head1 = atomic_load(&stack.head);
        int next1 = atomic_exchange(&stack.array[head1].Next, -1);

        if (next1 >= 0) {
            int head2 = head1;
            if (atomic_compare_exchange_strong(&stack.head, &head2, next1)) {
                atomic_fetch_sub(&stack.count, 1);
                return head1;
            } else {
                atomic_exchange(&stack.array[head1].Next, next1);
            }
        }
    }
    return -1;
}

void Push(int index) {
    int loop_iter = 0;
    int head1 = atomic_load(&stack.head);
    do {
	    atomic_store(&stack.array[index].Next, head1);
    } while (!(atomic_compare_exchange_strong(&stack.head, &head1, index)) && loop_iter++ < LIMIT);
    atomic_fetch_add(&stack.count, 1);
}


void* thread(void* arg) {
    int idx = (int)(uintptr_t)arg;
    uintptr_t i = iters[idx];

    int outer_loop_iter = 0;
    int inner_loop_iter = 0;
    for (;;) {
        if (i-- == 0) return NULL;
        int elem;
        do { elem = Pop(); } while (elem == -1 && inner_loop_iter++ < LIMIT);

        // Check for double-pop. Requires N=3,3,1 (or greater) to
        // reproduce original bug
        stack.array[elem].Value = idx;
        assert(stack.array[elem].Value == idx);

        if (i-- == 0) return NULL;
        Push(elem);

        if (outer_loop_iter++ >= LIMIT) 
            break;
    }
    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    uintptr_t i;
    
    Init(STACK_SIZE);

    for (int i = 0; i < N; i++) {
        iters[i] = N;
    }
    // iters[0] = 3; 
    // iters[1] = 3; 
    // iters[2] = 1; 

    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, thread, (void*) i);
    }

    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
