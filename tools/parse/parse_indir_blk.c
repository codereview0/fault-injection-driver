#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/ext2_fs.h>

int main()
{
	int i;
	char buf[4096];
	int *blkno;

	read(0, buf, 4096);
	blkno = (int*)buf;

	for(i = 0; i< 1024; i++) {
		printf("buf[%d] = %d\n", i, *(int *)(blkno + i));
	}

	return 1;
}
