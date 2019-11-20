#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
struct superblock {
	int8_t signature;
	int16_t block_amount;
	int16_t root_dir_index;
	int16_t data_block_index;
	int16_t data_block_amount;
	int8_t fat_block;
	int8_t padding[4079];
};

struct fat {
	int16_t entry;
};

struct root_dir {
	int8_t filename[16];
	int32_t filesize;
	int16_t first_index;
	int8_t padding[10];
};

struct fd {
	root_dir_t *root_dir;
	size_t offset;
};

superblock_t superBlk;
fd_t fd_tmp;
/**
 * fs_mount - Mount a file system
 * @diskname: Name of the virtual disk file
 *
 * Open the virtual disk file @diskname and mount the file system that it
 * contains. A file system needs to be mounted before files can be read from it
 * with fs_read() or written to it with fs_write().
 *
 * Return: -1 if virtual disk file @diskname cannot be opened, or if no valid
 * file system can be located. 0 otherwise.
 */
int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname) == -1) {
		return -1;
	}
	if (block_read(0, superBlk) == -1) {
		return -1;
	}
	size_t i;
	size_t fat_size;
	fat_size = 4096 * superBlk.fat_block;
	fat.entry = malloc(sizeof(fat_size));






	for (i = 0; i < 32; i++) {
		fd_tmp->root_dir = NULL;
		fd_tmp->offset = 0;
		if(block_read(i, fat.entries) == -1) {
			return -1;
		}
	}
	return 0;

}

int fs_umount(void)
{
	if (block_write(0, superBlk) == -1) {
		return -1;
	}
}

int fs_info(void)
{
    size_t i;
    int free_blocks_count = 0;

    for (i = 1; i < superBlk.data_block_amount; i++) {
            if (fat.entry[i] == 0)
                    free_blocks_count++;
    }
    printf("FS Info:\n");
    printf("total data_block_amount=%u\n", superBlk.data_block_amount);
    printf("fat_block=%u\n", superBlk.fat_block);
    printf("root_dir_index=%u\n", superBlk.root_dir_index);
    printf("data_block_index=%u\n", superBlk.data_block_index);
    printf("data_block_amount=%u\n", superBlk.data_block_amount);
    printf("fat_free_ratio=%u/%u\n", free_blocks_count, superBlk.data_block_amount);
    printf("rdir_free_ratio=%u/%u\n", free_rdir_count, 128);

}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

