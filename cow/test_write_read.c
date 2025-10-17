#include "tls.h"
#include <assert.h>
#include <string.h>

#define TLS_SIZE 5

int main() {
  const char *arg = "hello";
  assert(tls_create(TLS_SIZE) == 0);
  assert(tls_write(0, TLS_SIZE, arg) == 0);

  char read_buffer[TLS_SIZE];
  assert(tls_read(0, TLS_SIZE, read_buffer) == 0);
  assert(strcmp(read_buffer, arg) == 0);
  assert(tls_destroy() == 0);
}
