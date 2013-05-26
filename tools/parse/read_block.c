#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	int fd;
	int block;
	char buf[4096];

	if (argc != 3) {
		printf("Usage: read_block <dev> <block>\n");
		return -1;
	}

	fd = open (argv[1], O_RDONLY);
	block = atoi(argv[2]);
	lseek(fd, block*4096, SEEK_SET);
	if (read(fd, buf, 4096) != 4096) {
		perror("read");
		return -1;
	}

	write(1, buf, 4096);
	close(fd);

	return 1;
}
