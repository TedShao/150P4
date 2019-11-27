# Program 4 Report: File system
  Authors: Junting Zhang, Qiyun Shao 
  SID: 914222589, 913195219
  This report has the following sections
  * Overview 
  * Phase 1: Mounting / unmounting
  * Phase 2: File creation / deletion
  * Phase 3: File descriptor operations
  * Phase 4: File reading / writing
  * Testing 
  * Resources
  
# Overview 
Overall, in this project we implemented a FAT-based filesystem software stack from mounting 
and unmounting a formatted partition, to reading and writing files, and including creating and
removing files. 
  
# Phase 1: Mounting / Unmounting

## Data Structures
In this phase, we implemented four data strucutures mentioned in the prompt. We created a
struct named `Superblock` which is the first block of the file system and it stores information 
of the file system such as Signature, Total amount of blocks of virtual disk, and etc. We 
declared a global variable named `superblock` which is a pointer to a `Superblock` struct. 
Another global variable that we created is `fat_array`, whose entries are composed of 16-bit 
unsigned words. We created a struct named `Root_dir` to store the descriptions (including
`filename`, `filesize` and etc.) of files. We created an list of `Root_dir` called `root_dir` to 
store a maximum of 128 `Root_dir_t` entries. The last struct `Fd` will be described in Phase 3. 
## `fs_mount()`
We used `block_disk_open()` to open disk and `block_read()` to read the block with index
of 0 to `superblock`. If one of the two functions return -1, our `fs_mount()` will return -1 as 
an error indicator. Then, we checked if `superblock` has read correct info and then we read 
and check fat blocks. Since each entry in `fat_array` is 2 bytes, in each block there will be 
2048 fat entries. Hence we allocated `fat_array`'s memory space and use `block_read()`
to read fat blocks, and set the first entry of `fat_array` to `FAT_EOC`. We also allocated 128 
entries size to `root_dir` and used `block_read()` to read `root_dir`. The initialization of 
`Fd` array will be described in Phase 3.
## `fs_umount()`
Before `fs_umount()` we checked if there is a virtual disk opened or not by checking if 
`block_disk_count()` equals to -1. Then we write back `superblock`, `fat_array`, and 
`root_dir`'s contents using `block_write()`. We used `block_disk_close()` to close disk
and at last, we free the allocated memory in `fs_mount()` if there is any. 
## `fs_info()`
Before `fs_info()` we checked if there is a virtual disk opened or not by checking if 
`block_disk_count()` equals to -1. Then we counted the number of free data blocks by 
counting total number of `fat_array` entries that equal to 0. We also counted the number of 
free root directories by counting the total number of `root_dir` entries whose `filename` is 
empty. Then we `printf()` all needed info and return 0. 

# Phase 2: File creation / deletion

## `fs_create()`
We first checked if the input `filename` is valid in terms of length. Then, we checked if the 
`filename` is already existed with `strcmp()` and if there is no available space in `root_dir`. 
If no error case is satisfied, we reseted the available `root_dir` entry using `memset()`, 
assigned `filename`, set `filesize` to 0, and `first_blk_index` to `FAT_EOC` which is 
`0xFFFF`.
## `fs_delete()`
We first checked if the input `filename` is valid in terms of length. Then, we checked if the 
`filename` is existed to be deleted. We freed the file's content in `fat_array` from the 
`root_dir` entry's `first_blk_index` to the first `FAT_EOC` after `first_blk_index`. At 
last, we reseted the deleted `root_dir` entry using `memset()`, reset `filename`, set 
`filesize` to 0, and `first_blk_index` to 0.
## `fs_ls()` 
Again, we used `block_disk_count()` to check if there is no underlying virtual disk opened.
Then we followed the instructions and `printf()` the infomation of `root_dir` entries whose
`filename` is not empty. 

# Phase 3: File descriptor operations

## Data Strucutures
In this phase we implemented a new struct named `Fd` (file descriptor) which consists a pointer
named `open_file` that points to a specific `Root_dir` and an `offset`. In addition, We 
created an list of `Fd` called `File_descriptors` to store a maximum of 32 `Fd_t` entries. If a 
file is opened, the `Fd` entry's `open_file` will point to the precise `Root_dir`. Otherwise, 
`open_file` is assigned with `NULL`.
## Update `fs_mount()`, `fs_umount()`, and `fs_delete()`
Since we didn't consider the `Fd` struct in Phase 1 & 2, we needed to have some adjustments
to some functions implemented in these phases. In `fs_mount()`, similar to `root_dir`, we 
also needed to allocate space for our `File_descriptors` array (but with only 32 entries). In
`fs_umount()`, before we started the write back process, we needed to check if there is any 
opened file by comparing each `File_descriptors` entry's `open_file` to `NULL`. If any 
`File_descriptors` entry's `open_file` is not equivalent to `NULL`, `fs_umount()` returns 
-1 since there exist one or more unclosed files. If `fs_umount()` successed, the memory
space allocated for `File_descriptors` also needed to be freed. In `fs_delete()`, before 
the delete process, we needed to check if the deleting file is opened or not. If opened, 
`fs_delete()` outputs an error. 
## `fs_open()`
We first checked if the input `filename` is valid in terms of length. Then we iterate through 
`root_dir` to check if there exists a file named `filename` to be opened. After that, we 
checked if there is an availble space in `File_descriptors`. If none of the error conditions is 
satisfied, we assigned the available `File_descriptors` entry with a reference of the 
corresponding `root_dir` entry and an `offset` of 0. 
## `fs_close()`
We first checked if the input `fd` is valid in terms of length (0->31). Then, we checked if the 
target file is opened or not. If it is opened, we set the file's `File_descriptors` entry's 
`open_file` to `NULL` and `offset` to 0.
## `fs_stat()`
We first checked if the input `fd` is valid in terms of length (0->31). Then, we checked if the 
target file is opened or not. If it is opened, we return the `filesize` of `open_file`
## `fs_lseek()`
We first checked if the input `fd` is valid in terms of length (0->31). Then, we checked if the 
target file is opened or not. We also checked if the `offset` is out of bound by comparing it
with `filesize` of `open_file`. If no error condition is satisfied, we asssign the corresponding
`File_descriptors` entry's `offset` to input `offset` and return 0.

# Phase 4: File reading / writing

## `fs_read()`


## `fs_write()`
