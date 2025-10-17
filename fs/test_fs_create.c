#include "../fs.h"
#include <assert.h>

int main() {
  const char *disk_name = "test_fs";
  const char *file_names[64] = {
      "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10", "11",
      "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22",
      "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33",
      "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44",
      "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55",
      "56", "57", "58", "59", "60", "61", "62", "63", "64",
  };

  remove(disk_name);
  assert(make_fs(disk_name) == 0);              // create disk
  assert(fs_create(file_names[0]) == -1);       // disk not mounted
  assert(mount_fs(disk_name) == 0);             // mount the disk
  assert(fs_create("") == -1);                  // invalid file name
  assert(fs_create("0123456789abcdefg") == -1); // invalid file name

  for (int i = 0; i < 64; i++) {
    assert(fs_create(file_names[i]) == 0);  // create the file
    assert(fs_create(file_names[i]) == -1); // file already exists
  }

  assert(fs_create("65") == -1);     // no more space
  assert(umount_fs(disk_name) == 0); // unmount the disk
  assert(fs_create("65") == -1);     // disk not mounted
  assert(remove(disk_name) == 0);    // remove the disk

  // 6.2) Create different amounts of files
}
