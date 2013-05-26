#ifndef __COORDINATOR_H__
#define __COORDINATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/ext3_fs.h>
#include "sba_common_defs.h"
#include "fslib.h"
#include "posix_lib.h"

#define DEVICE	"/dev/SBA"

#define MAX_WORKLOAD			100

typedef struct _workload {
	int total;
	int blk_type;
	int *id;
	int *minor_blktype; /*this is for declassifying inodes into file/dir/symboliclnk */
} workload;

#define POS_ACCESS				2342
#define POS_CHDIR				2343
#define POS_CHMOD				2344
#define POS_CHOWN				2345
#define POS_CHROOT				2346
#define POS_CREAT				2347
#define POS_STAT				2348
#define POS_STATFS				2349
#define POS_FSYNC				2350
#define POS_TRUNCATE			2351
#define POS_GETDIRENT			2352
#define POS_LINK				2353
#define POS_LSTAT				2354
#define POS_MKDIR				2355
#define POS_MOUNT				2356
#define POS_CREATOPEN			2357
#define POS_OPEN				2358
#define POS_READ				2359
#define POS_READLINK			2360
#define POS_RENAME				2361
#define POS_RMDIR				2362
#define POS_SYMLINK				2363
#define POS_SYNC				2364
#define POS_UMASK				2365
#define POS_UNLINK				2366
#define POS_UMOUNT				2367
#define POS_UTIMES				2368
#define POS_WRITE				2369
#define POS_CLOSE				2370
/*special cases*/
#define SINDIR_WORKLOAD			2371
#define DINDIR_WORKLOAD			2372
#define REVOKE_WORKLOAD			2373
#define DIRCRASH_WORKLOAD		2374
#define FILECRASH_WORKLOAD		2375

/*minor block types*/
#define FILE_INODE				3579
#define DIR_INODE				3580
#define SYMLINK_INODE			3581

int init_sys();
int cleanup_sys();
int wait_for_response();
char *get_block_type_str(int btype);
int print_fault(fault *fs_fault, char *fault_info);
int get_fault_specification(char *fault_spec);
int reinit_fault();
int inject_fault();
int remove_fault();
int init_dir_blocks(unsigned long inodenr);
int init_indir_blocks(unsigned long inodenr);
int fault_injected();
int wait_for_checkpoint();
int revoke_workload(int crash_commit);
int my_double_indir_workload();
int my_single_indir_workload();
int get_probable_workload();
int run_all_workloads(int blocktype);
int notify_sba();
int clear_all();
int create_filesystem();
int build_fs();
int create_test_state(int block_type);
int unmount_filesystem();
int mount_filesystem();
int flush_from_cache();
int create_setup(int block_type);
int get_next_block(char *input_file, fault *fs_fault);
int get_block_type(char *btype);

int test_system(char *input_file);
workload *get_all_workloads(int block_type);
int run_all_workloads(int block_type);
int construct_fault(char *path, int blknr, int minor_blktype);
int coord_create_file(char *name, char *size);
int prepare_for_testing(char *filename, int blknr, int minor_blktype);
int run_workload(int workload, int blocktype, int minor_blktype);
int run_tests(int callid, int argc, void *argv[]);

#endif
