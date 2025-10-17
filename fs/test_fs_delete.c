#include "../fs.h"
#include <assert.h>
#include <stdlib.h>

#define BYTES_KB 1024
#define BYTES_MB (1024 * BYTES_KB)

int main() {
  const char *disk_name = "test_fs";
  const char *file_name = "test_file";
  const char *cr8del_file = "cr8del_file";
  char *buf;
  int fd;

  buf = malloc(BYTES_MB);
  memset(buf, 'a', BYTES_MB);

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);

  assert(fs_create(file_name) == 0);
  fd = fs_open(file_name);
  assert(fd >= 0);
  assert(fs_delete(file_name) == -1); // file is open
  assert(fs_close(fd) == 0);
  assert(fs_delete(file_name) == 0);
  assert(fs_delete(file_name) == -1); // file does not exist

  assert(fs_create(file_name) == 0);
  assert(umount_fs(disk_name) == 0);
  assert(fs_delete(file_name) == -1); // disk is not mounted

  // 7.5) Create and delete a 1 MiB file many times while another file exists
  assert(mount_fs(disk_name) == 0);
  for (int i = 0; i < 100; i++) {
    assert(fs_create(cr8del_file) == 0);
    fd = fs_open(cr8del_file);
    assert(fd >= 0);
    assert(fs_write(fd, buf, BYTES_MB) == BYTES_MB);
    assert(fs_close(fd) == 0);
    assert(fs_delete(cr8del_file) == 0);
  }

  assert(umount_fs(disk_name) == 0);
  assert(remove(disk_name) == 0);
  free(buf);
}
