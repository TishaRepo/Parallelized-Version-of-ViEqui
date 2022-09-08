

#include <atomic>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#define NUM_THREADS 5
#define MAX_SPIN 5

using namespace std;

struct SafeStackItem
{
  int Value;
  atomic<int> Next;
};

class SafeStack
{
  atomic<int> head;
  atomic<int> count;

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
      while (count.load(memory_order_seq_cst) > 1 && spin_counter < MAX_SPIN)
      {
          spin_counter++;

          int head1 = head.load(memory_order_seq_cst);
          int next1 = array[head1].Next.exchange(-1, memory_order_seq_cst);

          if (next1 >= 0)
          {
              int head2 = head1;
              if (atomic_compare_exchange_strong_explicit(&head, &head2, next1, memory_order_seq_cst, memory_order_seq_cst))
              {
                  count.fetch_sub(1, memory_order_seq_cst);
                  return head1;
              }
              else
              {
                array[head1].Next.exchange(next1, memory_order_seq_cst);
              }
          }
      }

      return -1;
  }

  void Push(int index)
  {
      int spin_counter = 0;

      int head1 = head.load(memory_order_seq_cst);
      do
      {
          spin_counter++;
          array[index].Next.store(head1, memory_order_release);
         
      } while (!atomic_compare_exchange_strong_explicit(&head, &head1, index, memory_order_seq_cst, memory_order_seq_cst) && spin_counter <= MAX_SPIN);
      count.fetch_add(1, memory_order_seq_cst);
  }
};

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

