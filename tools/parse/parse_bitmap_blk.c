#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main()
{
	int i, j;
	char *buf = (char*)malloc(4096);

	read(0, buf, 4096);

	for (i = 0; i < 4096; i ++) {
		if (buf[i]) {
			for (j = 0; j < 8; j ++) {
				if (buf[i] & (1 << (7-j))) {
					printf("blk %d is allocated\n", i*8 + j);
				}
				else {
					printf("blk %d is free\n", i*8 + j);
				}
			}
		}
		else {
			for (j = 0; j < 8; j ++) {
				printf("blk %d is free\n", i*8 + j);
			}
		}
	}

	return 1;
}
