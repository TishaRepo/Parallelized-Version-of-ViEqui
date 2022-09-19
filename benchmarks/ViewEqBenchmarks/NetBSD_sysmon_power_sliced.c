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

#define PSWITCH_EVENT_RELEASED 1
#define PENVSYS_EVENT_NORMAL 2
#define POWER_EVENT_RECVDICT 3

int LOADED;
int LOADING;

int sysmon_queue_power_event(int k){
	assert(MTX == 1);
  // assert(1);
	if (k)
		return 0;
	return 1; }

int sysmon_get_power_event(int k){
	assert(MTX == 1);
  // assert(1);
	if (k)	
		return 0;
	return 1; }

int sysmon_power_daemon_task(int k){
	if (k) return k;
	acquire();
	switch (k) {
	case PSWITCH_EVENT_RELEASED:
		assert(MTX == 1);
		if (k) {
			release();
			goto out;}
		break;
	case PENVSYS_EVENT_NORMAL:
		assert(MTX == 1);
		if (k) {
			release();
			goto out;}
		break;
	default:
		release();
		goto out;}
	sysmon_queue_power_event(k);
	if (k) {
		release();
		goto out;} 
	else {
		cv_broadcast();
		release();}
	out:
  // assert(1);
	return k; }

void sysmonopen_power(int k){
	acquire();
	if (k)
		assert(MTX == 1);
	release();
  // assert(1);
}

void sysmonclose_power(int k){
	acquire();
	assert(MTX == 1);
	release();
  // assert(1);
}

void sysmonread_power(int k){
	if (k){
		acquire();
		for (;;) {
			if (sysmon_get_power_event(k)) {
				break;}
			if (k) {
				break;}
			cv_wait();
      assert(COND); }
		release(); }
  // assert(1);
}

void sysmonpoll_power(int k){
	if(k){
		acquire();
		release(); }
  // assert(1);
}

void filt_sysmon_power_rdetach(){
	acquire();
	release();
  // assert(1);
}

void filt_sysmon_power_read(){
	acquire();
	release();
  // assert(1);
}

void sysmonkqfilter_power(){
	acquire();
	release();
  // assert(1);
}

void sysmonioctl_power(int k){
	switch (k) {
	case POWER_EVENT_RECVDICT:
		acquire();
		if (k) {
			release();
			break;}
		release();
		acquire();
		release();
		break; }
  // assert(1);
}

void* thr1(void* arg){
  int ctr = 0;
  while(ctr++ < LOOP_LIMIT)
    switch((ctr+6)%9){
    case 0: sysmon_power_daemon_task(ctr); break;
    case 1: sysmonopen_power(ctr); break;
    case 2: sysmonclose_power(ctr); break;
    case 3: sysmonread_power(ctr); break;
    case 4: sysmonpoll_power(ctr); break;
    case 5: filt_sysmon_power_rdetach(); break;
    case 6: filt_sysmon_power_read(); break;
    case 7: sysmonkqfilter_power(); break;
    case 8: sysmonioctl_power(ctr); break; }

	return NULL;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&MTX, 0);
  COND = 0;

  LOADED = 0;
  LOADING = 0;

  int arg[NUM_THREADS];
  for (int n = 0; n < NUM_THREADS; n++) {
	arg[n] = n;
    pthread_create(&t[n], 0, thr1, &arg[n]);
  }
}

