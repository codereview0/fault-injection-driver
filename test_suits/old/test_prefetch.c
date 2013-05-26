//cc -I/root/vijayan/sosp05/analysis/include -I/usr/src/linux-2.4/include/ -Wall -O6 -g -o test_prefetch test_prefetch.c
#include "fslib.h"

int main(int argc, char *argv[])
{
	int fd;
	int blk;
	char buffer[4096];
	char *file = argv[1];

	if (argc != 3) {
		printf("usage: test_prefetch <filename> <blk#>\n");
		return -1;
	}

	printf("opening file %s\n", file);
	if ((fd = open(file, O_RDONLY)) < 0) {
		perror("open");
		return -1;
	}

	blk = atoi(argv[2]);

	printf("begin seek (%d) ...\n", blk);
	if (lseek(fd, blk*4096, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	printf("begin read (%d) ...\n", blk);
	read(fd, buffer, 4096);
	printf("end read %s ...\n", buffer);

	close(fd);

	return 1;
}
