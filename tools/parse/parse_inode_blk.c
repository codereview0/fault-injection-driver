#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/ext3_fs.h>

int main(int argc, char *argv[])
{
	int ino;
	int i, j;
	char buf[256];
	struct ext3_inode *inode;

	if (argc < 2) {
		printf("usage: parse_inode_blk <inode no>");
		return 1;
	}

	ino = atoi(argv[1]);

	for(i = 0; i< 32; i++) {
		read(0, buf, sizeof(struct ext3_inode));
		inode = (struct ext3_inode*)buf;

		if (i == ino) { //only for log
			printf("ino#%d: mode=%d size=%d mtime=%d dtime=%d\n", i, inode->i_mode, inode->i_size, inode->i_mtime, inode->i_dtime);
			for (j = 0; j < 15; j ++) {
				printf("bptr[%d]=%d\n", j, inode->i_block[j]);
			}
		}
	}

	return 1;
}
