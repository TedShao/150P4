#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define SIG "ECS150FS"
#define FAT_EOC 0xffff

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

Root_dir_t root_dir = NULL;

typedef struct __attribute__((__packed__)) Fd {
    Root_dir_t open_file;
    size_t offset;
} *Fd_t;

Fd_t fds = NULL;

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

    /* check superblock */
    if (memcmp(superblock->signature, SIG, 8) != 0) {
        return -1;
    }
    if (block_disk_count() != superblock->total_blks) {
        return -1;
    }

    /* read & check fat blocks */
    size_t fat_arr_size = BLOCK_SIZE / 2 * superblock->total_fat_blks;
    fat_array = (uint16_t*)malloc(fat_arr_size * sizeof(uint16_t));
    if (fat_array == NULL) {
        return -1;
    }
    for (int i = 0; i < superblock->total_fat_blks; i++) {
        if (block_read(i+1, fat_array + (i * BLOCK_SIZE) / 2) == -1) {
            return -1;
        }
    }
    fat_array[0] = 0xFFFF;

    /* read & check root directory */
    root_dir = (Root_dir_t)malloc(FS_FILE_MAX_COUNT * sizeof(struct Root_dir));
    if (root_dir == NULL) {
        return -1;
    }
    if (block_read(superblock->root_dir_idx, root_dir) == -1) {
        return -1;
    }

    /* Phase 3: set default fd opened files */
    fds = (Fd_t)malloc(FS_OPEN_MAX_COUNT * sizeof(struct Fd));
    if (fds == NULL) {
        return -1;
    }
    for (int i = 0; i < 32; i++) {
        fds[i].open_file = NULL;
    }

    return 0;
}

int fs_umount(void)
{
    /* if there is no virtual disk opened */
    if (block_disk_count() == -1) {
        return -1;
    }

    /* if there exists any opened file */
    for (int i = 0; i < 32; i++) {
        if (fds[i].open_file != NULL) {
            return -1;
        }
    }
    /* write backs */
    if (block_write(0, superblock) == -1) {
        return -1;
    }
    for (int i = 0; i < superblock->total_fat_blks; i++) {
        if (block_write(i+1, fat_array + (i * BLOCK_SIZE) / 2) == -1) {
            return -1;
        }
    }

    if (block_write(superblock->root_dir_idx, root_dir) == -1) {
        return -1;
    }

    /* close file and error check */
    if (block_disk_close() == -1) {
        return -1;
    }

    /* free allocated memory */
    if (superblock != NULL) {
        free(superblock);
    }
    if (root_dir != NULL) {
        free(root_dir);
    }
    if (fat_array != NULL) {
        free(fat_array);
    }
    if (fds != NULL) {
        free(fds);
    }
    return 0;
}

int fs_info(void)
{
    /* if there is no virtual disk opened */
    if (block_disk_count() == -1) {
        return -1;
    }

    /* count the number of free data blocks */
    int free_blks = 0;
    for (int i = 0; i < superblock->total_data_blks; i++) {
        if (fat_array[i] == 0) {
            free_blks++;
        }
    }

    /* count the number of free root directories */
    int free_rdir_count = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; ++i) {
        if (root_dir[i].filename[0] == '\0') {
            free_rdir_count++;
        }
    }

    /* print all info */
    printf("FS Info:\n");
    printf("total_blk_count=%u\n", superblock->total_blks);
    printf("fat_blk_count=%u\n", superblock->total_fat_blks);
    printf("rdir_blk=%u\n", superblock->root_dir_idx);
    printf("data_blk=%u\n", superblock->data_blk_idx);
    printf("data_blk_count=%u\n", superblock->total_data_blks);
    printf("fat_free_ratio=%u/%u\n", free_blks, superblock->total_data_blks);
    printf("rdir_free_ratio=%u/%u\n", free_rdir_count, 128);

    return 0;
}

int fs_create(const char *filename)
{
    /* check valid filename */
    if (filename == NULL) {
        return -1;
    }
    if (strlen(filename) == 0 || strlen(filename) > FS_FILENAME_LEN) {
        return -1;
    }

    int availableIndex = -1;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        /* if filename exists */
        if (strcmp(filename, (char*)root_dir[i].filename) == 0) {
            return -1;
        }
        /* if there is an available space, assign to available index */
        if (root_dir[i].filename[0] == '\0' && availableIndex == -1) {
            availableIndex = i;
        }
    }
    /* if there is no available space */
    if (availableIndex == -1) {
        return -1;
    }

    /* set the root directory */
    memset(&(root_dir[availableIndex]), 0, BLOCK_SIZE/FS_FILE_MAX_COUNT);
    strcpy((char*)root_dir[availableIndex].filename, filename);
    root_dir[availableIndex].filesize = 0;
    root_dir[availableIndex].first_blk_index = FAT_EOC;
    return 0;
}

int fs_delete(const char *filename)
{
    if (filename == NULL) {
        return -1;
    }
    if (strlen(filename) == 0 || strlen(filename) > FS_FILENAME_LEN) {
        return -1;
    }

    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        if (fds->open_file != NULL
          && strcmp(filename, (char*)fds->open_file->filename) == 0) {
            return -1;
        }
    }

    int idx = -1;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (strcmp(filename, (char*)root_dir[i].filename) == 0 && idx == -1) {
            idx = i;
        }
    }
    if (idx == -1) {
        return -1;
    }

    /* free file's conetent in FAT */
    int delete_blk_idx = root_dir[idx].first_blk_index;
    while (delete_blk_idx != FAT_EOC) {
        uint16_t temp = fat_array[delete_blk_idx];
        fat_array[delete_blk_idx] = 0;
        delete_blk_idx = temp;
    }

    /* reset related content in root directory */
    memset(&(root_dir[idx]), 0, BLOCK_SIZE/FS_FILE_MAX_COUNT);
    strcpy((char*)root_dir[idx].filename, "\0");
    root_dir[idx].filesize = 0;
    root_dir[idx].first_blk_index = 0;
    return 0;
}

int fs_ls(void)
{
    if (block_disk_count() == -1) {
        return -1;
    }

    printf("FS Ls:\n");
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (root_dir[i].filename[0] != '\0') {
            printf("file: %s, ", (char*)root_dir[i].filename);
            printf("size: %u, ", root_dir[i].filesize);
            printf("data_blk: %u\n", root_dir[i].first_blk_index);
        }
    }

    return 0;
}

int fs_open(const char *filename)
{
    /* check if filename is valid */
    if (filename == NULL) {
        return -1;
    }
    if (strlen(filename) == 0 || strlen(filename) > FS_FILENAME_LEN) {
        return -1;
    }

    /* find file location */
    int f_loc = -1;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (strcmp(filename, (char*)root_dir[i].filename) == 0 && f_loc == -1) {
            f_loc = i;
        }
    }
    // file named filename not found
    if (f_loc == -1) {
        return -1;
    }

    /* find free file descriptor location */
    int fd_idx = -1;
    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        if (fd_idx == -1 && fds[i].open_file == NULL) {
            fd_idx = i;
        }
    }
    // no fd opening
    if (fd_idx == -1) {
        return -1;
    }

    fds[fd_idx].open_file = &(root_dir[f_loc]);
    fds[fd_idx].offset = 0;

    return fd_idx;
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (fds[fd].open_file == NULL) {
        return -1;
    }

    fds[fd].open_file = NULL;
    fds[fd].offset = 0;

    return 0;
}

int fs_stat(int fd)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (fds[fd].open_file == NULL) {
        return -1;
    }

    return fds[fd].open_file->filesize;
}

int fs_lseek(int fd, size_t offset)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (fds[fd].open_file == NULL) {
        return -1;
    }

    if (offset > fds[fd].open_file->filesize) {
        return -1;
    }

    fds[fd].offset = offset;

    return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
    /* handle error */
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }
    if (fds[fd].open_file == NULL) {
        return -1;
    }

    /* allocate space if needed */
    if (fds[fd].offset + count > fds[fd].open_file->filesize) {
        uint16_t cur;
        if (fds[fd].open_file->first_blk_index == FAT_EOC) {
            // nxt = first_free_block();
            for (int j = 1; j < superblock->total_data_blks; j++) {
                if (fat_array[j] == 0) {
                    fds[fd].open_file->first_blk_index = j;
                    break;
                }
            }
        }
        if (fds[fd].open_file->first_blk_index != FAT_EOC) {
            fat_array[fds[fd].open_file->first_blk_index] = FAT_EOC;
            cur = fds[fd].open_file->first_blk_index;
        }

        int bounce_blk_count = (fds[fd].offset + count - 1) / BLOCK_SIZE + 1;
        for (int i = 1; i < bounce_blk_count; i++) {
            if (fat_array[cur] == FAT_EOC) {
                // nxt = first_free_block();
                for (int j = 1; j < superblock->total_data_blks; j++) {
                    if (fat_array[j] == 0) {
                        fat_array[cur] = j;
                        break;
                    }
                }
            }
            if (fat_array[cur] == FAT_EOC) {
                break;
            }
            // fat_array[cur] = nxt;
            fat_array[fat_array[cur]] = FAT_EOC;
            cur = fat_array[cur];
            fat_array[cur] = fat_array[fat_array[cur]];
        }
    }

    /* read to bounce buffer */
    int start_blk_idx = fds[fd].offset / BLOCK_SIZE;
    int end_blk_idx = (fds[fd].offset + count - 1) / BLOCK_SIZE;
    size_t total_buffer_blks = end_blk_idx - start_blk_idx + 1;

    int total_fat_blks = 0;
    uint16_t idx = fds[fd].open_file->first_blk_index;
    while (idx != FAT_EOC) {
        idx = fat_array[idx];
        total_fat_blks++;
    }
    if (total_buffer_blks > total_fat_blks) {
        total_buffer_blks = total_fat_blks;
        end_blk_idx = total_buffer_blks + start_blk_idx - 1;
    }
    if (total_buffer_blks == 0) {
        return 0;
    }
    uint8_t* buffer = NULL;
    buffer = (uint8_t*)malloc(total_buffer_blks * BLOCK_SIZE * sizeof(uint8_t));
    if (buffer == NULL) {
        return -1;
    }
    idx =  fds[fd].open_file->first_blk_index;
    for (int i = 0; i < end_blk_idx; i++) {
        if (i >= start_blk_idx) {
            block_read(superblock->data_blk_idx + idx, buffer + i * BLOCK_SIZE);
        }
        idx = fat_array[idx];
    }

    /* write to bounce buffer */
    if (count > total_buffer_blks * BLOCK_SIZE - fds[fd].offset) {
        count = total_buffer_blks * BLOCK_SIZE - fds[fd].offset;
    }
    memcpy(buffer + (fds[fd].offset % BLOCK_SIZE), buf, count);

    /* write from bounce buffer to file system */
    idx =  fds[fd].open_file->first_blk_index;
    for (int i = 0; i < end_blk_idx; i++) {
        if (i >= start_blk_idx) {
           block_write(superblock->data_blk_idx + idx, buffer + i * BLOCK_SIZE);
        }
        idx = fat_array[idx];
    }

    if (fds[fd].open_file->filesize < fds[fd].offset + count) {
        fds[fd].open_file->filesize = fds[fd].offset + count;
    }
    free(buffer);
    return count;
}

int fs_read(int fd, void *buf, size_t count)
{
    /* handle error */
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }
    if (fds[fd].open_file == NULL) {
        return -1;
    }

    /* read to bounce buffer */
    int start_blk_idx = fds[fd].offset / BLOCK_SIZE;
    int end_blk_idx = (fds[fd].offset + count - 1) / BLOCK_SIZE;
    size_t total_buffer_blks = end_blk_idx - start_blk_idx + 1;

    int total_fat_blks = 0;
    uint16_t idx = fds[fd].open_file->first_blk_index;
    while (idx != FAT_EOC) {
        idx = fat_array[idx];
        total_fat_blks++;
    }
    if (total_buffer_blks > total_fat_blks) {
        total_buffer_blks = total_fat_blks;
        end_blk_idx = total_buffer_blks + start_blk_idx - 1;
    }
    if (total_buffer_blks == 0) {
        return 0;
    }
    uint8_t* buffer = NULL;
    buffer = (uint8_t*)malloc(total_buffer_blks * BLOCK_SIZE * sizeof(uint8_t));
    if (buffer == NULL) {
        return -1;
    }
    idx =  fds[fd].open_file->first_blk_index;
    for (int i = 0; i < end_blk_idx; i++) {
        if (i >= start_blk_idx) {
            block_read(superblock->data_blk_idx + idx, buffer + i * BLOCK_SIZE);
        }
        idx = fat_array[idx];
    }

    /* read from bounce buffer to buf */
    if (count > fds[fd].open_file->filesize - fds[fd].offset) {
        count = fds[fd].open_file->filesize - fds[fd].offset;
    }
    memcpy(buf, buffer + (fds[fd].offset % BLOCK_SIZE), count);
    fds[fd].offset += count;
    free(buffer);
    return count;
}
