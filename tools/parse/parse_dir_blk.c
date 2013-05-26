#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/ext3_fs.h>

int main()
{
	int pos = 0;
	char *buf = (char*)malloc(4096);
	struct ext3_dir_entry_2 *de;

	read(0, buf, 4096);

	while(pos < 4096) {
		de = (struct ext3_dir_entry_2 *)buf;
		printf("Directory: Inode = %d, rec_len = %d, name_len = %d, name = %s\n", de->inode, de->rec_len, de->name_len, de->name);

		buf += de->rec_len;
		pos += de->rec_len;
	}

	return 1;
}
