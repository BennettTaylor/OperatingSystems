#include "../fs.h"
#include <assert.h>

int main() {
  const char *disk_name = "test_fs";
  const char *file_name = "test_file";
  char write_buf[] = "hello world";
  char truncated[] = "hello";
  char read_buf[sizeof(write_buf)] = {0};

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);

  assert(fs_create(file_name) == 0);
  int fd = fs_open(file_name);
  assert(fd >= 0);
  assert(fs_write(fd, write_buf, sizeof(write_buf)) == sizeof(write_buf));
  int file_size = fs_get_filesize(fd);
  assert(file_size == sizeof(write_buf));
  assert(fs_truncate(fd, -1) == -1);            // invalid size
  assert(fs_truncate(fd, file_size + 1) == -1); // invalid size

  off_t new_size = strlen(truncated);
  assert(fs_truncate(fd, new_size) == 0);
  assert(fs_get_filesize(fd) == new_size);
  assert(fs_lseek(fd, 0) == 0);
  assert(fs_read(fd, read_buf, sizeof(read_buf)) == new_size);
  assert(strcmp(read_buf, truncated) == 0);
  assert(fs_close(fd) == 0);
  assert(fs_truncate(fd, 0) == -1); // file not open

  fd = fs_open(file_name);
  assert(umount_fs(disk_name) == 0);
  assert(fs_truncate(fd, 0) == -1); // disk not mounted
  assert(remove(disk_name) == 0);
}
