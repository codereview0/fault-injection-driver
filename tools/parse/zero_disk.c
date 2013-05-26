#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	int i;
	int fd;
	int from;
	int to;
	char buf[4096];

	if (argc != 4) {
		printf("Usage: read_block <dev> <from block#(4KB blocks)> <to block#(4KB blocks)>\n");
		return -1;
	}

	if ((fd = open (argv[1], O_WRONLY)) < 0) {
		perror("open");
		return -1;
	}
	
	from = atoi(argv[2]);
	to = atoi(argv[3]);
	memset(buf, 0, 4096);

	/* go to the 'from' block */
	lseek(fd, from*4096, SEEK_SET);
	
	for (i = from; i <= to; i ++) {
		write(fd, buf, 4096);
	}

	fsync(fd);
	close(fd);

	return 1;
}
