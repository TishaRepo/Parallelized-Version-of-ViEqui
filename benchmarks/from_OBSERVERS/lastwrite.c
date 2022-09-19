#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define N 500

atomic_int var;

void *writer(void *arg) {
  atomic_store(&var, (int)(intptr_t)arg);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t t[N];

  atomic_init(&var, 0);

  for (int i = 0; i < N; i++) {
    pthread_create(&t[i], NULL, writer, (void*)(intptr_t)(i+1));
  }

  for (int i = 0; i < N; i++) {
    pthread_join(t[i], NULL);
  }

  return atomic_load(&var);
}
