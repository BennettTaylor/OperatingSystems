#ifndef _DISK_H_
#define _DISK_H_

#define DISK_BLOCKS  8192
#define BLOCK_SIZE   4096

int make_disk(const char *name);
int open_disk(const char *name);
int close_disk();

int block_write(int block, const void *buf);
int block_read(int block, void *buf);

#endif
