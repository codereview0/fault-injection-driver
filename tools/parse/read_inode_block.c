#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/ext2_fs.h>

#define SDS_BLKSIZE					4096
#define SDS_BLKS_PER_GP				(SDS_BLKSIZE * 8)
#define SDS_NR_INODES_PER_BLK		(SDS_BLKSIZE/128)
#define SDS_NR_GROUPS(size)			((size/4)/SDS_BLKS_PER_GP + 1)
#define SDS_NR_PTRS_PER_BLK			(SDS_BLKSIZE/sizeof(u32))

/* kludge - FIXME */
//#define SDS_INODE_BLKS_PER_GROUP	973 
#define SDS_INODE_BLKS_PER_GROUP	506 
#define SDS_INODET_OFF 4			/* inode table is 4 blk away */
#define SDS_BITMAP_OFF 2			/* bitmaps are 2 blks away */

#define SDS_SECTOR_TO_BLOCK(a)	((a) >> (SDS_BLKSIZE_BITS - SDS_HARDSECT_BITS))
#define SDS_BLOCK_TO_SECTOR(a)	((a) << (SDS_BLKSIZE_BITS - SDS_HARDSECT_BITS))
#define SDS_INODES_PER_GP		(SDS_INODE_BLKS_PER_GROUP * SDS_NR_INODES_PER_BLK)
#define SDS_SECS_PER_GP			(((SDS_BLKSIZE) * (SDS_BLKS_PER_GP)) / (SDS_HARDSECT))
#define SDS_GP_NO(s)			((s) / SDS_SECS_PER_GP)
#define SDS_INODE_TABLE(g)		(((g) * (SDS_BLKS_PER_GP)) + SDS_INODET_OFF)
#define SDS_INODE_TABLE_NEW(g)	(((g) * (SDS_BLKS_PER_GP)) + 2)
#define SDS_GET_SECT(b)			(((b) * (SDS_BLKSIZE)) / (SDS_HARDSECT))
#define SDS_GET_BITMAP_BLK(g) 	((g%2)?((g*SDS_BLKS_PER_GP)+SDS_BITMAP_OFF):(g*SDS_BLKS_PER_GP))
#define SDS_BLOCK_TO_SECTOR_v2(b,s)	(((b)*(s))/SDS_HARDSECT)

int main(int argc, char **argv)
{
	int fd;
	int block;
	int inode_no;
	int group;
	int inode_start;
	int offset_inodes;
	int i_offset;
	char buf[4096];
	int j;
	char *buf1;
	struct ext2_inode *inode;

	if (argc != 3) {
		printf("Usage: read_block <dev> <inode number>\n");
		return -1;
	}

	fd = open (argv[1], O_RDONLY);
	inode_no = atoi(argv[2]);

	inode_no --;
	group = inode_no / SDS_INODES_PER_GP;

	if (group <= 9)
		inode_start = SDS_INODE_TABLE(group);
	else
		inode_start = SDS_INODE_TABLE_NEW(group);

	printf("inode table starts at %d\n", inode_start);

	offset_inodes = inode_no - (group * SDS_INODES_PER_GP);
	block = inode_start + offset_inodes/SDS_NR_INODES_PER_BLK;
	i_offset = offset_inodes % SDS_NR_INODES_PER_BLK;

	printf("inode block = %d for inode %d\n", block, inode_no);
	
	lseek(fd, block*4096, SEEK_SET);
	if (read(fd, buf, 4096) != 4096) {
		perror("read");
		return -1;
	}

	close(fd);

	buf1 = buf + i_offset * sizeof(struct ext2_inode);
	inode = (struct ext2_inode*)buf1;

	printf("inodenr = %d blocknr = %d ioffset = %d\n", inode_no, block, i_offset);
	printf("ino#%d: iblocks=%d mode=%d size=%d mtime=%d dtime=%d links=%d\n", inode_no, inode->i_blocks, inode->i_mode, inode->i_size, inode->i_mtime, inode->i_dtime, inode->i_links_count);
	for (j = 0; j < 15; j ++) {
		printf("bptr[%d]=%d\n", j, inode->i_block[j]);
	}
	return 1;
}
