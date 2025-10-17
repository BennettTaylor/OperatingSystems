#include "../fs.h"
#include <assert.h>
#include <unistd.h>

int main() {
  const char *disk_name = "test_fs";
  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(access(disk_name, F_OK) == 0); // check if disk exists
  assert(remove(disk_name) == 0);
}
