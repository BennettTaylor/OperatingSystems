#include "tls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TLS_SIZE 5
#define NUM_THREADS 4

pthread_t threads[NUM_THREADS];
char write_buffer[TLS_SIZE], read_buffer[TLS_SIZE];

static void *cow(void *arg) {
  long i = (long)arg;
  size_t tid = (size_t)pthread_self();
  printf("thread: %lu cloning TLS from thread: %lu\n", tid,
         (size_t)threads[i - 1]);
  assert(tls_clone(threads[i - 1]) == 0);

  if (++i < NUM_THREADS) {
    pthread_create(&threads[i], 0, &cow, (void *)i);
    pthread_join(threads[i], NULL);
  }

  const char *str = "world";
  strcpy(write_buffer, str);
  printf("thread: %lu triggering COW\n", tid);
  assert(tls_write(0, TLS_SIZE, write_buffer) == 0);

  printf("thread: %lu reading from TLS after COW\n", tid);
  assert(tls_read(0, TLS_SIZE, read_buffer) == 0);
  assert(strcmp(read_buffer, str) == 0);
  assert(tls_destroy() == 0);
  return 0;
}

static void *create() {
  size_t tid = (size_t)pthread_self();
  assert(tls_create(TLS_SIZE) == 0);
  printf("thread: %lu created TLS\n", tid);

  const char *str = "hello";
  strcpy(write_buffer, str);
  printf("thread: %lu writing to TLS\n", tid);
  assert(tls_write(0, TLS_SIZE, write_buffer) == 0);

  long cow_idx = 1;
  pthread_create(&threads[cow_idx], 0, &cow, (void *)cow_idx);
  pthread_join(threads[cow_idx], NULL);

  printf("thread: %lu reading from TLS\n", tid);
  assert(tls_read(0, TLS_SIZE, read_buffer) == 0);
  assert(strcmp(read_buffer, str) == 0);
  assert(tls_destroy() == 0);
  return 0;
}

int main() {
  printf("main thread: %lu\n", (size_t)pthread_self());
  pthread_t *create_thread = &threads[0];
  pthread_create(create_thread, 0, &create, 0);
  pthread_join(*create_thread, NULL);
}
