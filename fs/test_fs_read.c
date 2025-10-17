#include "../fs.h"
#include <assert.h>

int main() {
  const char *disk_name = "test_fs";
  const char *file_name = "test_file";
  char write_buf[] = "hello world";
  char read_buf[sizeof(write_buf)];

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);
  assert(fs_create(file_name) == 0);

  int fd = fs_open(file_name);
  assert(fd >= 0);
  assert(fs_read(fd, read_buf, sizeof(read_buf)) == 0); // empty file
  assert(fs_write(fd, write_buf, sizeof(write_buf)) == sizeof(write_buf));

  assert(fs_lseek(fd, -1) == -1); // invalid offset
  int file_size = fs_get_filesize(fd);
  assert(file_size == sizeof(write_buf));
  assert(fs_lseek(fd, file_size + 1) == -1); // invalid offset

  assert(fs_lseek(fd, 0) == 0);
  assert(fs_read(fd, read_buf, sizeof(read_buf)) == sizeof(read_buf));
  assert(strcmp(write_buf, read_buf) == 0);
  assert(fs_close(fd) == 0);
  assert(fs_read(fd, read_buf, sizeof(read_buf)) == -1); // file not opened

  fd = fs_open(file_name);
  assert(fd >= 0);
  assert(umount_fs(disk_name) == 0);
  assert(fs_read(fd, read_buf, sizeof(read_buf)) == -1); // disk not mounted
  assert(remove(disk_name) == 0);
}
