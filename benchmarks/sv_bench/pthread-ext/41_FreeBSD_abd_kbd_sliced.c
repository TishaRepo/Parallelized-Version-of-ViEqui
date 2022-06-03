#include <assert.h>
#include <pthread.h>

#define Nthreads 10
#define LOOP 10

/*
to correctly model the cv_broadcast(COND) statement "b1_COND := 1;" must be manually changed to "b1_COND$ := 1;" in the abstract BP
*/

// #define cv_wait(c,m){ \
//   c = 0; \
//   __VERIFIER_atomic_release(); \
  // assume(c); \
  // __VERIFIER_atomic_acquire(); }

// #define cv_broadcast(c) c = 1 //overapproximates semantics (for threader)

#define LOCKED 1

// #define mtx_lock(m) __VERIFIER_atomic_acquire();//assert(m==LOCKED); //acquire lock and ensure no other thread unlocked it
#define mtx_unlock(m) __VERIFIER_atomic_release()

// volatile _Bool MTX = !LOCKED;
// __thread _Bool COND = 0; //local
// _Bool buf = 0;
int MTX;
int buf;
int COND;

// void __VERIFIER_atomic_acquire()
// {
// 	// assume(MTX==0);
// 	MTX = 1;
// }

// void __VERIFIER_atomic_release()
// {
// 	// assume(MTX==1);
// 	MTX = 0;
// }

int adb_kbd_receive_packet(){
	// mtx_lock(MTX);
  if (MTX!=0) return 0;
  MTX = 1;
	// mtx_unlock(MTX);
  MTX = 0;
	// cv_broadcast(COND);
  COND = 1;
	return 0; }
	
void akbd_repeat() {
	// mtx_lock(MTX);
  if (MTX!=0) return;
  MTX = 1;
	// mtx_unlock(MTX); 
  MTX = 0; }
	
void akbd_read_char(int wait) {
	// mtx_lock(MTX);
  if (MTX!=0) return;
  MTX = 1;
	if (!buf && wait){
		// cv_wait(COND,MTX);
    COND = 0;
    MTX = 0;
    if (MTX != 0) return;
    MTX = 1;
		assert(COND);}
	if (!buf) {
		// mtx_unlock(MTX);
    MTX = 0;
		return; 	}
	// mtx_unlock(MTX); 
  MTX = 0; }
	
void akbd_clear_state(){
	// mtx_lock(MTX);
  if (MTX!=0) return;
  MTX = 1;
	buf = 0;
	// mtx_unlock(MTX); 
  MTX = 0; }

void* thr1(void* arg){
  for (int i=0; i<LOOP; i++)
  {
    // switch(((i+1)*7-4)%5)
    switch(i%4)
    {
    case 0: adb_kbd_receive_packet(); break;
    case 1: akbd_repeat(); break;
    case 2: akbd_read_char(i); break;
    case 3: akbd_clear_state(); break;
    case 4: for(int j=0; j<LOOP; j++){
        // mtx_lock(MTX);
        if (MTX!=0) return 0;
          MTX = 1;
        buf = !buf;
        // mtx_unlock(MTX);
        MTX = 0;
      }
    }
  }

  return 0;
}

int main(){
  pthread_t t[Nthreads];

  MTX = 0;
  COND = 0;
  buf = 0;

  for(int i=0; i<Nthreads; i++) 
    pthread_create(&t[i], 0, thr1, 0);
}

