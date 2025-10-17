#include "tls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define TLS_SIZE (PAGE_SIZE * 256)
#define CLONES (128 - 1)

pthread_t thread_create;
pthread_t thread_clone[CLONES];
char write_buffer[PAGE_SIZE], read_buffer[PAGE_SIZE];
const char *str = "this is a random string to test tls";

static void *clone(void *arg) {
  printf("thread: %lu cloning TLS from thread: %lu\n", (size_t)pthread_self(),
         (size_t)thread_create);
  assert(tls_clone(thread_create) == 0);

  tls_read(PAGE_SIZE, PAGE_SIZE, read_buffer);
  assert(memcmp(read_buffer, write_buffer, PAGE_SIZE) == 0);

  long i = (long)arg;
  if (++i < CLONES) {
    pthread_create(&thread_clone[i], 0, &clone, (void *)i);
    pthread_join(thread_clone[i], NULL);
  }
  assert(tls_destroy() == 0);
  return 0;
}

static void *create() {
  assert(tls_create(TLS_SIZE) == 0);
  printf("thread: %lu created TLS\n", (size_t)thread_create);

  strcpy(write_buffer, str);
  tls_write(PAGE_SIZE, PAGE_SIZE, write_buffer);

  pthread_create(&thread_clone[0], 0, &clone, 0);
  pthread_join(thread_clone[0], NULL);

  assert(tls_destroy() == 0);
  return 0;
}

int main() {
  printf("main thread: %lu\n", (size_t)pthread_self());
  pthread_create(&thread_create, 0, &create, 0);
  pthread_join(thread_create, NULL);
}
