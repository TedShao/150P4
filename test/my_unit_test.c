#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fs.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define test_fs_error(fmt, ...) \
	fprintf(stderr, "%s: "fmt"\n", __func__, ##__VA_ARGS__)

#define die(...)				\
do {							\
	test_fs_error(__VA_ARGS__);	\
	exit(1);					\
} while (0)

#define die_perror(msg)			\
do {							\
	perror(msg);				\
	exit(1);					\
} while (0)


void test_mount(){

}

void test_umount(){

}

void test_info(){

}

void test_create(){

}

void test_delete(){

}

void test_ls(){

}

void test_open(){

}

void test_close(){

}

void test_stat(){

}

void test_lseek(){

}

void test_write(){

}

void test_read(){

}

int main(void)
{
		char *disk1 = "./fs_make.x" disk1.fs "4096";
		char *disk1 = "./fs_make.x" disk2.fs "4096";
		char *disk1 = "./fs_make.x" disk3.fs "4096";

        printf("\n------- Testing starts -------\n");

        test_mount();

        test_umount();

        test_info();

        test_create();

        test_delete();

        test_ls();

        test_open();

        test_close();

        test_stat();

        test_lseek();

        test_write();

        test_read();

        printf("\n-------  Testing ends  -------\n");

        char *clean_disk = "rm -f " disk1.fs " " disk2.fs " " disk3.fs;
}