#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS 10
#define LOOP_LIMIT 4

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

int refctr;

static void put_client(int client){
	acquire();
	--refctr;
	if (refctr == 0) {
		cv_broadcast(); }
	release();
//   assert(1);
}

void rdma_addr_unregister_client(int client){
	put_client(client);
	acquire();
	if (refctr) {
		cv_wait(); }
		assert(COND);
	release();
//   assert(1);
}

static void queue_req(/*struct addr_req *req*/){
	acquire();
	release();
//   assert(1);
}

static void process_req(/*void *ctx, int pending*/){
	acquire();
	release();
//   assert(1);
}

void rdma_resolve_ip(int v/*struct rdma_addr_client *client,struct sockaddr *src_addr, struct sockaddr *dst_addr,struct rdma_dev_addr *addr, int timeout_ms,void (*callback)(int status, struct sockaddr *src_addr,struct rdma_dev_addr *addr, void *context),void *context*/){
	acquire();
	refctr++;
	release();
	if(v){
		acquire();
		refctr--;
		release(); }
//   assert(1);
}

void rdma_addr_cancel(/*struct rdma_dev_addr *addr*/){
	acquire();
	release();
//   assert(1);
}

void* thr1(void* arg){
  int ctr = 0;
  while(ctr++ < LOOP_LIMIT)
    switch((ctr+3)%5){
    case 0: rdma_addr_unregister_client(ctr); break;
    case 1: queue_req(); break;
    case 2: process_req(); break;
    case 3: rdma_resolve_ip(ctr); break;
    case 4: rdma_addr_cancel(); break; 
  }

  return 0;
}

int main(){
  pthread_t t[NUM_THREADS];

  atomic_init(&MTX, 0);
  COND = 0;
  refctr = 0;

  for (int n = 0; n < NUM_THREADS; n++)
    pthread_create(&t[n], 0, thr1, 0);
}