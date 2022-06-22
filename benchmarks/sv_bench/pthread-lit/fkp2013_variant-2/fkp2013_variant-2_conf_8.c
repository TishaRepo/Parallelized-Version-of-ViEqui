// Source: Azadeh Farzan, Zachary Kincaid, Andreas Podelski: "Inductive Data
// Flow Graphs", POPL 2013

#include <pthread.h>
#include <assert.h>

#define N 8

volatile int x;
volatile int n;

void* thr1(void* arg) {
    assert(x < n);
    return 0;
}

void* thr2(void* arg) {
    int t;
    t = x;
    x = t + 1;
    return 0;
}

int main(int argc, char* argv[]) {
    pthread_t t1, t2;
    int i;
    x = 0;
    n = N;
    pthread_create(&t1, 0, thr1, 0);    
    for (i = 0; i < n; i++) {
	pthread_create(&t2, 0, thr2, 0);
    }
    return 0;
}