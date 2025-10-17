#include "../fs.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#define BYTES_KB 1024
#define BYTES_MB (1024 * BYTES_KB)
#define NUM_FILES 16

int main() {
  const char *disk_name = "test_fs";
  char write_buf0[BYTES_MB];
  char write_buf1[BYTES_MB];
  char read_buf[BYTES_MB];
  int fds[NUM_FILES];
  const char *file_names[NUM_FILES] = {
      "1", "2",  "3",  "4",  "5",  "6",  "7",  "8",
      "9", "10", "11", "12", "13", "14", "15", "16",
  };
  int file_index = 0;

  srand(time(NULL));
  for (int i = 0; i < BYTES_MB; i++) {
    write_buf0[i] = 'A' + rand() % 26;
    write_buf1[i] = 'A' + rand() % 26;
  }

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);

  // 9.3) Create 1 MiB file, write 1 MiB data --> check size
  assert(fs_create(file_names[file_index]) == 0);
  fds[file_index] = fs_open(file_names[file_index]);
  assert(fds[file_index] >= 0);
  assert(fs_write(fds[file_index], write_buf0, BYTES_MB) == BYTES_MB);
  assert(fs_get_filesize(fds[file_index]) == BYTES_MB);
  assert(fs_close(fds[file_index]) == 0);
  assert(fs_delete(file_names[file_index]) == 0);

  // 9.4) Write {1 KiB, 4 KiB, 1 MiB} data to file
  assert(fs_create(file_names[file_index]) == 0);
  fds[file_index] = fs_open(file_names[file_index]);
  assert(fds[file_index] >= 0);
  assert(fs_write(fds[file_index], write_buf0, BYTES_KB) == BYTES_KB);
  assert(fs_lseek(fds[file_index], 0) == 0);
  assert(fs_write(fds[file_index], write_buf0, 4 * BYTES_KB) == 4 * BYTES_KB);
  assert(fs_lseek(fds[file_index], 0) == 0);
  assert(fs_write(fds[file_index], write_buf0, BYTES_MB) == BYTES_MB);
  assert(fs_close(fds[file_index]) == 0);
  assert(fs_delete(file_names[file_index]) == 0);

  // 9.6) Write 1 MiB data to file / write bytes 500-600 / read whole file
  assert(fs_create(file_names[file_index]) == 0);
  fds[file_index] = fs_open(file_names[file_index]);
  assert(fds[file_index] >= 0);
  assert(fs_write(fds[file_index], write_buf0, BYTES_MB) == BYTES_MB);
  assert(fs_lseek(fds[file_index], 500) == 0);
  assert(fs_write(fds[file_index], write_buf1, 100) == 100);
  assert(fs_lseek(fds[file_index], 0) == 0);
  assert(fs_read(fds[file_index], read_buf, BYTES_MB) == BYTES_MB);
  assert(memcmp(read_buf, write_buf0, 500) == 0);       // first 500 bytes
  assert(memcmp(read_buf + 500, write_buf1, 100) == 0); // bytes 500-600
  assert(memcmp(read_buf + 600, write_buf0 + 600, BYTES_MB - 600) == 0); // rest
  assert(fs_close(fds[file_index]) == 0);
  assert(fs_delete(file_names[file_index]) == 0);

  // 9.7) Write 16 files, 1 MiB each. Verify their contents
  for (int i = file_index; i < file_index + NUM_FILES; i++) {
    assert(fs_create(file_names[i]) == 0);
    fds[i] = fs_open(file_names[i]);
    assert(fds[i] >= 0);
    if (i % 2 == 0) {
      assert(fs_write(fds[i], write_buf0, BYTES_MB) == BYTES_MB);
    } else {
      assert(fs_write(fds[i], write_buf1, BYTES_MB) == BYTES_MB);
    }
  }
  for (; file_index < NUM_FILES; file_index++) {
    assert(fs_lseek(fds[file_index], 0) == 0);
    memset(read_buf, 0, BYTES_MB);
    assert(fs_read(fds[file_index], read_buf, BYTES_MB) == BYTES_MB);
    if (file_index % 2 == 0) {
      assert(memcmp(read_buf, write_buf0, BYTES_MB) == 0);
    } else {
      assert(memcmp(read_buf, write_buf1, BYTES_MB) == 0);
    }
    assert(fs_close(fds[file_index]) == 0);
  }

  // Clean up
  assert(umount_fs(disk_name) == 0);
  assert(remove(disk_name) == 0);
}
