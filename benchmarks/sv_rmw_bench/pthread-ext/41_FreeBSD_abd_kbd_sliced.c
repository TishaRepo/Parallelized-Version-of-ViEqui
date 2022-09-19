#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS 5
#define LOOP_LIMIT 5

/*
to correctly model the cv_broadcast(COND) statement "b1_COND := 1;" must be manually changed to "b1_COND$ := 1;" in the abstract BP
*/

atomic_int MTX;
int COND;

bool acquire()
{
	int e = 0, v = 1;
	return atomic_compare_exchange_strong_explicit(&MTX, &e, v, memory_order_seq_cst, memory_order_seq_cst);
}

bool release()
{
	int e = 1, v = 0;
	return atomic_exchange_explicit(&MTX, v, memory_order_seq_cst);
}

bool cv_wait(){
  int ctr = 0;
  COND = 0;
  release();
  while (ctr++ < LOOP_LIMIT) {
    if (COND == 0)
      continue;

    return acquire(); 
  }

  return false;
}

void cv_broadcast() {
  COND = 1; //overapproximates semantics (for threader)
}

int buf = 0;

int adb_kbd_receive_packet(){
	acquire();
	release();
	cv_broadcast(COND);
	return 0; 
}
	
void akbd_repeat() {
	acquire();
	release(); 
}
	
void akbd_read_char(int wait) {
	acquire();
	if (!buf && wait){
		cv_wait();
		assert(COND);
  }
	if (!buf) {
		release();
		return; 	
  }
	release(); 
}
	
void akbd_clear_state(){
	acquire();
	buf = 0;
	release(); }

void* thr1(void* arg){
  int ctr = 0;
  while(ctr++ < LOOP_LIMIT)
  {
    // switch((ctr+2+ctr*2)%5)
    switch((ctr+1)%5)
    {
    case 0: adb_kbd_receive_packet(); break;
    case 1: akbd_repeat(); break;
    case 2: akbd_read_char(ctr); break;
    case 3: akbd_clear_state(); break;
    case 4: {
        acquire();
        buf = !buf;
        release();
      }
    }
  }

  return 0;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&MTX, 0);
  COND = 0;
  buf = 0;

  for (int n = 0; n < NUM_THREADS; n++)
    pthread_create(&t[n], 0, thr1, 0);
}

