#include "tls.h"
#include <assert.h>
#include <stdio.h>

#define TLS_SIZE 5

char read_buffer[TLS_SIZE], write_buffer[TLS_SIZE];

int main() {
  printf("thread id: %lu\n", (size_t)pthread_self());

  // tls_create
  assert(tls_create(0) == -1);
  assert(tls_create(TLS_SIZE) == 0); // actual creation
  assert(tls_create(TLS_SIZE) == -1);

  // tls_write overflow
  assert(tls_write(TLS_SIZE, TLS_SIZE, write_buffer) == -1);

  // tls_read overflow
  assert(tls_read(TLS_SIZE, TLS_SIZE, read_buffer) == -1);

  // tls_clone
  assert(tls_clone(pthread_self()) == -1);

  // tls_destroy
  assert(tls_destroy() == 0); // actual destruction
  assert(tls_destroy() == -1);

  // tls_read after destroy
  assert(tls_read(0, TLS_SIZE, read_buffer) == -1);

  // tls_write after destroy
  assert(tls_write(0, TLS_SIZE, write_buffer) == -1);

  // tls_clone after destroy
  assert(tls_clone(pthread_self()) == -1);
}
