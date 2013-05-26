#ifndef __INCLUDE_SBA_IOCTLS_H__
#define __INCLUDE_SBA_IOCTLS_H__

/*file system types*/
#define EXT3		0
#define REISERFS	1

/*error types*/
#define SBA_FAIL	0
#define SBA_CORRUPT	1

/*error modes*/
#define TRANSIENT	0
#define STICKY		1

/*This structure encodes the fault type that will
 *be caused*/
typedef struct _fault {
	int rw;			//inject during read or write
	int fault_type;	//fault type: fail or corrupt
	int fault_mode;	//transient or sticky
	int filesystem;	//file system
	int blk_type;	//block type
	int offset;		//optional
	int size;		//optional
	int blocknr;	//optional
	int inodenr;	//optional
} fault;

#define START_SBA		6000
#define STOP_SBA		6001
#define START_TRACING	6002
#define STOP_TRACING	6003
#define INJECT_FAULT	6004
#define PRINT_JOURNAL	6005

/* Types of Blocks */
#define SBA_EXT3_UNKNOWN	0x0000
#define SBA_EXT3_INODE 		0x0001
#define SBA_EXT3_DBITMAP	0x0002
#define SBA_EXT3_IBITMAP	0x0004
#define SBA_EXT3_SUPER	 	0x0008
#define SBA_EXT3_GROUP 		0x0010
#define SBA_EXT3_DATA		0x0020
#define SBA_EXT3_REVOKE		0x0040
#define SBA_EXT3_DESC		0x0080
#define SBA_EXT3_COMMIT		0x0100
#define SBA_EXT3_JSUPER		0x0200
#define SBA_EXT3_JDATA		0x0400
#define SBA_EXT3_DIR		0x0800
#define SBA_EXT3_INDIR		0x1000

#define WR_PATTERN	"hello cruel world!"

#endif /* __INCLUDE_SBA_IOCTLS_H__ */
