#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define SIG "ECS150FS"


typedef struct __attribute__((__packed__)) Superblock {
    uint8_t  signature[8];
    uint16_t total_blks;            // total number of blocks
    uint16_t root_dir_idx;          // root directory index
    uint16_t data_blk_idx;          // data block index
    uint16_t total_data_blks;       // total number of data blocks
    uint8_t  total_fat_blks;        // total number of fat blocks
    uint8_t  padding[4079];
} *Superblock_t;

Superblock_t superblock = NULL;

uint16_t *fat_array = NULL;

typedef struct __attribute__((__packed__)) Root_dir {
    uint8_t  filename[FS_FILENAME_LEN];
    uint32_t filesize;
    uint16_t first_blk_index;
    uint8_t  padding[10];
} *Root_dir_t;

Root_dir_t root_dir[FS_FILE_MAX_COUNT];

// typedef struct __attribute__((__packed__)) Fd {
//     Root_dir_t *root_dir;
//     size_t offset;
// } *Fd_t;

// Fd_t fd[FS_FILE_MAX_COUNT];


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
    /* open disk & error check */
    if (block_disk_open(diskname) == -1) {
        return -1;
    }

    /* read & error check superblock */
    superblock = (Superblock_t)malloc(sizeof(struct Superblock));
    if (superblock == NULL) {
        return -1;
    }
    if (block_read(0, superblock) == -1) {
        return -1;
    }

    /* check sig */
    char sig_checker[8];
    memcpy(sig_checker, superblock->signature, 8);
    if (strcmp(SIG, sig_checker) != 0) {
        return -1;
    }

    /* check total_blks */
    if (block_disk_count() != superblock->total_blks) {
        return -1;
    }

    /* read & check fat blocks */
    size_t fat_arr_size = 4096 * superblock->total_fat_blks;
    fat_array = (uint16_t*)malloc(fat_arr_size * sizeof(uint16_t));
    if (fat_array == NULL) {
        return -1;
    }
    for (int i = 0; i < superblock->total_fat_blks; i++) {
        if (block_read(i+1, fat_array + (i * 4096)) == -1) {
            return -1;
        }
    }

    /* read & check root directory */
    if (block_read(superblock->root_dir_idx, root_dir) == -1) {
        return -1;
    }

    return 0;
}

/**
 * fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no underlying virtual disk was opened, or if the virtual disk
 * cannot be closed, or if there are still open file descriptors. 0 otherwise.
 */
int fs_umount(void)
{
    /* write & check superblock */
    if (block_write(0, superblock) == -1) {
        return -1;
    }

    /* write & check fat blocks */
    for (int i = 0; i < superblock->total_fat_blks; i++) {
        if (block_write(i+1, fat_array + (i * 2048)) == -1) {
            return -1;
        }
    }

    /* write & check root directory */
    if (block_write(superblock->root_dir_idx, root_dir) == -1) {
        return -1;
    }


    /* close file and error check */
    if (block_disk_close() == -1) {
        return -1;
    }

    /* free allocated memory */
    free(superblock);
    free(fat_array);

    return 0;
}

int fs_info(void)
{
    /* count the number of free data blocks */
    int free_blocks_count = 0;
    for (int i = 0; i < superblock->total_data_blks; i++) {
        if (fat_array[i] == 0) {
            free_blocks_count++;
        }
    }

    /* count the number of free root directories*/
    int free_rdir_count = 0;
    for (int i = 0; i < 128; i++) {
        if (root_dir[i]->filesize == 0) {
            free_rdir_count++;
        }
    }

    printf("FS Info:\n");
    printf("total blocks=%u\n", superblock->total_blks);
    printf("total_fat_blks=%u\n", superblock->total_fat_blks);
    printf("root_dir_idx=%u\n", superblock->root_dir_idx);
    printf("data_blk_idx=%u\n", superblock->data_blk_idx);
    printf("total_data_blks=%u\n", superblock->total_data_blks);
    printf("fat_free_ratio=%u/%u\n", free_blocks_count, superblock->total_data_blks);
    printf("rdir_free_ratio=%u/%u\n", free_rdir_count, 128);

    return 0;
}

int fs_create(const char *filename)
{
    /* TODO: Phase 2 */
    return 0;
}

int fs_delete(const char *filename)
{
    /* TODO: Phase 2 */
    return 0;
}

int fs_ls(void)
{
    /* TODO: Phase 2 */
    return 0;
}

int fs_open(const char *filename)
{
    /* TODO: Phase 3 */
    return 0;
}

int fs_close(int fd)
{
    /* TODO: Phase 3 */
    return 0;
}

int fs_stat(int fd)
{
    /* TODO: Phase 3 */
    return 0;
}

int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    return 0;
}
