#include <assert.h>
#include <pthread.h>

int x;
int y;

void *t0(void *arg){
  int a = x;
  y = 1;
  return NULL;
}

void *t1(void *arg){
  int c = y;
  x = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0;
  pthread_t thr1;

  x = 0;
  y = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  return 0;
}

// T1 - Rx(0).Ry(0).Wy=1.Wx=1  1.2.1.2   po_pre = 2:Ry
// T2 - Rx(0).Wy=1.Ry(1).Wx=1  1.1.2.2   po_pre = 1:Rx.1:Wy.2:Ry
// T3 - Ry(0).Wx=1.Rx(1).Wy=1  2.2.1.1