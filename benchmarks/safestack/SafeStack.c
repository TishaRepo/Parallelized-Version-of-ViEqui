

#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#define MAX_SPIN 2

struct SafeStackItem
{
  int Value;
  atomic_int Next;
};

class SafeStack
{
  atomic_int head;
  atomic_int count;

public:
  SafeStackItem array[3];

  void init(int pushCount)
  {
      atomic_init(&count, pushCount);
      atomic_init(&head, 0);
      for (int i = 0; i < pushCount-1; i++)
          atomic_init(&array[i].Next, (i+1));
      atomic_init(&array[pushCount-1].Next, (-1));
  }

  int Pop()
  {
      int spin_counter = 0;
      while (atomic_load_explicit(&count, memory_order_seq_cst) > 1 && spin_counter < MAX_SPIN)
      {
          spin_counter++;

          int head1 = atomic_load_explicit(&head, memory_order_seq_cst);
          int next1 = atomic_exchange_explicit(&array[head1].Next, -1, memory_order_seq_cst);

          if (next1 >= 0)
          {
              int head2 = head1;
              if (atomic_compare_exchange_strong_explicit(&head, &head2, next1, memory_order_seq_cst, memory_order_seq_cst))
              {
                  atomic_fetch_sub_explicit(&count, 1, memory_order_seq_cst);
                  return head1;
              }
              else
              {
                  atomic_exchange_explicit(&array[head1].Next, next1, memory_order_seq_cst);
              }
          }
      }

      return -1;
  }

  void Push(int index)
  {
      int spin_counter = 0;

      int head1 = atomic_load_explicit(&head, memory_order_seq_cst);
      do
      {
          spin_counter++;
          atomic_store_explicit(&array[index].Next, head1, memory_order_release);
         
      } while (!atomic_compare_exchange_strong_explicit(&head, &head1, index, memory_order_seq_cst, memory_order_seq_cst) && spin_counter <= MAX_SPIN);
      atomic_fetch_add_explicit(&count, 1, memory_order_seq_cst);
  }
};

#define NUM_THREADS 3
SafeStack stack;

void* thread(void* arg)
{
  int idx = (int)(size_t)arg;
    for (size_t i = 0; i != 2; i += 1)
    {
        int elem;
        for (int spin_counter = 0; spin_counter < MAX_SPIN; spin_counter++)
        {
            elem = stack.Pop();
            if (elem >= 0)
                break;
            if (spin_counter == MAX_SPIN-1) return NULL;
        }

        stack.array[elem].Value = idx;
        assert(stack.array[elem].Value == idx);

        stack.Push(elem);
    }
    return NULL;
}

int main()
{
  pthread_t threads[NUM_THREADS];

  stack.init(3);

  for (unsigned i = 0; i < NUM_THREADS; ++i) {
    pthread_create(&threads[i], NULL, thread, (void*)i);
  }
  
  for (unsigned i = 0; i < NUM_THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }
  
  return 0;
}

