// Source: Azadeh Farzan, Zachary Kincaid, Andreas Podelski: "Inductive Data
// Flow Graphs", POPL 2013

#include <pthread.h>
#include <assert.h>

volatile int x;
#define N 5

void* thr1(void* arg) {
    assert(x <= N);
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
    pthread_create(&t1, 0, thr1, 0);    
    for (i = 0; i < N; i++) {
	pthread_create(&t2, 0, thr2, 0);
    }
    return 0;
}
