/*
 * Tests are run as a separate process instead of a executing as a 
 * function from this process because we need some fault isolation
 * from the process that is executing the tests.
 */


#include "inject_fault.h"

#define FAULTY_DEV			"/dev/SBA"
#define FAULTY_MOUNT_PT		"/mnt/sba"
#define NONFAULTY_DEV		"/dev/NF_SBA"
#define NONFAULTY_MOUNT_PT	"/mnt/nf_sba"

/*from linux/fs.h */
#define READ 0
#define WRITE 1

#define CORRECT_FS	1
#define FAULTY_FS	0

/* by default, the checkpointing takes place after 5 seconds */
#define CHECKPOINT_INTERVAL	30
#define COMMIT_INTERVAL		3

/*some global values*/
char *junk_log		= "junk.out";
char *logfile		= "logfile";
char *test_suit_prefix;

/* what journaling mode are we targetting ? */
int fs_jmode;

int init_system()
{
	test_suit_prefix = (char *)malloc(256);
	sprintf(test_suit_prefix, "%s/%s %s", TEST_SUITS, COLONY, logfile);
	
	vp_init_lib(logfile);
	return 1;
}

int cleanup_system()
{
	free(test_suit_prefix);
	
	return 1;
}

int unmount_filesystem(int which_fs)
{
	char cmd[4096];

	switch(which_fs) {
		case FAULTY_FS:
			sprintf(cmd, "umount %s", FAULTY_MOUNT_PT);
		break;

		case CORRECT_FS:
			sprintf(cmd, "umount %s", NONFAULTY_MOUNT_PT);
		break;
	}

	system(cmd);

	return 1;
}

int mount_filesystem(int filesystem_type, int which_fs)
{
	char cmd[4096];
	char jmode_str[64];
	char *mount_pt;
	char *dev;
	int ret;

	switch(which_fs) {
		case FAULTY_FS:
			mount_pt = FAULTY_MOUNT_PT;
			dev = FAULTY_DEV;
		break;

		case CORRECT_FS:
			mount_pt = NONFAULTY_MOUNT_PT;
			dev = NONFAULTY_DEV;
		break;

		default:
			printf("Error: unknown file system\n");
			return -1;
	}

	switch (fs_jmode) {
		case DATA_JOURNALING:
			strcpy(jmode_str, "-o data=journal");
		break;

		case ORDERED_JOURNALING:
			strcpy(jmode_str, "-o data=ordered");
		break;

		case WRITEBACK_JOURNALING:
			strcpy(jmode_str, "-o data=writeback");
		break;

		default:
			printf("Error: unknown journaling mode\n");
			return -1;
	}

	switch (filesystem_type) {
		case EXT3:
			sprintf(cmd, "mount -o commit=1 %s %s %s -t ext3 >> %s", jmode_str, dev, mount_pt, logfile);
		break;

		case REISERFS:
			sprintf(cmd, "mount -o commit=1 %s %s %s -t reiserfs >> %s", jmode_str, dev, mount_pt, logfile);
		break;

		case JFS:
			sprintf(cmd, "mount %s %s -t jfs >> %s", dev, mount_pt, logfile);
		break;

		default:
			fprintf(stderr, "Error: unknown file system\n");
			return -1;
	}

	ret = system(cmd);

	return ret;
}

int clean_dir(char *dirname)
{
	char cmd[4096];

	sprintf(cmd, "rm -rf %s/*", dirname);
	printf("Executing %s ...\n", cmd);
	system(cmd);

	return 1;
}

int move_to_clean_state(int filesystem, int state, int which_fs)
{
	char cmd[4096];
	char *mount_pt;
	char *dev;

	printf("Cleaning the file system ...\n");

	switch(which_fs) {
		case FAULTY_FS:
			mount_pt = FAULTY_MOUNT_PT;
			dev = FAULTY_DEV;
		break;

		case CORRECT_FS:
			mount_pt = NONFAULTY_MOUNT_PT;
			dev = NONFAULTY_DEV;
		break;

		default:
			fprintf(stderr, "Error: unknown file system\n");
			return -1;
	}

	/* first unmount and remount the file system 
	 * this should clear most of the errors */
	unmount_filesystem(which_fs);

	/* run e2fsck */
	sprintf(cmd, "e2fsck -fy %s >& %s", dev, junk_log);
	system(cmd);

	mount_filesystem(filesystem, which_fs);
	
	/* clean the file system */
	sprintf(cmd, "rm -rf %s/*", mount_pt);
	system(cmd);

	/* create the new state */
	switch(state) {
		case 0:
		break;

		case 1:
			sprintf(cmd, "./test_suits/colony %s create_dir %s/dir1 0", junk_log, mount_pt);
			system(cmd);

			sprintf(cmd, "./test_suits/colony %s create_dir %s/dir2 0", junk_log, mount_pt);
			system(cmd);
		break;

		case 2:
			sprintf(cmd, "./test_suits/colony %s create_file %s/file 0", junk_log, mount_pt);
			system(cmd);
		break;

		case 3:
			sprintf(cmd, "./test_suits/colony %s create_file %s/file 12", junk_log, mount_pt);
			system(cmd);
		break;
	}

	/* mount it afresh */
	flush_from_cache(filesystem, which_fs);

	printf("Moved the file system to a cleaner state\n");

	return 1;
}

char *get_block_type_str(int btype)
{
	switch(btype) {
		case JOURNAL_DESC_BLOCK:
			return "journal_desc";
		break;

		case JOURNAL_DATA_BLOCK:
			return "journal_data";
		break;

		case JOURNAL_SUPER_BLOCK:
			return "journal_super";
		break;

		case JOURNAL_COMMIT_BLOCK:
			return "journal_commit";
		break;

		case CHECKPOINT_BLOCK:
			return "checkpoint";
		break;

		case ORDERED_BLOCK:
			return "ordered";
		break;

		case UNORDERED_BLOCK:
			return "unordered";
		break;

		default:
			return "unknown";
	}
}

int print_fault(fault *sba_fault)
{
	char *filesystem = "";
	char *rw = "";
	char *ftype = "";
	char *fmode = "";
	char *btype = "";

	printf("<=========Fault Type=========>\n");

	/*Filesystem*/
	switch(sba_fault->filesystem) {
		case EXT3:
			filesystem = "Ext3";
			btype = get_block_type_str(sba_fault->blk_type);
		break;

		case REISERFS:
			filesystem = "Reiserfs";
		break;

		case JFS:
			filesystem = "JFS";
		break;

		default:
			filesystem = "Unknown";
		break;
	}

	/*When*/
	switch(sba_fault->rw) {
		case READ:
			rw = "read";
		break;

		case WRITE:
			rw = "write";
		break;

		default:
			rw = "unknown";
		break;
	}

	/*fault type*/
	switch(sba_fault->fault_type) {
		case SBA_FAIL:
			ftype = "fail";
		break;

		case SBA_CORRUPT:
			ftype = "corrupt";
		break;

		default:
			ftype = "unknown";
		break;
	}
	
	/*fault mode*/
	switch(sba_fault->fault_mode) {
		case STICKY:
			fmode = "sticky";
		break;

		case TRANSIENT:
			fmode = "transient";
		break;

		default:
			fmode = "unknown";
		break;
	}

	printf("FS %s RW %s TYPE %s MODE %s BLK %s\n", filesystem, rw, ftype, fmode, btype);

	return 1;
}

int inject_fault(int dev_fd, int request, void *data)
{
	//printf("Injecting the fault ...\n");
	ioctl(dev_fd, request, data);

	return 1;
}

int wait_for_response()
{
	printf("when complete type \"yes\" to proceed\n");

	while (1) {
		char response[64];
		scanf("%s", response);
		if (strcmp("yes", response) == 0) {
			break;
		}
	}

	return 1;
}

int mount_after_crash(int fd, int filesystem, int which_fs)
{
	unmount_filesystem(which_fs);
	allow_writes(fd);

	/* for jfs the logredo must be run manually */
	if (filesystem == JFS) {
		char cmd[4096];
		char *dev;

		switch(which_fs) {
			case FAULTY_FS:
				dev = FAULTY_DEV;
			break;

			case CORRECT_FS:
				dev = NONFAULTY_DEV;
			break;

			default:
				fprintf(stderr, "Error: unknown file system\n");
				return -1;
		}

		sprintf(cmd, "logredo %s > %s", dev, junk_log);
		printf("running command: %s\n", cmd);
		system(cmd);
		system(cmd);

		if (mount_filesystem(filesystem, which_fs) == 0) {
			return 1;
		}
	}

	mount_filesystem(filesystem, which_fs);

	return 1;
}

int flush_from_cache(int filesystem, int which_fs)
{
	unmount_filesystem(which_fs);
	mount_filesystem(filesystem, which_fs);

	return 1;
}

int move_sba_to_start_state(int dev_fd)
{
	//printf("Moving SBA to start state ...\n");
	ioctl(dev_fd, MOVE_2_START);

	return 1;
}

int stop_verifying(int dev_fd)
{
	//printf("Stoping SBA from verifying requests ...\n");
	ioctl(dev_fd, DONT_VERIFY);

	return 1;
}

int start_verifying(int dev_fd)
{
	//printf("Starting SBA to verify requests ...\n");
	ioctl(dev_fd, VERIFY_MODEL);

	return 1;
}

char *read_block(int dev_fd, int blocknr)
{
	char *buf;
	
	buf = (char *)malloc(4096);
	lseek(dev_fd, 4096*blocknr, SEEK_SET);
	read(dev_fd, buf, 4096);

	return buf;
}

int zero_block(int dev_fd, int blocknr)
{
	char zero_buf[4096];

	memset(zero_buf, 0, 4096);
	lseek(dev_fd, blocknr*4096, SEEK_SET);
	write(dev_fd, zero_buf, 4096);

	return 1;
}

int squash_writes(int dev_fd)
{
	printf("Squashing the writes ...\n");
	ioctl(dev_fd, SQUASH_WRITES);

	return 1;
}

int allow_writes(int dev_fd)
{
	printf("Allowing the writes ...\n");
	ioctl(dev_fd, ALLOW_WRITES);

	return 1;
}

int zero_ext3_journal(int dev_fd)
{
#define JOURNAL_INODE_BLK	4
#define JOURNAL_INODE_NO	7

	int i;
	char *buf;
	char *inode_blk;
	struct ext3_inode *inode;

	/* read the journal inode block */
	inode_blk = read_block(dev_fd, JOURNAL_INODE_BLK);

	/* parse the journal inode structure */
	buf = inode_blk + JOURNAL_INODE_NO*sizeof(struct ext3_inode);
	inode = (struct ext3_inode*)buf;

	for (i = 0; i < 15; i ++) {
		if (inode->i_block[i]) {
			if (i == 0) {
				fprintf(stderr, "Skipping first journal block %d (journal super block)\n", inode->i_block[i]);
				continue;
			}

			/* zero all the blocks pointed to by the single indirect block */
			if (i == 12) {
				char *single_indir;
				int *blkno;
				
				/* read the single indirect block */
				if ((single_indir = read_block(dev_fd, inode->i_block[i])) != NULL) {
					int j;
					blkno = (int *)single_indir;

					for(j = 0; j < 1024; j++) {
						int blk = *(int *)(blkno + j);

						if (blk) {
							zero_block(dev_fd, blk);
						}	
					}

					free(single_indir);
				}
			}
			else
			/* zero all the direct blocks pointed to by the double indirect block */
			if (i == 13) {
				char *double_indir;
				
				if ((double_indir = read_block(dev_fd, inode->i_block[i])) != NULL) {
					int k;
					int *blkno1 = (int *)double_indir;
					
					for(k = 0; k < 1024; k++) {
						int blk1 = *(int *)(blkno1 + k);

						if (blk1) {
							char *single_indir;

							if ((single_indir = read_block(dev_fd, blk1)) != NULL) {
								int j;
								int *blkno2 = (int *)single_indir;

								for(j = 0; j< 1024; j++) {

									int blk2 = *(int *)(blkno2 + j);

									if (blk2) {
										zero_block(dev_fd, blk2);
									}	
								}

								free(single_indir);
							}
						}
					}

					free(double_indir);
				}
			}
			else {
				/* Zero the other blocks */
				zero_block(dev_fd, inode->i_block[i]);
			}

		}
	}

	free(inode_blk);

	return 1;
}

int compare_filesystems(char *f_root, char *c_root)
{
	char cmd[4096];

	
	sprintf(cmd, "echo \"<<------file system differences: Begin------>>\" >> %s", logfile);
	system(cmd);

	sprintf(cmd, "diff -qr %s %s >> %s", f_root, c_root, logfile);
	system(cmd);

	sprintf(cmd, "echo \"<<-------file system differences: End------->>\" >> %s\n", logfile);
	system(cmd);

	return 1;
}

int prepare_system(int dev_fd, fault *sba_fault, int state, int inject)
{
	/* stop sba from verifying this traffic */
	stop_verifying(dev_fd);
	
	/* clean the file system and move sba to start state */
	move_to_clean_state(sba_fault->filesystem, state, CORRECT_FS);
	move_to_clean_state(sba_fault->filesystem, state, FAULTY_FS);
	move_sba_to_start_state(dev_fd);
	
	/* inject the fault and start sba */
	if (inject) {
		inject_fault(dev_fd, INJECT_FAULT, sba_fault);
	}
	start_verifying(dev_fd);
	
	wait_for_response();

	return 1;
}

int fail_reiserfs_super_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 0;
	int inject			= 0;

	printf("Failing the journal super block ...\n");

	/* ------- running the test zero ------- */
	#if 0
	//test where we fail the journal super block write during mount
	//this results in unmountable file system
	prepare_system(dev_fd, sba_fault, state, inject);
	unmount_filesystem(FAULTY_FS);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);
	mount_filesystem(REISERFS, FAULTY_FS);
	wait_for_response();
	#endif

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "Failing journal super block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	/* we don't have to verify with the model anymore */
	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

/*
 * we fail the journal super block write during mount.
 * this crashes the entire file system
 */
int fail_jfs_super_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 0;
	int inject			= 0;

	printf("Failing the journal super block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);
	unmount_filesystem(FAULTY_FS);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	wait_for_response();
	mount_filesystem(JFS, FAULTY_FS);
	wait_for_response();

	sprintf(err_msg, "Failing journal super block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	/* we don't have to verify with the model anymore */
	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

/*
 * in this we fail the journal super block.
 *
 * journal super block has two fields that are of interest here. one 
 * is a state variable, which is cleared to indicate a clean journal.
 * before a transaction, the journal super block is written with this 
 * field containing a non-zero value. after the checkpointing, this 
 * field is cleared and written. the other field is the next transaction
 * id.
 * 
 * if the journal super block write is failed and not taken care of, 
 * two things might happen. one, replay of old contents overwriting 
 * the same contents again. second, non-replay of some valid transaction
 * the requires a replay. usually, replaying again is not a proble. but
 * the second thing (non-replaying) can lead to data corruption, loss 
 * of data and loss of files and dirs.
 *
 * what we do in this test is this:
 * we create a file and fail the first write of the super block, which
 * marks the journal as having a valid transaction. after the commit,
 * we crash the file system and then remount and show loss of files.
 * the above experiment shows loss of files/dirs. similarly, we can show
 * data corruption (failing journal superblock on a file over write)
 * and data loss (failing journal superblock on a file append).
 */

int fail_super_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 0;
	int inject			= 1;

	printf("Failing the journal super block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing journal super block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	/* we don't have to verify with the model anymore */
	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	/* ------- running the second test ------- */
	/* lets move the system to a different state to show data loss */
	state			= 2;
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "\n\nFailing journal super block on a \"append_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	/* we don't have to verify with the model anymore */
	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	/* ------- running the third test ------- */
	/* lets move the system to a different state to show data corruption */
	state			= 3;
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "\n\nFailing journal super block on a \"write_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "write_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "write_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	/* we don't have to verify with the model anymore */
	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	
	return 1;
}

int fail_reiserfs_unordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 0;

	printf("Failing an unordered block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "Failing an unordered block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file_async", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file_async", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

/*
 * we create a file asynchronously and fail its unordered write 
 * to see what happens.
 */
int fail_unordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 1;

	printf("Failing an unordered block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing an unordered block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file_async", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file_async", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
} 

int fail_reiserfs_commit_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 0;

	printf("Failing the journal commit block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "Failing journal commit block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* we wait here */
	wait_for_response();
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

int fail_jfs_commit_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *dirname0		= "D0";
	char *dirname1		= "D1";
	int subfiles		= 0;
	//char *filename		= "file";
	//int filesize		= 12;
	int state			= 0;
	int inject			= 0;

	printf("Failing the jfs journal commit block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing journal commit block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	#if 0
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);
	#else
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_dir", NONFAULTY_MOUNT_PT, dirname0, subfiles);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_dir", FAULTY_MOUNT_PT, dirname0, subfiles);
	system(cmd);

	sprintf(cmd, "rm -rf %s/*;", NONFAULTY_MOUNT_PT);
	system(cmd);

	sprintf(cmd, "rm -rf %s/*;", FAULTY_MOUNT_PT);
	system(cmd);

	printf("before unmount ...\n");
	wait_for_response();

	sba_fault->blocknr = 61222;
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	//stop_verifying(dev_fd);

	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	wait_for_response();

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_dir", NONFAULTY_MOUNT_PT, dirname1, subfiles);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_dir", FAULTY_MOUNT_PT, dirname1, subfiles);
	system(cmd);

	wait_for_response();

	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	wait_for_response();

	#endif

	wait_for_response();

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

/*
 * the tests we run for journal commit block failure are exactly
 * the same as the tests we run for journal descriptor block failure.
 */

int fail_commit_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 1;

	printf("Failing the journal commit block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing journal commit block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	/* ------- running the second test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "\n\nFailing journal commit block on a \"create_file\" and crashing after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we wait for 7 seconds for the checkpointing to take place
	 * and then squash the writes - hopefully, the 7 seconds wait
	 * shud cause the checkpointing to happen
	 */
	sleep(CHECKPOINT_INTERVAL);
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

int revoke_ok(int filesystem)
{
	switch(filesystem) {
		case EXT3:
			switch(fs_jmode) {
				case ORDERED_JOURNALING:
				case WRITEBACK_JOURNALING:
					return 1;
				break;
			}
		break;
	}

	return 0;
}

/*
 * revoke blocks are written only during the following conditions
 * in ext3.
 *
 * if (data!=journal && (is_metadata || should_journal_data(inode))) {
 *    write revoke record
 * }
 *
 * so, first we check if the journaling mode and file system 
 * satisfies the above condition. then, we create a dir.
 *
 * we move the system to state 1, so that a revoke block will be
 * written to the journal instead of the journal block being empty
 */

int fail_revoke_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	int state			= 1;
	int inject			= 1;

	printf("Failing the journal revoke block ...\n");

	if (!revoke_ok(sba_fault->filesystem)) {
		fprintf(stderr, "Revoke block will not be written for this file system in this journaling mode\n");
		return -1;
	}

	/* ------- running the first test ------- */
	sba_fault->blocknr = 992;
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing journal revoke block on a \"create_dir\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));

	strcpy(cmd, "./test_suits/colony NULL create_file /mnt/sba/f1 52; rm -rf /mnt/sba/*; ./test_suits/colony NULL create_file /mnt/sba/f1 48; ./test_suits/colony NULL create_file /mnt/sba/f2 8;");
	//strcpy(cmd, "./test_suits/colony NULL create_file /mnt/sba/f1 52; rm -rf /mnt/sba/*; ./test_suits/colony NULL create_file /mnt/sba/f1 4; ./test_suits/colony NULL create_file /mnt/sba/f2 8;");
	system(cmd);

	wait_for_response();

	strcpy(cmd, "./test_suits/colony NULL create_file /mnt/nf_sba/f1 52; rm -rf /mnt/nf_sba/*; ./test_suits/colony NULL create_file /mnt/nf_sba/f1 48; ./test_suits/colony NULL create_file /mnt/nf_sba/f2 8;");
	system(cmd);

	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

/*
 * we don't inject the fault in prepare system. we inject it after 
 * that so that we skip a dummy transaction (one with the super block)
 * that is written during the prepare system.
 */
int fail_reiserfs_desc_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 0;

	printf("Failing the journal descriptor block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "\n\nFailing journal desc block on a \"create_file\" and crashing after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we wait for 7 seconds for the checkpointing to take place
	 * and then squash the writes - hopefully, the 7 seconds wait
	 * shud cause the checkpointing to happen
	 */
	sleep(30);
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}


/*
 * one or more desc blocks are written for every transaction.
 * so, we have to create some transaction and issue it to disk.
 *
 * we run two tests here:
 *
 * 1. in the first test we crash the file system before the 
 * checkpointing occurs.
 *
 * 2. in the second test, we crash after the checkpointing.
 *
 * before running each of these tests, we move the system to 
 * state 1, where some dir blocks are created during the previous
 * mount. this means that the journal desc block position 
 * would not be empty and it will have some old contents.
 */
int fail_desc_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 1;

	printf("Failing the journal descriptor block ...\n");

	/* ------- running the first test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing journal desc block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	/* ------- running the second test ------- */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "\n\nFailing journal desc block on a \"create_file\" and crashing after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we wait for 7 seconds for the checkpointing to take place
	 * and then squash the writes - hopefully, the 7 seconds wait
	 * shud cause the checkpointing to happen
	 */
	sleep(CHECKPOINT_INTERVAL);
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

int fail_reiserfs_data_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 0;

	printf("Failing the journal data block ...\n");

	/* ------- running the first test ------- */
	sba_fault->blocknr = -1; //FIXME

	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "Failing journal data block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}


/*
 * journal data blocks are those blocks that are journaled.
 * if failure of these blocks are not taken care properly, it
 * can cause serious file system errors.
 *
 * in the following test, we will fail the journaling of 
 * the journal data block that corresponds to group descriptor.
 * then we will crash the file system. when the file system
 * recovers, it overwrites the group descriptor with the 
 * old contents from the journal. this can lead to unmountable
 * file system.
 * FIXME: other cases
 */

int fail_data_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 1;

	printf("Failing the journal data block ...\n");

	/* ------- running the first test ------- */
	sba_fault->blocknr = 985; //FIXME

	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing journal data block on a \"create_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

int fail_reiserfs_checkpoint_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 0;

	printf("Failing the checkpoint block ...\n");

	/* ------- running the first test ------- */
	sba_fault->blocknr = 34816; 
	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "Failing checkpoint block on a \"create_file\" and remounting after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();
	/* remount the file systems */
	//squash_writes(nf_dev_fd);
	//squash_writes(dev_fd);

	stop_verifying(dev_fd);
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}


int fail_jfs_checkpoint_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	char *dirname		= "d0";
	int filesize		= 12;
	int subfiles		= 8;
	int state			= 1;
	int inject			= 1;

	printf("Failing the checkpoint block ...\n");

	/* ------- running the first test ------- */
	sba_fault->blocknr = 314; //FIXME: failing the dir data block
	prepare_system(dev_fd, sba_fault, state, inject);

	#if 0
	sprintf(err_msg, "Failing checkpoint block on a \"create_file\" and remounting after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);
	#else
	sprintf(err_msg, "Failing checkpoint block on a \"create_dir\" and remounting after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_dir", NONFAULTY_MOUNT_PT, dirname, subfiles);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d; sync;", test_suit_prefix, "create_dir", FAULTY_MOUNT_PT, dirname, subfiles);
	system(cmd);

	wait_for_response();

	sprintf(cmd, "%s %s %s/%s/d0 %d; sync;", test_suit_prefix, "create_dir", FAULTY_MOUNT_PT, dirname, subfiles);
	system(cmd);

	sprintf(cmd, "./run_chkpt.sh;");
	system(cmd);

	wait_for_response();

	sprintf(cmd, "./run_chkpt1.sh;");
	system(cmd);
	#endif

	//squash_writes(dev_fd);
	wait_for_response();

	/* remount the file systems */
	flush_from_cache(sba_fault->filesystem, CORRECT_FS);
	flush_from_cache(sba_fault->filesystem, FAULTY_FS);

	stop_verifying(dev_fd);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}



/*
 * failing checkpoint blocks can cause data corruption or data loss
 * or files/dirs loss. we show the extreme case of files and dirs loss.
 * FIXME: other cases
 */

int fail_checkpoint_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 1;
	int inject			= 1;

	printf("Failing the checkpoint block ...\n");

	/* ------- running the first test ------- */
	sba_fault->blocknr = 977; //FIXME: failing the dir data block
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "Failing checkpoint block on a \"create_file\" and remounting after checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "create_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* remount the file systems */
	flush_from_cache(sba_fault->filesystem, CORRECT_FS);
	flush_from_cache(sba_fault->filesystem, FAULTY_FS);

	stop_verifying(dev_fd);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

int fail_reiserfs_ordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 2;
	int inject			= 0;

	printf("Failing the ordered block ...\n");

	/* ------- running the first test ------- */
	/* lets move the system to state 2 to show data loss */
	prepare_system(dev_fd, sba_fault, state, inject);
	inject_fault(dev_fd, INJECT_FAULT, sba_fault);

	sprintf(err_msg, "\n\nFailing ordered data block on a \"append_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* remount the file systems */
	flush_from_cache(sba_fault->filesystem, CORRECT_FS);
	flush_from_cache(sba_fault->filesystem, FAULTY_FS);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}


int fail_jfs_ordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 2;
	int inject			= 1;

	printf("Failing the ordered block ...\n");

	/* ------- running the first test ------- */
	/* lets move the system to state 2 to show data loss */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "\n\nFailing ordered data block on a \"append_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	wait_for_response();
	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

/*
 * failing an ordered block can cause corrupted
 * data.
 */
int fail_ordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	char cmd[4096];
	char err_msg[4096];
	char *filename		= "file";
	int filesize		= 12;
	int state			= 2;
	int inject			= 1;

	printf("Failing the ordered block ...\n");

	/* ------- running the first test ------- */
	/* lets move the system to state 2 to show data loss */
	prepare_system(dev_fd, sba_fault, state, inject);

	sprintf(err_msg, "\n\nFailing ordered data block on a \"append_file\" and crashing before checkpointing ...");
	vp_log_error(err_msg, NO_ERROR, !(EXIT_ON_ERROR));
	
	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", NONFAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	sprintf(cmd, "%s %s %s/%s %d", test_suit_prefix, "append_file", FAULTY_MOUNT_PT, filename, filesize);
	system(cmd);

	/* 
	 * we immediately squash the writes - hopefully, 
	 * this shud happen before the checkpointing begins
	 */
	squash_writes(nf_dev_fd);
	squash_writes(dev_fd);

	stop_verifying(dev_fd);

	/* remount the file systems */
	mount_after_crash(nf_dev_fd, sba_fault->filesystem, CORRECT_FS);
	mount_after_crash(dev_fd, sba_fault->filesystem, FAULTY_FS);

	/* and compare their contents */
	compare_filesystems(FAULTY_MOUNT_PT, NONFAULTY_MOUNT_PT);
	
	wait_for_response();
	/* ----------------------------------------------- */

	return 1;
}

int issue_fail_fault(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	switch (sba_fault->blk_type) {
		case JOURNAL_DESC_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_desc_block(dev_fd, nf_dev_fd, sba_fault);
					//fail_revoke_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_desc_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;

		case JOURNAL_REVOKE_BLOCK:
			fail_revoke_block(dev_fd, nf_dev_fd, sba_fault);
		break;

		case JOURNAL_COMMIT_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_commit_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_commit_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case JFS:
					fail_jfs_commit_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;

		case JOURNAL_SUPER_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_super_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_super_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case JFS:
					fail_jfs_super_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;

		case JOURNAL_DATA_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_data_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_data_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;

		case CHECKPOINT_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_checkpoint_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_checkpoint_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case JFS:
					fail_jfs_checkpoint_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;

		case ORDERED_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_ordered_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_ordered_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case JFS:
					fail_jfs_ordered_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;

		case UNORDERED_BLOCK:
			switch(sba_fault->filesystem) {
				case EXT3:
					fail_unordered_block(dev_fd, nf_dev_fd, sba_fault);
				break;

				case REISERFS:
					fail_reiserfs_unordered_block(dev_fd, nf_dev_fd, sba_fault);
				break;
			}
		break;
	}
	
	return 1;
}

int issue_fault(int dev_fd, int nf_dev_fd, fault *sba_fault)
{
	if (sba_fault->fault_type == SBA_FAIL) {
		issue_fail_fault(dev_fd, nf_dev_fd, sba_fault);
		return 1;
	}

	return 1;
}

int set_jfs_params(char *btype, fault *sba_fault)
{
	/*block type*/
	if (strcmp(btype, "jourdata") == 0) {
		sba_fault->blk_type = JOURNAL_DATA_BLOCK;
	}
	else
	if (strcmp(btype, "joursuper") == 0) {
		sba_fault->blk_type = JOURNAL_SUPER_BLOCK;
	}
	else
	if (strcmp(btype, "jourcommit") == 0) {
		sba_fault->blk_type = JOURNAL_COMMIT_BLOCK;
	}
	else
	if (strcmp(btype, "checkpt") == 0) {
		sba_fault->blk_type = CHECKPOINT_BLOCK;
	}
	else
	if (strcmp(btype, "ordered") == 0) {
		sba_fault->blk_type = ORDERED_BLOCK;
	}
	else {
		fprintf(stderr, "Error: %s is not supported\n", btype);
		return -1;
	}

	return 1;
}

int set_ext3_reiserfs_params(char *btype, fault *sba_fault)
{
	/*block type*/
	if (strcmp(btype, "jourdesc") == 0) {
		sba_fault->blk_type = JOURNAL_DESC_BLOCK;
	}
	else
	if (strcmp(btype, "jourrevk") == 0) {
		sba_fault->blk_type = JOURNAL_REVOKE_BLOCK;
	}
	else
	if (strcmp(btype, "jourdata") == 0) {
		sba_fault->blk_type = JOURNAL_DATA_BLOCK;
	}
	else
	if (strcmp(btype, "joursuper") == 0) {
		sba_fault->blk_type = JOURNAL_SUPER_BLOCK;
	}
	else
	if (strcmp(btype, "jourcommit") == 0) {
		sba_fault->blk_type = JOURNAL_COMMIT_BLOCK;
	}
	else
	if (strcmp(btype, "checkpt") == 0) {
		sba_fault->blk_type = CHECKPOINT_BLOCK;
	}
	else
	if (strcmp(btype, "ordered") == 0) {
		sba_fault->blk_type = ORDERED_BLOCK;
	}
	else
	if (strcmp(btype, "unordered") == 0) {
		sba_fault->blk_type = UNORDERED_BLOCK;
	}
	else {
		fprintf(stderr, "Error: %s is not supported\n", btype);
		return -1;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	int fd;
	int nf_fd;
	char *filesystem;
	char *rw;
	char *ftype;
	char *fmode;
	char *btype;
	char *jmode;
	fault *sba_fault;

	if (argc != 7) {
		printf("Usage: inject_fault <filesystem> <rw> <ftype> <fmode> <btype> <jmode>\n");
		printf("\tfilesystem : ext3|reiser|jfs\n\trw         : read|write\n\tftype      : fail|corrupt\n");
		printf("\tfmode      : sticky|transient\n");
		printf("\tbtype      : jourdesc|jourrevk|jourdata|joursuper|jourcommit|checkpt|ordered|unordered\n");
		printf("\tjmode      : data|ordered|writeback\n");
		return -1;
	}

	switch(argc) {
		case 7:
			filesystem = argv[1];
			rw = argv[2];
			ftype = argv[3];
			fmode = argv[4];
			btype = argv[5];
			jmode = argv[6];
		break;

		default:
			fprintf(stderr, "Error: invalid arguments\n");
			return -1;
	}

	/*Initialize the system*/
	init_system();

	fd = open(FAULTY_DEV, O_RDONLY);
	nf_fd = open(NONFAULTY_DEV, O_RDONLY);

	sba_fault = (fault *)malloc(sizeof(fault));
	memset(sba_fault, 0, sizeof(fault));

	/* 
	 * Don't change this. SBA looks for block number 
	 * matches if the blocknr is not -1 
	 */
	sba_fault->blocknr = -1;

	/*When*/
	if (strcmp(rw, "read") == 0) {
		sba_fault->rw = READ;
	}
	else
	if (strcmp(rw, "write") == 0) {
		sba_fault->rw = WRITE;
	}
	else {
		fprintf(stderr, "Error: invalid rw\n");
		return -1;
	}

	/*fault type*/
	if (strcmp(ftype, "fail") == 0) {
		sba_fault->fault_type = SBA_FAIL;
	}
	else
	if (strcmp(ftype, "corrupt") == 0) {
		sba_fault->fault_type = SBA_CORRUPT;
		fprintf(stderr, "We don't handle corruption errors for now\n");
		return -1;
	}
	else {
		fprintf(stderr, "Error: invalid fault type\n");
		return -1;
	}

	/*fault mode*/
	if (strcmp(fmode, "sticky") == 0) {
		sba_fault->fault_mode = STICKY;
	}
	else
	if (strcmp(fmode, "transient") == 0) {
		sba_fault->fault_mode = TRANSIENT;
	}
	else {
		fprintf(stderr, "Error: invalid fault mode\n");
		return -1;
	}

	/*Filesystem*/
	if (strcmp(filesystem, "ext3") == 0) {
		sba_fault->filesystem = EXT3;
		set_ext3_reiserfs_params(btype, sba_fault);
	}
	else
	if (strcmp(filesystem, "reiser") == 0) {
		sba_fault->filesystem = REISERFS;
		set_ext3_reiserfs_params(btype, sba_fault);
	}
	else
	if (strcmp(filesystem, "jfs") == 0) {
		sba_fault->filesystem = JFS;
		set_jfs_params(btype, sba_fault);
	}
	else {
		fprintf(stderr, "Error: invalid filesystem\n");
		return -1;
	}

	if (strcmp(jmode, "data") == 0) {

		if (sba_fault->filesystem == JFS) {
			fprintf(stderr, "Error: invalid journaling mode\n");
			return -1;
		}

		fs_jmode = DATA_JOURNALING;
	}
	else 
	if (strcmp(jmode, "ordered") == 0) {
		fs_jmode = ORDERED_JOURNALING;
	}
	else 
	if (strcmp(jmode, "writeback") == 0) {

		if (sba_fault->filesystem == JFS) {
			fprintf(stderr, "Error: invalid journaling mode\n");
			return -1;
		}

		fs_jmode = WRITEBACK_JOURNALING;
	}
	else {
		fprintf(stderr, "Error: invalid journaling mode\n");
		return -1;
	}
	

	/*------------------------------------------------------------------------*/
	issue_fault(fd, nf_fd, sba_fault);
	
	/*------------------------------------------------------------------------*/
	cleanup_system();
	
	return 1;
}
