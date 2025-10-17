#include "../fs.h"
#include <assert.h>

int main() {
  const char *disk_name = "test_fs";
  remove(disk_name);

  assert(mount_fs(disk_name) == -1);  // disk doesn't exist
  assert(make_fs(disk_name) == 0);    // create disk
  assert(umount_fs(disk_name) == -1); // disk is not mounted
  assert(mount_fs(disk_name) == 0);
  assert(umount_fs(disk_name) == 0);

  FILE *fp;
  fp = fopen(disk_name, "r");
  assert(fp != NULL);

  // check that some metadata is written to the disk
  bool non_zero_byte = false;
  char byte;
  while ((byte = fgetc(fp)) != EOF) {
    if (byte != 0) {
      non_zero_byte = true;
      break;
    }
  }
  assert(non_zero_byte == true);
  assert(fclose(fp) == 0);
  assert(remove(disk_name) == 0);
}
