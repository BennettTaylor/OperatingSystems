#include "../fs.h"
#include <assert.h>
#include <stdlib.h>

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

  remove(disk_name); // remove disk if it exists
  assert(make_fs(disk_name) == 0);
  assert(mount_fs(disk_name) == 0);
  for (int i = 0; i < 64; i++) {
    assert(fs_create(file_names[i]) == 0);
  }

  char ***file_list_ptr = malloc(sizeof(char *));
  assert(fs_listfiles(file_list_ptr) == 0);
  char **file_list = *file_list_ptr;
  for (int i = 0; i < 64; i++) {
    assert(strcmp(file_list[i], file_names[i]) == 0);
    free(file_list[i]);
  }
  free(file_list);
  free(file_list_ptr);
  assert(umount_fs(disk_name) == 0);
  assert(remove(disk_name) == 0);
}
