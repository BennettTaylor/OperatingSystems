#include "../fs.h"
#include <assert.h>

#define MAX_FD 32

int main() {
  const char *disk_name = "test_fs";
  const char *file_name = "test_file";
  int fds[MAX_FD];
  const char *file_names[MAX_FD] = {
      "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10", "11",
      "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22",
      "23", "24", "25", "26", "27", "28", "29", "30", "31", "32",
  };

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);
  assert(fs_open(file_name) == -1); // file does not exist
  assert(fs_create(file_name) == 0);

  // open the same file multiple times
  for (int i = 0; i < MAX_FD; i++) {
    fds[i] = fs_open(file_name);
    assert(fds[i] >= 0);
  }
  assert(fs_open(file_name) == -1); // fd limit reached
  for (int i = 0; i < MAX_FD; i++) {
    assert(fs_close(fds[i]) == 0);
  }
  assert(fs_close(fds[0]) == -1); // fd not in use

  // open multiple files
  for (int i = 0; i < MAX_FD; i++) {
    assert(fs_create(file_names[i]) == 0);
    fds[i] = fs_open(file_names[i]);
  }
  assert(fs_open(file_name) == -1); // fd limit reached
  for (int i = 0; i < MAX_FD; i++) {
    assert(fs_close(fds[i]) == 0);
  }

  assert(umount_fs(disk_name) == 0);
  assert(fs_open(file_name) == -1); // disk not mounted
  assert(remove(disk_name) == 0);
}
