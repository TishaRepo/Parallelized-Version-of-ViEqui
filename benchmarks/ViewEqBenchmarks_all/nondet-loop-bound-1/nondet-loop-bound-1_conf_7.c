#include <pthread.h>
#include "assert.h"
#define LOOP 7

volatile int x;
volatile int n;

void* thr1(void* arg) {
    assert(x < n);
}

void* thr2(void* arg) {
    int t;
    t = x;
    x = t + 1;
}

int main(int argc, char* argv[]) {
    pthread_t t1, t2;
    int i;
    x = 0;
    n = LOOP;
    pthread_create(&t1, 0, thr1, 0);
    for (i = 0; i < n; i++) {
	     pthread_create(&t2, 0, thr2, 0);
    }
    return 0;
}
