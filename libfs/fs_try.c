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

Fd_t File_descriptors = NULL;

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
    if (superblock->root_dir_idx != superblock->total_fat_blks + 1) {
        return -1;
    }
    if (superblock->data_blk_idx != superblock->root_dir_idx + 1) {
        return -1;
    }
    if (superblock->total_data_blks == 0) {
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

    /* set default fd opened files */
    File_descriptors = (Fd_t)malloc(FS_OPEN_MAX_COUNT * sizeof(struct Fd));
    if (File_descriptors == NULL) {
        return -1;
    }
    for (int i = 0; i < 32; i++) {
        File_descriptors[i].open_file = NULL;
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
        if (File_descriptors[i].open_file != NULL) {
            return -1;
        }
    }
    /* write & check superblock */
    if (block_write(0, superblock) == -1) {
        return -1;
    }

    /* write & check fat blocks */
    for (int i = 0; i < superblock->total_fat_blks; i++) {
        if (block_write(i+1, fat_array + (i * BLOCK_SIZE) / 2) == -1) {
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
    if (superblock != NULL) {
        free(superblock);
    }
    if (root_dir != NULL) {
        free(root_dir);
    }
    if (fat_array != NULL) {
        free(fat_array);
    }
    if (File_descriptors != NULL) {
        free(File_descriptors);
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
    int free_blocks_count = 0;
    for (int i = 0; i < superblock->total_data_blks; i++) {
        if (fat_array[i] == 0) {
            free_blocks_count++;
        }
    }

    /* count the number of free root directories*/
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
    printf("fat_free_ratio=%u/%u\n", free_blocks_count, superblock->total_data_blks);
    printf("rdir_free_ratio=%u/%u\n", free_rdir_count, 128);

    return 0;
}

int fs_create(const char *filename)
{
    /* check valid filename */
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
    if (strlen(filename) == 0 || strlen(filename) > FS_FILENAME_LEN) {
        return -1;
    }

    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        if (File_descriptors->open_file != NULL && strcmp(filename, (char*)File_descriptors->open_file->filename) == 0) {
            return -1;
        }
    }

    int targetIndex = -1;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (strcmp(filename, (char*)root_dir[i].filename) == 0 && targetIndex == -1) {
            targetIndex = i;
        }
    }
    if (targetIndex == -1) {
        return -1;
    }

    /* free file's conetent in FAT */
    int delete_blk_idx = root_dir[targetIndex].first_blk_index;
    while (delete_blk_idx != FAT_EOC) {
        uint16_t temp = fat_array[delete_blk_idx];
        fat_array[delete_blk_idx] = 0;
        delete_blk_idx = temp;
    }

    /* reset related content in root directory */
    memset(&(root_dir[targetIndex]), 0, BLOCK_SIZE/FS_FILE_MAX_COUNT);
    strcpy((char*)root_dir[targetIndex].filename, "\0");
    root_dir[targetIndex].filesize = 0;
    root_dir[targetIndex].first_blk_index = 0;
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
    if (strlen(filename) == 0 || strlen(filename) > FS_FILENAME_LEN) {
        return -1;
    }

    /* find file location */
    int file_location = -1;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (strcmp(filename, (char*)root_dir[i].filename) == 0 && file_location == -1) {
            file_location = i;
        }
    }
    // file named filename not found
    if (file_location == -1) {
        return -1;
    }

    /* find free file descriptor location */
    int fd_idx = -1;
    for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
        if (fd_idx == -1 && File_descriptors[i].open_file == NULL) {
            fd_idx = i;
        }
    }
    // no fd opening
    if (fd_idx == -1) {
        return -1;
    }

    File_descriptors[fd_idx].open_file = &(root_dir[file_location]);
    File_descriptors[fd_idx].offset = 0;

    return fd_idx;
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (File_descriptors[fd].open_file == NULL) {
        return -1;
    }

    File_descriptors[fd].open_file = NULL;
    File_descriptors[fd].offset = 0;

    return 0;
}

int fs_stat(int fd)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (File_descriptors[fd].open_file == NULL) {
        return -1;
    }

    return File_descriptors[fd].open_file->filesize;
}

int fs_lseek(int fd, size_t offset)
{
    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (File_descriptors[fd].open_file == NULL) {
        return -1;
    }

    if (offset > File_descriptors[fd].open_file->filesize) {
        return -1;
    }

    File_descriptors[fd].offset = offset;

    return 0;
}


int write_allocate(Fd_t fd_write, int new_size){
  //fat_array
    int index = fd_write->open_file->first_blk_index + ((fd_write->open_file->filesize - 1)/ BLOCK_SIZE);
    int k;
    for (k = 1; k < superblock->total_data_blks; k++) {
        if (fat_array[k] == 0) {
            fat_array[index] = k;
            break;
        }
        if (k == superblock->total_data_blks - 1 && fat_array[k] != 0) {
            fat_array[index] = FAT_EOC;
        }
    }
    int j;
    for (int i = (fd_write->open_file->filesize - 1)/ BLOCK_SIZE; i < ((new_size-1)/BLOCK_SIZE); i++) {
        index = fat_array[index];
        for (j = fat_array[index]; j < superblock->total_data_blks; j++) {
          if (fat_array[j] == 0) {
              fat_array[index] = j;
              break;
            }
            if (j == superblock->total_data_blks - 1 && fat_array[j] != 0) {
                fat_array[index] = FAT_EOC;
            }
        }
        if (fat_array[index] ==FAT_EOC) {
            new_size = (i+1) * BLOCK_SIZE;
            fd_write->open_file->filesize = new_size;
            return new_size;
        }

    }

    fat_array[index] = FAT_EOC;
    fd_write->open_file->filesize = new_size;
    return new_size;
}


int fs_write(int fd, void *buf, size_t count)
{
    char* bounce_buffer = (char*)malloc(BLOCK_SIZE * sizeof(char));

    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }
    if (File_descriptors[fd].open_file == NULL) {
        return -1;
    }

    if (count == 0) {
        return 0;
    }
    //if no space
    if (File_descriptors[fd].open_file->first_blk_index == FAT_EOC) {
        //extend
        size_t cur_file_blk = File_descriptors[fd].offset + count;
        if(cur_file_blk > File_descriptors[fd].open_file->filesize) {
            int actual_count = write_allocate(File_descriptors,cur_file_blk);
            count = actual_count - File_descriptors[fd].offset;
        }

        size_t new_free_block;
        for (int i = 0; i < superblock->total_data_blks;i++) {
            if (i == superblock->total_data_blks) {
                new_free_block = 0;
            }
            if (fat_array[i] == 0) {
              new_free_block = i;
              break;
            }
        }
        fat_array[new_free_block] = FAT_EOC;

        File_descriptors[fd].open_file->first_blk_index = new_free_block;
        if (File_descriptors[fd].open_file->first_blk_index == FAT_EOC) {
            return 0;
        }
    }






    //new first block index
    uint16_t head_index = File_descriptors[fd].open_file->first_blk_index + superblock->data_blk_idx;
    //read first block
    block_read(head_index, bounce_buffer);
    size_t byte_offset = File_descriptors[fd].offset % BLOCK_SIZE;
    memcpy(bounce_buffer + byte_offset, buf, (BLOCK_SIZE - byte_offset));
    //int write_num = block_write(head_index, bounce_buffer);
    block_write(head_index, bounce_buffer);

    //need update size
    //momomomomomogaigai
    buf += BLOCK_SIZE - byte_offset;
    size_t byte_count = count / BLOCK_SIZE + 1;
    head_index = fat_array[head_index];
    for (int i = 1; i < byte_count-1; i++) {
        block_write(head_index, buf);
        buf += BLOCK_SIZE;
        head_index = fat_array[head_index];
    }
    if (head_index != FAT_EOC) {
        size_t byte_rem = (byte_count + File_descriptors[fd].offset) % BLOCK_SIZE;
        block_read(head_index, bounce_buffer);
        memcpy(bounce_buffer + byte_offset, buf, byte_rem);
        //int write_num = block_write(head_index, bounce_buffer);
        block_write(head_index, bounce_buffer);
    }

    File_descriptors[fd].offset += byte_count;
    return byte_count;
}


int fs_read(int fd, void *buf, size_t count)
{
    // if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
    //     return -1;
    // }
    //
    // if (File_descriptors[fd].open_file == NULL) {
    //     return -1;
    // }

    // /* find length of bounce buffer */
    // int start_idx = File_descriptors[fd].offset / BLOCK_SIZE;
    // int end_idx = (offset + count - 1) / BLOCK_SIZE;

    // uint16_t temp = File_descriptors[fd].open_file.first_blk_index;
    // int last_idx = -1;
    // while (temp != FAT_EOC) {
    //     last_idx++;
    //     temp = fat_array[temp];
    // }
    // if (end_idx > last_idx) {
    //     end_idx = last_idx;
    // }
    // int length = end_idx - start_idx + 1;

    // /* get bounce buffer */
    // uint16_t bounce_buffer = (uint16_t*)malloc(BLOCK_SIZE / 2 * length * sizeof(uint16_t));

    // temp = File_descriptors[fd].open_file.first_blk_index;
    // for (int i = 0; i < )

    // int first_block_idx = File_descriptors[fd].offset / BLOCK_SIZE;
    // int last_block_idx  = (offset + count - 1) / BLOCK_SIZE;
    //
    // uint16_t temp = File_descriptors[fd].open_file.first_blk_index;
    // int last_fat_blk_idx = -1;
    // while (temp != FAT_EOC) {
    //     last_fat_blk_idx++;
    //     temp = fat_array[temp];
    // }
    //
    // if (last_block_idx > last_fat_blk_idx) last_block_idx = last_fat_blk_idx;

    if (fd < 0 || fd >= FS_OPEN_MAX_COUNT) {
        return -1;
    }

    if (File_descriptors[fd].open_file == NULL) {
        return -1;
    }


    if (File_descriptors[fd].open_file->filesize <= File_descriptors[fd].offset) {
    	return 0;
    }

    //offset reach eof
    if (File_descriptors[fd].open_file->first_blk_index == FAT_EOC) {
    	return 0;
    }


    int start_idx = File_descriptors[fd].offset / BLOCK_SIZE;
    int end_idx = (File_descriptors[fd].offset + count - 1) / BLOCK_SIZE;

    uint16_t temp = File_descriptors[fd].open_file->first_blk_index;
    int last_idx = -1;
    while (temp != FAT_EOC) {
        last_idx++;
        temp = fat_array[temp];
    }
    if (end_idx > last_idx) {
        end_idx = last_idx;
    }
    int length = end_idx - start_idx + 1;
    char* bounce_buffer = (char*)malloc(BLOCK_SIZE / 2 * length * sizeof(char));
    for (int i = 0;i < superblock->total_data_blks;i++) {
    	block_read(File_descriptors[fd].open_file->first_blk_index + superblock->data_blk_idx, bounce_buffer + (BLOCK_SIZE * i));
    }

    //whether count > offset
    size_t offset_count;
    if ((File_descriptors->open_file->filesize - File_descriptors->offset) < count) {
    	offset_count = File_descriptors->open_file->filesize - File_descriptors->offset;
    } else {
    	offset_count = count;
    }

    //copy data from bounce_buffer
    size_t file_offset = File_descriptors->offset % BLOCK_SIZE;
    memcpy(buf, (file_offset + bounce_buffer), offset_count);
    return offset_count;
}
