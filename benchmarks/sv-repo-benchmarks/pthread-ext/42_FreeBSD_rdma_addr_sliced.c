#include <assert.h>
#include <pthread.h>

/*
to correctly model the cv_broadcast(COND) statement "b1_COND := 1;" must be manually changed to "b1_COND$ := 1;" in the abstract BP
*/

#define LOCKED 1

#define Nthreads 2
#define LOOP 5

// volatile _Bool MTX = !LOCKED;
// __thread _Bool COND = 0;
int MTX;
int COND;

// void __VERIFIER_atomic_acquire()
// {
// 	assume(MTX==0);
// 	MTX = 1;
// }

// void __VERIFIER_atomic_release()
// {
// 	assume(MTX==1);
// 	MTX = 0;
// }

// volatile unsigned int refctr = 0;

int refctr;

inline static void put_client(int client){
	// mtx_lock(MTX);
	if(MTX != 0) return;
	MTX = 1;
	--refctr;
	if (refctr == 0) {
		// cv_broadcast(COND);
		COND = 1; }
	// mtx_unlock(MTX);
	MTX = 0;
  assert(1);
}

inline void rdma_addr_unregister_client(int client){
	// put_client(client);
	// mtx_lock(MTX);
	if(MTX != 0) return;
	MTX = 1;
	if (refctr) {
		// cv_wait(COND,MTX); 
		COND = 0;
		MTX = 0;
		if(MTX != 0) return;
		MTX = 1;
	}
	// mtx_unlock(MTX);
	MTX = 0;
  assert(1);
}

inline static void queue_req(/*struct addr_req *req*/){
	// mtx_lock(MTX);
	if(MTX != 0) return;
		MTX = 1;
	// mtx_unlock(MTX);
	MTX = 0;
  assert(1);
}

inline static void process_req(/*void *ctx, int pending*/){
	// mtx_lock(MTX);
	if(MTX != 0) return;
		MTX = 1;
	// mtx_unlock(MTX);
	MTX = 0;
  assert(1);
}

inline void rdma_resolve_ip(/*struct rdma_addr_client *client,struct sockaddr *src_addr, struct sockaddr *dst_addr,struct rdma_dev_addr *addr, int timeout_ms,void (*callback)(int status, struct sockaddr *src_addr,struct rdma_dev_addr *addr, void *context),void *context*/){
	// mtx_lock(MTX);
	if(MTX != 0) return;
		MTX = 1;
	refctr++;
	// mtx_unlock(MTX);
	MTX = 0;
	for(int i = 0; i < 2; i++){
		if(i){
			// mtx_lock(MTX);
			if(MTX != 0) return;
			MTX = 1;
			refctr--;
			// mtx_unlock(MTX); 
			MTX = 0;}
	}
	
  assert(1);
}

inline void rdma_addr_cancel(/*struct rdma_dev_addr *addr*/){
	// mtx_lock(MTX);
	if(MTX != 0) return;
		MTX = 1;
	// mtx_unlock(MTX);
	MTX = 0;
  assert(1);
}

void* thr1(void* arg){
    for (int i=0; i<LOOP; i++)
  {
    switch(i%5){
    case 0: rdma_addr_unregister_client(i); break;
    case 1: queue_req(); break;
    case 2: process_req(); break;
    case 3: rdma_resolve_ip(); break;
    case 4: rdma_addr_cancel(); break; 
  }
  }

  return 0;
}

int main(){
  pthread_t t[Nthreads];

	MTX = 0;
	COND = 0;
	refctr = 0;

  for(int i=0; i<Nthreads; i++) 
    pthread_create(&t[i], 0, thr1, 0);
}