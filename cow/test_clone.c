#include "tls.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#define TLS_SIZE 5

pthread_t thread_create, thread_clone;

static void *clone() {
  printf("thread: %lu cloning TLS from thread: %lu\n", (size_t)pthread_self(),
         (size_t)thread_create);
  assert(tls_clone(thread_create) == 0);
  assert(tls_destroy() == 0);
  return 0;
}

static void *create() {
  assert(tls_create(TLS_SIZE) == 0);
  printf("thread: %lu created TLS\n", (size_t)thread_create);
  pthread_create(&thread_clone, 0, &clone, 0);
  pthread_join(thread_clone, NULL);
  assert(tls_destroy() == 0);
  return 0;
}

int main() {
  printf("main thread: %lu\n", (size_t)pthread_self());
  pthread_create(&thread_create, 0, &create, 0);
  pthread_join(thread_create, NULL);
}
