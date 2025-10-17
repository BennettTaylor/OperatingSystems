#include "../fs.h"
#include <assert.h>

int main() {
  const char *disk_name = "test_fs";
  const char *file_name = "test_file";
  char buf[] = "hello world";

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);

  assert(fs_create(file_name) == 0);
  int fd = fs_open(file_name);
  assert(fd >= 0);

  assert(fs_write(fd, buf, sizeof(buf)) == sizeof(buf));
  assert(fs_get_filesize(fd) == sizeof(buf));
  assert(fs_close(fd) == 0);

  assert(umount_fs(disk_name) == 0);
  assert(remove(disk_name) == 0);
}
