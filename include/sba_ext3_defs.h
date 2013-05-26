#ifndef __INCLUDE_SBA_EXT3_DEFS_H__
#define __INCLUDE_SBA_EXT3_DEFS_H__

#include "sba_common_defs.h"

#define SBA_BLKS_PER_GP				(SBA_BLKSIZE * 8)
#define SBA_NR_INODES_PER_BLK		(SBA_BLKSIZE/128)
#define SBA_NR_GROUPS(size)			((size/4)/SBA_BLKS_PER_GP + 1)
#define SBA_NR_PTRS_PER_BLK			(SBA_BLKSIZE/sizeof(u32))

/* kludge - FIXME */
//#define SBA_EXT3_INODE_BLKS_PER_GROUP	484 
#define SBA_EXT3_INODE_BLKS_PER_GROUP	498 
#define SBA_INODET_OFF 4			/* inode table is 4 blk away */
#define SBA_BITMAP_OFF 2			/* bitmaps are 2 blks away */

//#define SBA_INODES_PER_GP		(SBA_INODE_BLKS_PER_GROUP * SBA_NR_INODES_PER_BLK)
#define SBA_SECS_PER_GP			(((SBA_BLKSIZE) * (SBA_BLKS_PER_GP)) / (SBA_HARDSECT))
#define SBA_GP_NO(s)			((s) / SBA_SECS_PER_GP)

#define SBA_INODE_TABLE(g)		(((g) * (SBA_BLKS_PER_GP)) + SBA_INODET_OFF)

#define SBA_GET_SECT(b)			(((b) * (SBA_BLKSIZE)) / (SBA_HARDSECT))
#define SBA_GET_BITMAP_BLK(g) 	((g%2)?((g*SBA_BLKS_PER_GP)+SBA_BITMAP_OFF):(g*SBA_BLKS_PER_GP))
#define SBA_BLOCK_TO_SECTOR_v2(b,s)	(((b)*(s))/SBA_HARDSECT)

//vp - FIXME
#define JOURNAL_INODE_BLK	4
#define JOURNAL_INODE_NO	7


#endif
