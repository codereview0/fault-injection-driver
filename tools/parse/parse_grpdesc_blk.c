#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ext3_fs.h>

int main()
{
	int i;
	char *buf = (char*)malloc(4096);
	struct ext3_group_desc *gd;

	read(0, buf, 4096);
	gd = (struct ext3_group_desc *)buf;

	printf("size of GDesc: %d\n", sizeof(*gd));
	printf("GDesc per block: %d\n", 4096/sizeof(*gd));

	for (i = 0; i < 128; i ++) {
		gd = ((struct ext3_group_desc *)buf) + i;

		if (gd->bg_block_bitmap == 0) {
			break;
		}

		printf("GDesc %d: blockbitmap = %d, inodebitmap = %d, inodetable = %d\n", 
		i, gd->bg_block_bitmap, gd->bg_inode_bitmap, gd->bg_inode_table);
	}

	return 1;
}
