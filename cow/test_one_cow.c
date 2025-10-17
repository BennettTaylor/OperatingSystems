#include "tls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TLS_SIZE 5

pthread_t thread_create, thread_cow;
char write_buffer[TLS_SIZE], read_buffer[TLS_SIZE];

static void *cow() {
  size_t tid = (size_t)pthread_self();
  printf("thread: %lu cloning TLS from thread: %lu\n", tid,
         (size_t)thread_create);
  assert(tls_clone(thread_create) == 0);

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

  pthread_create(&thread_cow, 0, &cow, 0);
  pthread_join(thread_cow, NULL);

  printf("thread: %lu reading from TLS\n", tid);
  assert(tls_read(0, TLS_SIZE, read_buffer) == 0);
  assert(strcmp(read_buffer, str) == 0);
  assert(tls_destroy() == 0);
  return 0;
}

int main() {
  printf("main thread: %lu\n", (size_t)pthread_self());
  pthread_create(&thread_create, 0, &create, 0);
  pthread_join(thread_create, NULL);
}
