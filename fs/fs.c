#include "disk.h"
#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_FILES 64
#define MAX_FILE_DESCRIPTORS 32
#define MAX_FILE_NAME 15
#define MAX_FILE_SIZE 1024 * 1024

/* Super block information */
struct super_block {
	char usage_bitmap[DISK_BLOCKS / 8];
	int directory_offset;
	int directory_size;
	int inode_table_offset;
	int inode_table_size;
	int data_offset;
	int data_size;
	bool is_mounted;
};

/* Inode information */
struct inode {
	int ref_count;
	int file_size;
	int blocks[MAX_FILE_SIZE / BLOCK_SIZE];
};

/* Directory file information */
struct directory_file {
	char name[MAX_FILE_NAME];
	int inode_index;
};

/* File descriptor information */
struct file_descriptor {
	int inode_index;
	int file_pointer;
};

/* Global variables */
static struct file_descriptor file_descriptors[MAX_FILE_DESCRIPTORS];
struct super_block disk_super_block;
struct inode inode_table[MAX_FILES];
struct directory_file directory[MAX_FILES];

/* Make the file system */
int make_fs(const char *disk_name) {
	/* Make the disk */
	if (make_disk(disk_name) != 0) {
		fprintf(stderr, "make_fs: cannot make disk\n");
		return -1;
	}

	/* Open the disk */
	if (open_disk(disk_name) != 0) {
		fprintf(stderr, "make_fs: cannot open disk\n");
		return -1;
	}

	/* Set up super block */
	disk_super_block.is_mounted = false;
	/* Super block is stored at disk block 0, directory at disk block 1 */
	disk_super_block.directory_offset = 1;
	/* Get size of directory */
	disk_super_block.directory_size = (sizeof(struct directory_file) * MAX_FILES) / BLOCK_SIZE;
	/* Inode table starts after directory */
	disk_super_block.inode_table_offset = disk_super_block.directory_offset + disk_super_block.directory_size;
	/* Inode size is same as number of files */
	disk_super_block.inode_table_size = MAX_FILES;
	/* Each inode needs its own block due to size */
	disk_super_block.data_offset = disk_super_block.inode_table_offset + disk_super_block.inode_table_size;
	/* Data size is disk size minus blocks need for metadata */
	disk_super_block.data_size = DISK_BLOCKS - disk_super_block.data_offset;
	/* Set usage bitmask to zero */
	for (int i = 0; i < sizeof(disk_super_block.usage_bitmap); i++) {
		disk_super_block.usage_bitmap[i] = 0;
	}

	/* Set bits that are used for metadata to 1 */
	for (int i = 0; i < disk_super_block.data_offset; i++) {
		disk_super_block.usage_bitmap[i / 8] |= (1 << (i % 8));
	}

	/* Write super block to first block on disk */
	char *block = calloc(1, BLOCK_SIZE);
	memcpy((void *) block, (void *) &disk_super_block, sizeof(struct super_block)); 
	block_write(0, block);

	/* Set up file directory */

	/* Set each directory file entry index to -1 to indicate unused file */
	for (int i  = 0; i < MAX_FILES; i++) {
		directory[i].inode_index = -1;
	}
	
	/* Write file directory to disk */
	memcpy((void *) block, (void *) &directory, sizeof(struct directory_file) * MAX_FILES);
	block_write(disk_super_block.directory_offset, block);

	/* Set up inode table */

	/* Set inodes in inode table to unused indication values */
	for (int i = 0; i < MAX_FILES; i++) {
		inode_table[i].ref_count = 0;
		inode_table[i].file_size = 0;
		for (int j = 0; j < (MAX_FILE_SIZE / BLOCK_SIZE); j++) {
			inode_table[i].blocks[j] = -1;
		}
	}

	/* Write inodes to disk */
	for (int i = 0; i < MAX_FILES; i++) {
		memcpy((void *) block, (void *) &inode_table[i], sizeof(struct inode));
		block_write(disk_super_block.inode_table_offset + i, block);
	}

	/* Close the disk */
	if (close_disk(disk_name) != 0) {
		fprintf(stderr, "make_fs: cannot close disk\n");
		return -1;
	}

	return 0;
}

/* Mount the file system onto the disk */
int mount_fs(const char *disk_name) {
	/* Open disk */
	if (open_disk(disk_name) != 0) {
		fprintf(stderr, "mount_fs: cannot open disk\n");
		return -1;
	}
	
	if (disk_super_block.is_mounted == true) {
		fprintf(stderr, "mount_fs: disk already mounted\n");
		return -1;
	}

	/* Load super block into global variable */
	char *block = calloc(1, BLOCK_SIZE);
	block_read(0, block);
	memcpy((void *) &disk_super_block, (void *) block, sizeof(struct super_block));

	/* Load directory into global variable */
	block_read(disk_super_block.directory_offset, block);
	memcpy((void *) &directory, (void *) block, sizeof(struct directory_file) * MAX_FILES);

	/* Load inodes into global variable */
	for (int i = 0; i < MAX_FILES; i++) {
		block_read(disk_super_block.inode_table_offset + i, block);
		memcpy((void *) &inode_table[i], (void *) block, sizeof(struct inode));
	}

	/* Set up file descriptors */
	for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
		/* Set file descriptor variables as empty */
		file_descriptors[i].inode_index = -1;
		file_descriptors[i].file_pointer = -1;
	}

	/* Indicate disk is mounted */
	disk_super_block.is_mounted = true;

	/* Free allocated variables */
	free(block);

	return 0;
}

int umount_fs(const char *disk_name) {
		
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "unmount_fs: to file system to dismount\n");
		return -1;
	}

	/* Indicate disk is unmounted */
	disk_super_block.is_mounted = false;

	/* Write super block to first block on disk */
	char *block = calloc(1, BLOCK_SIZE);
	memcpy((void *) block, (void *) &disk_super_block, sizeof(struct super_block));
	block_write(0, block);

	/* Write file directory to disk */
	memcpy((void *) block, (void *) &directory, sizeof(struct directory_file) * MAX_FILES);
	block_write(disk_super_block.directory_offset, block);

	/* Write inodes to disk */
	for (int i = 0; i < MAX_FILES; i++) {
		memcpy((void *) block, (void *) &inode_table[i], sizeof(struct inode));
		block_write(disk_super_block.inode_table_offset + i, block);
	}

	/* Close the disk */
	if (close_disk(disk_name) != 0) {
		fprintf(stderr, "make_fs: cannot close disk\n");
		return -1;
	}

	return 0;
}

int fs_open(const char *name) {
	/* Confirm disk is mounted */
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_open: disk not mounted\n");
		return -1;
	}
	/* Find file name in directory */
	int inode_index = -1;
	for (int i = 0; i < MAX_FILES; i++) {
		if (strcmp(directory[i].name, name) == 0) {
			inode_index = directory[i].inode_index;
		}
	}

	/* Check to see that inode was found */
	if (inode_index == -1) {
		fprintf(stderr, "fs_open: file could not be found\n");
		return -1;
	}

	/* Find valid file descriptor */
	int file_descriptor_index = -1;
	for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
		if (file_descriptors[i].inode_index  == -1) {
			// Found free file descriptor
			file_descriptors[i].inode_index = inode_index;
			file_descriptors[i].file_pointer = 0;
			inode_table[inode_index].ref_count++;
			file_descriptor_index = i;
			break;
		}
	}

	/* Check to see file descriptor was found */
	if (file_descriptor_index == -1) {
		fprintf(stderr, "fs_open: no free file descriptors\n");
		return -1;
	}

	return file_descriptor_index;
}

/* Close the file system*/
int fs_close(int fildes) {
	/* Check fildes bounds and existance */
	if ((fildes < 0) || (fildes > (MAX_FILE_DESCRIPTORS - 1)) || file_descriptors[fildes].inode_index == -1) {
		fprintf(stderr, "fs_close: file not found\n");
		return -1;
	}
	
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_close: disk not mounted\n");
		return -1;
	}

	/* Decrement the file reference counter */
	inode_table[file_descriptors[fildes].inode_index].ref_count--;

	/* Set file descriptor as free */
	file_descriptors[fildes].inode_index = -1;
	file_descriptors[fildes].file_pointer = -1;

	return 0;

}

/* Create a new file */
int fs_create(const char *name) {
	/* Check file name length */
	if (strlen(name) > MAX_FILE_NAME) {
		fprintf(stderr, "fs_create: file name too long\n");
		return -1;
	}

	/* Check that disk is mounted */
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_create: disk not mounted\n");
		return -1;
	}

	/* Check if file name already exists */
	for (int i = 0; i < MAX_FILES; i++) {
		if (strcmp(directory[i].name, name) == 0) {
			fprintf(stderr, "fs_create: file name already exists\n");
			return -1;
		}
	}

	/* Check that file system is created */
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_create: disk not mounted\n");
		return -1;
	}

	/* Find open file position */
	int directory_index = -1;
	for (int i = 0; i < MAX_FILES; i++) {
		if (directory[i].inode_index == -1) {
			directory_index = i;
			break;
		}
	}

	/* Check that open directory file exists */
	if (directory_index == -1) {
		fprintf(stderr, "fs_create: too many files in directory\n");
		return -1;
	}

	/* Find open inode table entry */
	int inode_index = -1;
	for (int i = 0; i < MAX_FILES; i ++) {
		if (inode_table[i].ref_count == 0) {
			inode_index = i;
		}
	}

	/* Check that open inode exists */
	if (inode_index == -1) {
		fprintf(stderr, "fs_create: no free inodes\n");
		return -1;
	}

	/* Create file */
	strcpy(directory[directory_index].name, name);
	directory[directory_index].inode_index = inode_index;
	inode_table[inode_index].ref_count++;
	
	return 0;
}

/* Delete a file */
int fs_delete(const char *name) {
	/* Check that disk is mounted */
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_delete: disk not mounted\n");
		return -1;
	}

	/* Find directory and inode index */
	int inode_index = -1;
	int directory_index = -1;
	for (int i = 0; i < MAX_FILES; i++) {
		if (strcmp(directory[i].name, name) == 0) {
			inode_index = directory[i].inode_index;
			directory_index = i;
		}
	}

	/* Check that directory entry exists */
	if (directory_index == -1) {
		fprintf(stderr, "fs_delete: file not found\n");
		return -1;
	}

	/* Check that inode exists */
	if (inode_index == -1) {
		fprintf(stderr, "fs_delete: file not found\n");
		return -1;
	}

	/* Check that there are no file descriptors pointing to file */
	if (inode_table[inode_index].ref_count > 1) {
		fprintf(stderr, "fs_delete: file is open\n");
		return -1;
	}

	/* Free data blocks */
	char *block = calloc(1, BLOCK_SIZE);
	for (int i = 0; i < (MAX_FILE_SIZE / BLOCK_SIZE); i++) {
		if (inode_table[inode_index].blocks[i] != -1) {
			/* Clear block data */
			block_write(inode_table[inode_index].blocks[i], block);
			/* Set usage bitmap at block location to unused */
			int block_location = inode_table[inode_index].blocks[i];
			disk_super_block.usage_bitmap[block_location / 8] &= ~(1 << (block_location % 8));
			/* Set inode block as unused */
			inode_table[inode_index].blocks[i] = -1;
		}
	}

	/* Clear directory entry */
	directory[directory_index].inode_index = -1;
	strcpy(directory[directory_index].name, "");

	/* Clear inode entry */
	inode_table[inode_index].ref_count = 0;
	inode_table[inode_index].file_size = 0;

	return 0;
}

/* Read from a file */
int fs_read(int fildes, void *buf, size_t nbyte) {
	/* Check that disk is mounted */
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_read: disk not mounted\n");
		return -1;
	}

	/* Check fildes bounds and existance */
	if ((fildes < 0) || (fildes > (MAX_FILE_DESCRIPTORS - 1)) || file_descriptors[fildes].inode_index == -1) {
		fprintf(stderr, "fs_read: file not found\n");
		return -1;
	}

	/* Check reading boundaries */
	int inode_index = file_descriptors[fildes].inode_index;
	if (file_descriptors[fildes].file_pointer + nbyte > inode_table[inode_index].file_size) {
		nbyte = inode_table[inode_index].file_size - file_descriptors[fildes].file_pointer;
	}

	/* Get initial file offset for reading */
	int file_offset = file_descriptors[fildes].file_pointer % BLOCK_SIZE;
	int file_block = file_descriptors[fildes].file_pointer / BLOCK_SIZE;

	char *block = calloc(1, BLOCK_SIZE);
	block_read(inode_table[inode_index].blocks[file_block], block);

	/* Allocate buffer to hold data in */
	char * buffer = malloc(nbyte);
	/* Loop through file byte by byte, copying into buf */
	int j = 0;
	for (int i = 0; i < nbyte; i++) {
		buffer[i] = block[file_offset + j];
		j++;
		if (file_offset + j >= BLOCK_SIZE) {
			/* Set block indexes back to zero */
			file_offset = 0;
			j = 0;
			/* Get next block */
			file_block++;
			block_read(inode_table[inode_index].blocks[file_block], block);
		}
	}

	/* Copy buffer into buf */
	memcpy(buf, (void *) (buffer), nbyte);

	/* Adjust file pointer */
	file_descriptors[fildes].file_pointer += nbyte;

	/* Free allocated variables */
	free(block);
	free(buffer);

	return nbyte;
}

/* Write to a file */
int fs_write(int fildes, void *buf, size_t nbyte) {
	/* Check that disk is mounted */
	if (disk_super_block.is_mounted == false) {
		fprintf(stderr, "fs_write: disk not mounted\n");
		return -1;
	}

	/* Check file descriptors bounds and existence */
	if ((fildes < 0) || (fildes > (MAX_FILE_DESCRIPTORS - 1)) || file_descriptors[fildes].inode_index == -1) {
		fprintf(stderr, "fs_write: file not found\n");
		return -1;
	}

	int inode_index = file_descriptors[fildes].inode_index;

	/* Check for write overflow and correct */
	if (file_descriptors[fildes].file_pointer + nbyte > MAX_FILE_SIZE) {
		nbyte = MAX_FILE_SIZE - file_descriptors[fildes].file_pointer;
		if (nbyte <= 0) {
			fprintf(stderr, "fs_write: file size exceeded\n");
			return -1;
		}
	}

	/* Get initial file offset for writing */
	int file_offset = file_descriptors[fildes].file_pointer % BLOCK_SIZE;
	int file_block = file_descriptors[fildes].file_pointer / BLOCK_SIZE;

	/* Check to see if file block is on disk */
	if (inode_table[inode_index].blocks[file_block] == -1) {
		/* Find free block on disk */
		int free_block = -1;
		for (int i = disk_super_block.data_offset; i < DISK_BLOCKS; i++) {
			if (!(disk_super_block.usage_bitmap[i / 8] & (1 << (i % 8)))) {
				free_block = i;

				/* Indicate block is now used */
				disk_super_block.usage_bitmap[i / 8] |= (1 << (i % 8));
				break;
			}
		}
		inode_table[inode_index].blocks[file_block] = free_block;
	}

	/* Update file size */
	if (file_descriptors[fildes].file_pointer + nbyte > inode_table[inode_index].file_size) {
		inode_table[inode_index].file_size = file_descriptors[fildes].file_pointer + nbyte;
	}

	/* Check if free blocks are available */
	if (inode_table[inode_index].blocks[file_block] == -1) {
		fprintf(stderr, "fs_write: disk full\n");
		return -1;
	}

	/* Get block of data that first needs to be written and read data */
	char *block = calloc(1, BLOCK_SIZE);
	block_read(inode_table[inode_index].blocks[file_block], block);

	/* Allocate buffer to hold data in */
	char * buffer = malloc(nbyte);
	memcpy(buffer, buf, nbyte);

	/* Write new data to block */
	int j = 0;
	for (int i = file_offset; (i < BLOCK_SIZE) && (j < nbyte); i++) {
		block[i] = buffer[j];
		j++;
	}
	block_write(inode_table[inode_index].blocks[file_block], block);

	/* Loop though remaining required blocks for writing */
	for (int i = file_block + 1; i < (file_descriptors[fildes].file_pointer + nbyte) / BLOCK_SIZE + 1; i++) {
		/* Check to see if file block is on disk */
		if (inode_table[inode_index].blocks[i] == -1) {
			/* Find free block on disk */
			int free_block = -1;
			for (int k = disk_super_block.data_offset; k < DISK_BLOCKS; k++) {
				if (!(disk_super_block.usage_bitmap[k / 8] & (1 << (k % 8)))) {
					free_block = k;
					/* Indicate block is used */
					disk_super_block.usage_bitmap[k / 8] |= (1 << (k % 8));
					break;
				}
			}
			inode_table[inode_index].blocks[i] = free_block;
		}

		/* Check if free blocks are available */
		if (inode_table[inode_index].blocks[i] == -1) {
			fprintf(stderr, "fs_write: disk full\n");
			return -1;
		}

		/* Read next block */
		block_read(inode_table[inode_index].blocks[i], block);
		/* Copy from buffer into block */
		for (int k = 0; (k < BLOCK_SIZE) && (j < nbyte); k++) {
			block[k] = buffer[j];
			j++;
		}

		/* Write back block to disk */
		block_write(inode_table[inode_index].blocks[i], block);
	}

	/* Increment file pointer */
	file_descriptors[fildes].file_pointer += nbyte;

	return nbyte;
}

int fs_get_filesize(int fildes) {
	/* Check that file descriptor is set to file */
	if (file_descriptors[fildes].inode_index == -1) {
		fprintf(stderr, "fs_get_filesize: file not found\n");
		return -1;
	}

	/* Return size of file */
	return inode_table[file_descriptors[fildes].inode_index].file_size;
}

int fs_listfiles(char ***files) {
	/* Loop through directory and find file names */
        *files = (char **) malloc((MAX_FILES + 1) * sizeof(char *));
	int name_index = 0;
	for (int i = 0; i < MAX_FILES; i++) {
		if (directory[i].inode_index != -1) {
			(*files)[name_index] = (char *) malloc(sizeof(directory[i].name));
			strcpy((*files)[name_index], directory[i].name);
			name_index++;
		}
	}

	/* Set last name to NULL */
	(*files)[name_index] = NULL;
	return 0;
}

/* Seek to a specific offset in a file */
int fs_lseek(int fildes, off_t offset) {
	/* Check file descriptor bounds and existance */
	if ((fildes < 0) || (fildes > (MAX_FILE_DESCRIPTORS - 1)) || file_descriptors[fildes].inode_index == -1) {
		fprintf(stderr, "fs_lseek: file not found\n");
		return -1;
	}

	/* Check offset bounds */
	if ((offset < 0) || (offset > (inode_table[file_descriptors[fildes].inode_index].file_size - 1))) {
		fprintf(stderr, "fs_lseek: offset out of bounds\n");
		return -1;
	}

	/* Set file offset */
	file_descriptors[fildes].file_pointer = offset;
	return 0;
}

/* Truncate a file to a specific length */
int fs_truncate(int fildes, off_t length) {
	/* Check file descriptor bounds and existance */
	if ((fildes < 0) || (fildes > (MAX_FILE_DESCRIPTORS - 1)) || file_descriptors[fildes].inode_index == -1) {
		fprintf(stderr, "fs_truncate: file not found\n");
	}
	
	int inode_index = file_descriptors[fildes].inode_index;

	/* Check that truncation length is not greater than file length */
	if (length > inode_table[inode_index].file_size) {
		fprintf(stderr, "fs_truncate: trunaction length greater than file size\n");
		return -1;
	}

	/* Truncate last block */
	int last_block = length / BLOCK_SIZE;
	int last_block_offset = length % BLOCK_SIZE;

	/* Get last block */
	char *block = calloc(1, BLOCK_SIZE);
	block_read(inode_table[inode_index].blocks[last_block], block);

	/* Set rest of block to 0 */
	for (int i = last_block_offset; i < BLOCK_SIZE; i++) {
		block[i] = 0;
	}

	/* Write block back to disk */
	block_write(inode_table[inode_index].blocks[last_block], block);

	/* Clear block */
	free(block);
	block = calloc(1, BLOCK_SIZE);

	/* Loop through rest of file to free blocks */
	while (inode_table[inode_index].blocks[last_block] != -1) {
		/* Set block to free in usage bitmask */
		disk_super_block.usage_bitmap[last_block / sizeof(char)] &= ~(1 << (last_block % sizeof(char)));
		/* Set data to free */
		block_write(inode_table[inode_index].blocks[last_block], block);
		/* Set blocks as unused */
		inode_table[inode_index].blocks[last_block] = -1;
		last_block++;
	}

	/* Update file size */
	inode_table[inode_index].file_size = length;

	/* Free allocated variables */
	free(block);

	return 0;
}
