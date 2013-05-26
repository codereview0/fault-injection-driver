#ifndef __INCLUDE_SBA_COMMON_DEFS_H__
#define __INCLUDE_SBA_COMMON_DEFS_H__

#define COLLECT_STAT 

#define INC_EXT3
//#define INC_REISERFS
//#define INC_JFS

#define SAME_DEV		0
#define SEP_DEV			1
#define JOUR_MINOR		1

#define SBA_MAJOR		0
#define SBA_DEVS		1	/*these are # devices exported by SBA*/
#define SBA_PHY_DEVS	1	/*these are the # underlying phy devices*/

#define SBA_BLKSIZE			4096	/* block size */
#define SBA_BLKSIZE_BITS	12		/* no of bits - BLKSIZE */
#define SBA_HARDSECT		512		/* sector size */
#define SBA_HARDSECT_BITS	9		/* no of bits - HARDSECT */

#define SBA_SECTOR_TO_BLOCK(a)	((a) >> (SBA_BLKSIZE_BITS - SBA_HARDSECT_BITS))
#define SBA_BLOCK_TO_SECTOR(a)	((a) << (SBA_BLKSIZE_BITS - SBA_HARDSECT_BITS))

//#define SBA_SIZE			248976  /* no of blocks in each of sba device (KB) */
//#define SBA_SIZE			498012	/* no of blocks in each of sba device (KB) */
//#define SBA_SIZE			987996  /* no of blocks in each of sba device (KB) */
#define SBA_SIZE			4008216	/* no of blocks in each of sba device (KB) */

//#define SBA_JOURNAL_SIZE	1004028	/* no of blocks in each of sba device (KB) */
#define SBA_JOURNAL_SIZE	4008216	/* no of blocks in each of sba device (KB) */

/* Define debugging macros */
#define SBA_DEBUG
#ifdef SBA_DEBUG
#define sba_debug(n, f, a...)	\
	do {	\
		if (n) {	\
			printk(KERN_DEBUG "(%s, %d): %s: ",	\
			__FILE__, __LINE__, __FUNCTION__);	\
			printk(f, ## a);	\
		}	\
	} while (0)
#else
#define sba_debug(f, a...)	/* */
#endif

/*sba lock definitions*/
#define SBA_LOCK_INIT(a)	spin_lock_init((a))
#define SBA_LOCK(a)			spin_lock((a))
#define SBA_TRYLOCK(a)		spin_trylock((a))
#define SBA_UNLOCK(a)		spin_unlock((a))
#define IS_EVEN(a) 			(!(a & 0x1))

/*file system types*/
#define EXT3		0
#define REISERFS	1
#define JFS			2

/*error types - don't redefine SBA_FAIL and SBA_CORRUPT 
 *with small numbers like 0 and 1 - they might conflict
 *with READ macros and WRITE macros. then we cannot add
 *SBA_FAIL as an event in the statistics
 */
#define SBA_FAIL			9876
#define SBA_CORRUPT			9877
#define SBA_CRASH			9878
#define SBA_DESC			9879
#define SBA_WKLOAD_START	9880
#define SBA_WKLOAD_END		9881

/*io types*/
#define SBA_READ		0
#define SBA_WRITE		1
#define SBA_READ_WRITE	2

/*error modes*/
#define TRANSIENT	0
#define STICKY		1

/*whether to remove the fault or not*/
#define REM_NOFORCE	0
#define REM_FORCE	1

typedef union _fs_spec_fault {
	struct ext3 {
		int inodenr;
		int logical_blocknr;
		int indir_blk;
	} ext3;
} fs_spec_fault; 

/*This structure encodes the fault type that will
 *be caused*/
typedef struct _fault {
	int rw;				//inject during read or write
	int fault_type;		//fault type: fail or corrupt
	int fault_mode;		//transient or sticky
	int filesystem;		//file system
	int blk_type;		//block type
	int blocknr;		//optional

	fs_spec_fault spec;	//file system specific fault specification
} fault;

typedef struct _sba_stat {
	int total_reads;
	int total_writes;
} sba_stat;

/* different ioctls */
#define START_SBA				6000
#define STOP_SBA				6001
#define START_TRACING			6002
#define STOP_TRACING			6003
#define INJECT_FAULT			6004
#define PRINT_JOURNAL			6005
#define PRINT_STAT				6006
#define ZERO_STAT				6007
#define REMOVE_FAULT			6008
#define PRINT_FAULT				6009
#define MOVE_2_START			6010
#define TEST_SYSTEM				6011
#define DONT_TEST				6012
#define SQUASH_WRITES			6013
#define ALLOW_WRITES			6014
#define PRINT_JBLOCKS			6015
#define CLEAN_STAT				6016
#define CLEAN_ALL_STAT			6017
#define FAULT_INJECTED			6018
#define PROCESS_FAULT			6019
#define REINIT_FAULT			6020
#define INIT_DIR_BLKS			6021
#define INIT_INDIR_BLKS			6022
#define EXTRACT_STATS			6023
#define CRASH_SYSTEM			6024
#define DONT_CRASH				6025
#define CRASH_COMMIT			6026
#define DONT_CRASH_COMMIT		6027
#define WORKLOAD_START			6028
#define WORKLOAD_END			6029

/* Types of Blocks */
#define SBA_EXT3_UNKNOWN		0x1000
#define SBA_EXT3_INODE 			0x1001
#define SBA_EXT3_DBITMAP		0x1002
#define SBA_EXT3_IBITMAP		0x1003
#define SBA_EXT3_SUPER	 		0x1004
#define SBA_EXT3_GROUP 			0x1005
#define SBA_EXT3_DATA			0x1006
#define SBA_EXT3_REVOKE			0x1007
#define SBA_EXT3_DESC			0x1008
#define SBA_EXT3_COMMIT			0x1009
#define SBA_EXT3_JSUPER			0x1010
#define SBA_EXT3_JDATA			0x1011
#define SBA_EXT3_DIR			0x1012
#define SBA_EXT3_INDIR			0x1013
#define SBA_EXT3_SINDIR			0x1014
#define SBA_EXT3_DINDIR			0x1015
#define SBA_EXT3_TINDIR			0x1016
#define SBA_EXT3_JINDIR			0x1017
#define ANY_BLOCK				0x1018	
#define JOURNAL_DESC_BLOCK		0x1019	
#define JOURNAL_REVOKE_BLOCK	0x1020	
#define JOURNAL_DATA_BLOCK		0x1021	
#define JOURNAL_COMMIT_BLOCK	0x1022
#define JOURNAL_SUPER_BLOCK		0x1023
#define CHECKPOINT_BLOCK		0x1024
#define ORDERED_BLOCK			0x1025
#define UNORDERED_BLOCK			0x1026
#define UNKNOWN_BLOCK			0x1027
#define	ALL_GROUPS				-999
#define	ALL_GROUPS_BUT_0		-998

/* Types of fields */

/*TOTAL_FIELDS should be the largest number defined for the 
 *following fields + 1. so that it can be used to define arrays*/
#define TOTAL_FIELDS		5 

/*ext3 inode*/
#define EXT3_INODE_I_BLOCK			0
#define EXT3_INODE_BITMAP			1
#define EXT3_DATA_BITMAP			2
#define EXT3_GRPDESC_BLOCK_BITMAP	3
#define EXT3_GRPDESC_INODE_BITMAP	4

#define WR_PATTERN			"hello cruel world!"

/* STATE DEFINITIONS */
#define INVALID_STATE		0
#define VALID_STATE			1

/* RESPONSE DEFINITIONS */
#define WRITE_FAILURE		0
#define WRITE_SUCCESS		1

/* JOURNALING MODE DEFINITIONS */
#define DATA_JOURNALING			1
#define ORDERED_JOURNALING		2
#define WRITEBACK_JOURNALING	3

/*buffer size to store the traces*/
#define MAX_UBUF_SIZE (50*1024*1024)

/*number of msgs sent on each ioctl call*/
#define MAX_MSGS 100000

#endif
