#include "coordinator.h"

/*
Pseudo code:

1. Define a set of functions that will be used for testing.

   function_set = {read, write, chmod, mount, unmount, ...)


2. Get from user the type of blocks to fail.

   block_types = {ext3 inode, ext3 dir, ext3 super block, ...)


3. Pass the information about the type of blocks to fail to
   the SBA driver.


4. Pick a function from the function_set and run it.


5. After the function ends, check with the SBA driver if it
   observed the block to fail and failed it. If so, return
   the result about file system behavior, SBA traces and system
   log file info to the user. If SBA driver did not observe
   the block to fail, run the next function from the function_set.


6. If none of the functions could create the traffic with the
   block to fail, then report to the user.
 */

/*this structure holds the fault specification*/
fault *sba_fault;

/*total number of workloads*/
int total_workloads = 15;

/*to issue ioctl calls to the device*/
int disk_fd = -1;

/*log file where all the results will go*/
char *logfile = "logfile";

/*testdir - where all the tests are conducted*/
char *roottest			= "/mnt/sba/roottest";
char *testdir			= "/mnt/sba/roottest/testdir";
char *longfilename		= "abcdefghijklmnopqrstuvwxyz01234567890abcdefghijklmnopqrstuvwxyz01234567890";
char *smallfile_pre		= "smallfile.pre";
char *smallfile_size	= "20"; /*KB*/
char *smalldir_pre		= "smalldir.pre";
char *smalldir_size		= "10";
char *mediumfile_pre	= "mediumfile.pre";
char *mediumfile_size	= "52";
char *symlink_dir		= "symlink_dir";
char *linkfile			= "linkfile";

char *revokefile1_new	= "revokefile1.new";
char *revokefile1_size1	= "52";
char *revokefile1_size2	= "48";
char *revokefile2_new	= "revokefile2.new";
char *revokefile2_size1	= "4";
char *recoverydir_pre	= "recoverydir.pre"; /*no of subfiles*/
char *recoverydir_size	= "100"; /*no of subfiles*/

/*Change this to be file system general*/
char *holefile_pre		= "holefile.pre";
char *hole1_pre			= "12";	/*ext3 specific - to create single indir block*/
char *hole1_new1		= "13";	/*ext3 specific - to write again single indir block*/
char *hole1_new2		= "14";	/*ext3 specific - to write again single indir block*/
char *hole2_pre			= "1036"; /*ext3 specific - to create double indir block = 1024 + 12*/
char *hole2_new1		= "2061"; /*ext3 specific - to write again double indir block = 1024 + 1024 + 12 + 1*/
char *hole2_new2		= "3085"; /*ext3 specific - to write again double indir block = 1024 + 2*1024 + 12 + 1*/
char *hole3_pre			= "2249612"; /*ext3 specific - to create triple indir block = 1024*1024 + 1024 + 12*/
char *hole3_new			= "1549613"; /*ext3 specific - to write again triple indir block = 1024*1024 + 1024 + 12 + 1*/

/*for single indir block read failures*/
char *sindir_file		= "sindirfile";
char *sindir_size		= "1024";

/*for double indir block read failures*/
char *dindir_file		= "dindirfile";
char *dindir_size		= "4400";

/*journaling file system parameters. TODO: move this 
 *parameters to input file*/
//int jcheckpoint			= 35;
int jcheckpoint			= 13;
int jcommit				= 1;
int jsize				= 32;
char *jmode				= "ordered";

int main(int argc, char *argv[])
{
	char *fault_spec;
	char *block_spec;
	
	if (argc != 3) {
		fprintf(stderr, "Usage: coordinator <fault-spec> <block-spec>\n");
		fprintf(stderr, "       where <fault-spec> is a file with fault specification\n");
		fprintf(stderr, "       where <block-spec> is a file with block types to fail\n");
		return -1;
	}

	init_sys();

	fault_spec = argv[1];
	block_spec = argv[2];

	get_fault_specification(fault_spec);

	test_system(block_spec);

	cleanup_sys();

	return 1;
}

int init_sys()
{
	sba_fault = (fault *)malloc(sizeof(fault));
	memset(sba_fault, 0, sizeof(fault));

	if ((disk_fd = open(DEVICE, O_RDONLY)) < 0) {
		fprintf(stderr, "Error: unable to open the dev %s\n", DEVICE);
		exit(-1);
	}

	vp_init_lib(logfile);

	return 1;
}

int cleanup_sys()
{
	free(sba_fault);
	close(disk_fd);

	return 1;
}

int wait_for_response()
{
	printf("when complete type \"ok\" to proceed\n");

	while (1) {
		char response[64];
		scanf("%s", response);
		if (strcmp("ok", response) == 0) {
			break;
		}
	}

	return 1;
}

char *get_block_type_str(int btype)
{
	switch(btype) {
		case SBA_EXT3_INODE:
			return "SBA_EXT3_INODE";
		break;

		case SBA_EXT3_DBITMAP:
			return "SBA_EXT3_DBITMAP";
		break;

		case SBA_EXT3_IBITMAP:
			return "SBA_EXT3_IBITMAP";
		break;

		case SBA_EXT3_SUPER:
			return "SBA_EXT3_SUPER";
		break;

		case SBA_EXT3_GROUP:
			return "SBA_EXT3_GROUP";
		break;

		case SBA_EXT3_DATA:
			return "SBA_EXT3_DATA";
		break;

		case SBA_EXT3_REVOKE:
			return "SBA_EXT3_REVOKE";
		break;

		case SBA_EXT3_DESC:
			return "SBA_EXT3_DESC";
		break;

		case SBA_EXT3_COMMIT:
			return "SBA_EXT3_COMMIT";
		break;

		case SBA_EXT3_JSUPER:
			return "SBA_EXT3_JSUPER";
		break;

		case SBA_EXT3_JDATA:
			return "SBA_EXT3_JDATA";
		break;

		case SBA_EXT3_DIR:
			return "SBA_EXT3_DIR";
		break;

		case SBA_EXT3_INDIR:
			return "SBA_EXT3_INDIR";
		break;

		case SBA_EXT3_SINDIR:
			return "SBA_EXT3_SINDIR";
		break;

		case SBA_EXT3_DINDIR:
			return "SBA_EXT3_DINDIR";
		break;

		case SBA_EXT3_TINDIR:
			return "SBA_EXT3_TINDIR";
		break;

		default:
			return "UNKNOWN";
	}
}

int print_fault(fault *fs_fault, char *fault_info)
{
	char *filesystem = "";
	char *rw = "";
	char *ftype = "";
	char *fmode = "";
	char *btype = "";

	printf("<=========Fault Type=========>\n");

	/*Filesystem*/
	switch(fs_fault->filesystem) {
		case EXT3:
			filesystem = "Ext3";
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
	switch(fs_fault->rw) {
		case SBA_READ:
			rw = "read";
		break;

		case SBA_WRITE:
			rw = "write";
		break;

		case SBA_READ_WRITE:
			rw = "read_write";
		break;

		default:
			rw = "unknown";
		break;
	}

	/*fault type*/
	switch(fs_fault->fault_type) {
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
	switch(fs_fault->fault_mode) {
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

	/*block type*/
	btype = get_block_type_str(fs_fault->blk_type);
	
	if (fault_info) {
		sprintf(fault_info, "FS: %s RW: %s TYPE: %s MODE: %s BLK: %s NR: %d\n", 
		filesystem, rw, ftype, fmode, btype, fs_fault->blocknr);
	}
	else {
		printf("FS: %s RW: %s TYPE: %s MODE: %s BLK: %s NR: %d\n", 
		filesystem, rw, ftype, fmode, btype, fs_fault->blocknr);
	}

	return 1;
}

int get_fault_specification(char *fault_spec)
{
	static FILE *fspec = NULL;

	fspec = fopen(fault_spec, "r");
	if (!fspec) {
		fprintf(stderr, "Error: unable to open the input file %s\n", fault_spec);
		exit(-1);
	}

	if (!feof(fspec)) {
		char input[64];

		fscanf(fspec, "rw: %s\n", input);
		if (strcmp(input, "read") == 0) {
			sba_fault->rw = SBA_READ;
		}
		else 
		if (strcmp(input, "write") == 0) {
			sba_fault->rw = SBA_WRITE;
		}
		else 
		if (strcmp(input, "rw") == 0) {
			sba_fault->rw = SBA_READ_WRITE;
		}
		else {
			goto fspec_err;
		}

		fscanf(fspec, "fault_type: %s\n", input);
		if (strcmp(input, "fail") == 0) {
			sba_fault->fault_type = SBA_FAIL;
		}
		else 
		if (strcmp(input, "corrupt") == 0) {
			sba_fault->fault_type = SBA_CORRUPT;
		}
		else {
			goto fspec_err;
		}

		fscanf(fspec, "fault_mode: %s\n", input);
		if (strcmp(input, "transient") == 0) {
			sba_fault->fault_mode = TRANSIENT;
		}
		else 
		if (strcmp(input, "sticky") == 0) {
			sba_fault->fault_mode = STICKY;
		}
		else {
			goto fspec_err;
		}

		fscanf(fspec, "filesystem: %s\n", input);
		if (strcmp(input, "ext3") == 0) {
			sba_fault->filesystem = EXT3;
		}
		else 
		if (strcmp(input, "reiserfs") == 0) {
			sba_fault->filesystem = REISERFS;
		}
		else 
		if (strcmp(input, "jfs") == 0) {
			sba_fault->filesystem = JFS;
		}
		else {
			goto fspec_err;
		}

		return 1;
	}

	fspec_err:
	print_fault(sba_fault, NULL);
	fprintf(stderr, "Error: invalid falut specification\n");

	return -1;
}

int get_next_block(char *input_file, fault *fs_fault)
{
	static FILE *file_ptr = NULL;

	if (!file_ptr) {
		/*this is the first time the file is opened*/
		if (input_file) {
			file_ptr = fopen(input_file, "r");
			if (!file_ptr) {
				fprintf(stderr, "Error: unable to open the input file %s\n", input_file);
				exit(-1);
			}
		}
		else {
			fprintf(stderr, "Error: invalid input file\n");
			exit(-1);
		}
	}
	
	if (file_ptr) {
		/*the file has been opened successfully*/

		if (!feof(file_ptr)) {
			char btype[64];

			fscanf(file_ptr, "%s \n", btype);
			fs_fault->blk_type = get_block_type(btype);

			switch(fs_fault->blk_type) {
				case SBA_EXT3_DBITMAP:
				case SBA_EXT3_IBITMAP:
				case SBA_EXT3_SUPER:
				case SBA_EXT3_GROUP:
				case SBA_EXT3_REVOKE:
				case SBA_EXT3_DESC:
				case SBA_EXT3_COMMIT:
				case SBA_EXT3_JSUPER:
				case SBA_EXT3_JDATA:
				case SBA_EXT3_DATA:
				case SBA_EXT3_DIR:
				case SBA_EXT3_SINDIR:	
				case SBA_EXT3_DINDIR:	
				case SBA_EXT3_TINDIR:	
				case SBA_EXT3_INODE:
					fscanf(file_ptr, "%d\n", &(fs_fault->blocknr));
				break;

				default:
					fprintf(stderr, "Error: Invalid block type\n");
					exit(-1);
			}

			switch(fs_fault->blk_type) {
				case SBA_EXT3_SINDIR:
				fs_fault->blk_type = SBA_EXT3_INDIR;
				fs_fault->spec.ext3.indir_blk = SBA_EXT3_SINDIR;
				break;

				case SBA_EXT3_DINDIR:
				fs_fault->blk_type = SBA_EXT3_INDIR;
				fs_fault->spec.ext3.indir_blk = SBA_EXT3_DINDIR;
				break;

				case SBA_EXT3_TINDIR:
				fs_fault->blk_type = SBA_EXT3_INDIR;
				fs_fault->spec.ext3.indir_blk = SBA_EXT3_TINDIR;
				break;
			}

			//printf("Read block type %s (%0x) with blocknr %d from input file\n", 
			//btype, fs_fault->blk_type, fs_fault->blocknr);
			return fs_fault->blk_type;
		}
	}

	return -1;

}

int get_block_type(char *btype)
{
	int block_type = UNKNOWN_BLOCK;

	/*for ext3*/
	if (strcmp(btype, "SBA_EXT3_INODE") == 0) {
		block_type = SBA_EXT3_INODE;
	}
	else
	if (strcmp(btype, "SBA_EXT3_DBITMAP") == 0) {
		block_type = SBA_EXT3_DBITMAP;
	}
	else
	if (strcmp(btype, "SBA_EXT3_IBITMAP") == 0) {
		block_type = SBA_EXT3_IBITMAP;
	}
	else
	if (strcmp(btype, "SBA_EXT3_SUPER") == 0) {
		block_type = SBA_EXT3_SUPER;
	}
	else
	if (strcmp(btype, "SBA_EXT3_GROUP") == 0) {
		block_type = SBA_EXT3_GROUP;
	}
	else
	if (strcmp(btype, "SBA_EXT3_DATA") == 0) {
		block_type = SBA_EXT3_DATA;
	}
	else
	if (strcmp(btype, "SBA_EXT3_REVOKE") == 0) {
		block_type = SBA_EXT3_REVOKE;
	}
	else
	if (strcmp(btype, "SBA_EXT3_DESC") == 0) {
		block_type = SBA_EXT3_DESC;
	}
	else
	if (strcmp(btype, "SBA_EXT3_COMMIT") == 0) {
		block_type = SBA_EXT3_COMMIT;
	}
	else
	if (strcmp(btype, "SBA_EXT3_JSUPER") == 0) {
		block_type = SBA_EXT3_JSUPER;
	}
	else
	if (strcmp(btype, "SBA_EXT3_JDATA") == 0) {
		block_type = SBA_EXT3_JDATA;
	}
	else
	if (strcmp(btype, "SBA_EXT3_DIR") == 0) {
		block_type = SBA_EXT3_DIR;
	}
	else
	if (strcmp(btype, "SBA_EXT3_SINDIR") == 0) {
		block_type = SBA_EXT3_SINDIR;
	}
	else
	if (strcmp(btype, "SBA_EXT3_DINDIR") == 0) {
		block_type = SBA_EXT3_DINDIR;
	}
	else
	if (strcmp(btype, "SBA_EXT3_TINDIR") == 0) {
		block_type = SBA_EXT3_TINDIR;
	}

	return block_type;
}

/*when running diff workloads some parameters like the inodenr
 *will change in the fault specification. so we need to reinit
 *fault sometimes*/
int reinit_fault()
{
	printf("Reinitializing the fault ...\n");
	ioctl(disk_fd, REINIT_FAULT, (void *)sba_fault);

	printf("Processing the fault ...\n");
	ioctl(disk_fd, PROCESS_FAULT);

	return 1;
}

int inject_fault()
{
	printf("Injecting the fault ...\n");
	ioctl(disk_fd, INJECT_FAULT, (void *)sba_fault);

	printf("Processing the fault ...\n");
	ioctl(disk_fd, PROCESS_FAULT);

	return 1;
}

int remove_fault()
{
	printf("Removing the fault ...\n");
	ioctl(disk_fd, REMOVE_FAULT);

	return 1;
}

int init_dir_blocks(unsigned long inodenr)
{
	printf("Initializing the dir blocks with inodenr %ld ...\n", inodenr);
	ioctl(disk_fd, INIT_DIR_BLKS, &inodenr);

	return 1;
}

int init_indir_blocks(unsigned long inodenr)
{
	printf("Initializing the indir blocks with inodenr %ld ...\n", inodenr);
	ioctl(disk_fd, INIT_INDIR_BLKS, &inodenr);

	return 1;
}

int fault_injected()
{
	int response = 0;
	
	ioctl(disk_fd, FAULT_INJECTED, &response);

	if (response) {
		printf("The fault has been injected successfully\n");
	}

	return response;
}

int journal_write_test()
{
	switch (sba_fault->blk_type) {
		case SBA_EXT3_JDATA:
		case SBA_EXT3_DESC:
		case SBA_EXT3_COMMIT:
		case SBA_EXT3_REVOKE:
		case SBA_EXT3_JSUPER:
			return 1;
		break;

		default:
			return 0;
	}

	return 0;
}

int wait_for_checkpoint()
{
	int epsilon = 1;

	if (journal_write_test()) {
		sleep(jcommit + epsilon + 1);
		return 1;
	}

	sleep(jcheckpoint + epsilon);
	return 1;
}

/* this function runs a set of workloads that can generate 
 * the revoke block traffic. the 'crash_commit' flag is used
 * insert crashe at certain point. such crash will force
 * recovery and we can study failures during recovery
 */
int revoke_workload(int crash_commit)
{
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

	char file1[256];
	char cmd[1024];
	int argc = 3;
	char *argv[3];

	printf("running the revoke workload ...\n");

	/*reinit the fault*/
	if (sba_fault->rw == SBA_READ) {
		switch(sba_fault->blk_type) {
			case SBA_EXT3_REVOKE:
				sba_fault->blocknr = 524;
			break;

			case SBA_EXT3_DESC:
				sba_fault->blocknr = 534;
			break;

			case SBA_EXT3_COMMIT:
				sba_fault->blocknr = 540;
			break;
		}
		reinit_fault();
	}

	/*first create a file, then delete it and recreate new files
	 *this workload creates revoke block in ext3*/

	/*create_file parameters: <file name> <size(KB)>*/
	sprintf(file1, "%s/%s", testdir, revokefile1_new);

	argv[0] = "create_file";
	argv[1] = file1;
	argv[2] = revokefile1_size1;
	
	printf("Running workload \"create_file\" to create %s ...\n", file1);
	create_file(argc, argv);

	/*now delete the file*/
	sprintf(cmd, "rm -rf %s", file1);
	system(cmd);

	/*then create two other files*/
	sprintf(file1, "%s/%s", testdir, revokefile1_new);

	argv[0] = "create_file";
	argv[1] = file1;
	argv[2] = revokefile1_size2;
	
	printf("Running workload \"create_file\" to create %s ...\n", file1);
	create_file(argc, argv);

	if (crash_commit) {
		/*insert a crash that would happen once the commit block
		 *from the next workload gets written*/
		notify_sba("crash_commit");
	}
	
	/*the second file*/
	sprintf(file1, "%s/%s", testdir, revokefile2_new);

	argv[0] = "create_file";
	argv[1] = file1;
	argv[2] = revokefile2_size1;
	
	printf("Running workload \"create_file\" to create %s ...\n", file1);
	create_file(argc, argv);

	return 1;
}

int notify_sba(char *cmd)
{
	char sys_cmd[64];

	sprintf(sys_cmd, "./tools/sba %s", cmd);
	system(sys_cmd);

	return 1;
}

int clear_all()
{
	/*stop testing*/
	notify_sba("dont_test");

	/*clean all the statistics*/
	notify_sba("clean_all_stats");
	notify_sba("zero_stat");

	/*remove the fault if there was any*/
	notify_sba("remove_fault");

	return 1;
}

int create_filesystem()
{
	char cmd[1024];

	switch(sba_fault->filesystem) {
		case EXT3:
			/* mke2fs -j -J size=32 -b 4096 /dev/SBA */
			sprintf(cmd, "mke2fs -j -J size=%d -b 4096 %s", jsize, DEVICE);
			system(cmd);
		break;

		default:
			fprintf(stderr, "Error: unsupported file system\n");
	}

	return 1;
}

int build_fs()
{
	create_filesystem();

	mount_filesystem();

	/*start the system*/
	notify_sba("start");

	return 1;
}

int create_test_state(int block_type)
{
	char cmd[256];

	sprintf(cmd, "mkdir %s", roottest);
	system(cmd);

	sprintf(cmd, "mkdir %s", testdir);
	system(cmd);

	return 1;
}

int unmount_filesystem()
{
	system("umount /mnt/sba");

	return 1;
}

int mount_filesystem()
{
	char cmd[4096];
	int ret;

	switch (sba_fault->filesystem) {
		case EXT3:
			sprintf(cmd, "mount -o commit=%d -o data=%s %s /mnt/sba -t ext3 >> %s", jcommit, jmode, DEVICE, logfile);
		break;

		case REISERFS:
			sprintf(cmd, "mount -o commit=%d -o data=%s %s /mnt/sba -t reiserfs >> %s", jcommit, jmode, DEVICE, logfile);
		break;

		case JFS:
			sprintf(cmd, "mount %s /mnt/sba -t jfs >> %s", DEVICE, logfile);
		break;

		default:
			fprintf(stderr, "Error: unknown file system\n");
			return -1;
	}

	ret = system(cmd);

	return ret;
}

int flush_from_cache()
{
	unmount_filesystem();
	mount_filesystem();

	return 1;
}

int create_setup(int block_type)
{
	/*stop any logging*/
	vp_dont_log();
	
	/*unmount the file system*/
	unmount_filesystem();

	/*delete all the old statistics and any data from the previous setup*/
	clear_all();

	/*stop interpreting the fs traffic*/
	notify_sba("stop");

	/*create the new file system*/
	build_fs();

	/*move the system to the required state for testing*/
	create_test_state(block_type);

	/*flush cache and remount the system freshly*/
	flush_from_cache();

	/*start testing the system*/
	notify_sba("test_system");
	
	/*start logging*/
	vp_start_log();
	
	return 1;
}

/* -------------------------test each posix call------------------------- */

workload *get_all_workloads(int block_type)
{
	workload *wkld = NULL;

	wkld = (workload *)malloc(sizeof(wkld));
	wkld->id = (int *)malloc(MAX_WORKLOAD*sizeof(int));
	wkld->minor_blktype = (int *)malloc(MAX_WORKLOAD*sizeof(int));
	
	wkld->blk_type = block_type;
	printf("block type = %s\n", get_block_type_str(block_type));

	switch(block_type) {
		case SBA_EXT3_INODE:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 31;
				wkld->id[0] = POS_ACCESS; wkld->minor_blktype[0] = FILE_INODE;
				wkld->id[1] = POS_ACCESS; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_ACCESS; wkld->minor_blktype[2] = SYMLINK_INODE;
				wkld->id[3] = POS_CHDIR; wkld->minor_blktype[3] = DIR_INODE;
				wkld->id[4] = POS_CHMOD; wkld->minor_blktype[4] = FILE_INODE;
				wkld->id[5] = POS_CHMOD; wkld->minor_blktype[5] = DIR_INODE;
				wkld->id[6] = POS_CHMOD; wkld->minor_blktype[6] = SYMLINK_INODE;
				wkld->id[7] = POS_CHOWN; wkld->minor_blktype[7] = FILE_INODE;
				wkld->id[8] = POS_CHOWN; wkld->minor_blktype[8] = DIR_INODE;
				wkld->id[9] = POS_CHOWN; wkld->minor_blktype[9] = SYMLINK_INODE;
				wkld->id[10] = POS_CHROOT; wkld->minor_blktype[10] = DIR_INODE;
				wkld->id[11] = POS_CREAT; wkld->minor_blktype[11] = FILE_INODE;
				wkld->id[12] = POS_STAT; wkld->minor_blktype[12] = FILE_INODE;
				wkld->id[13] = POS_STAT; wkld->minor_blktype[13] = DIR_INODE;
				wkld->id[14] = POS_STATFS; wkld->minor_blktype[14] = DIR_INODE;
				wkld->id[15] = POS_TRUNCATE; wkld->minor_blktype[15] = FILE_INODE;
				wkld->id[16] = POS_LINK; wkld->minor_blktype[16] = FILE_INODE;
				wkld->id[17] = POS_LSTAT; wkld->minor_blktype[17] = SYMLINK_INODE;
				wkld->id[18] = POS_MKDIR; wkld->minor_blktype[18] = DIR_INODE;
				wkld->id[19] = POS_MOUNT; wkld->minor_blktype[19] = DIR_INODE;
				wkld->id[20] = POS_CREATOPEN; wkld->minor_blktype[20] = FILE_INODE;
				wkld->id[21] = POS_OPEN; wkld->minor_blktype[21] = FILE_INODE;
				wkld->id[22] = POS_READLINK; wkld->minor_blktype[22] = SYMLINK_INODE;
				wkld->id[23] = POS_RENAME; wkld->minor_blktype[23] = DIR_INODE;
				wkld->id[24] = POS_RMDIR; wkld->minor_blktype[24] = DIR_INODE;
				wkld->id[25] = POS_SYMLINK; wkld->minor_blktype[25] = SYMLINK_INODE;
				wkld->id[26] = POS_UNLINK; wkld->minor_blktype[26] = FILE_INODE;
				wkld->id[27] = POS_UNLINK; wkld->minor_blktype[27] = SYMLINK_INODE;
				wkld->id[28] = POS_UTIMES; wkld->minor_blktype[28] = FILE_INODE;
				wkld->id[29] = POS_UTIMES; wkld->minor_blktype[29] = DIR_INODE;
				wkld->id[30] = POS_UTIMES; wkld->minor_blktype[30] = SYMLINK_INODE;
			}
			else {
				wkld->total = 25;
				wkld->id[0] = POS_CHMOD; wkld->minor_blktype[0] = FILE_INODE;
				wkld->id[1] = POS_CHMOD; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_CHMOD; wkld->minor_blktype[2] = SYMLINK_INODE;
				wkld->id[3] = POS_CHOWN; wkld->minor_blktype[3] = FILE_INODE;
				wkld->id[4] = POS_CHOWN; wkld->minor_blktype[4] = DIR_INODE;
				wkld->id[5] = POS_CHOWN; wkld->minor_blktype[5] = SYMLINK_INODE;
				wkld->id[6] = POS_CREAT; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_TRUNCATE; wkld->minor_blktype[7] = FILE_INODE;
				wkld->id[8] = POS_LINK; wkld->minor_blktype[8] = FILE_INODE;
				wkld->id[9] = POS_MKDIR; wkld->minor_blktype[9] = DIR_INODE;
				wkld->id[10] = POS_CREATOPEN; wkld->minor_blktype[10] = FILE_INODE;
				wkld->id[11] = POS_RENAME; wkld->minor_blktype[11] = DIR_INODE;
				wkld->id[12] = POS_RMDIR; wkld->minor_blktype[12] = DIR_INODE;
				wkld->id[13] = POS_SYMLINK; wkld->minor_blktype[13] = SYMLINK_INODE;
				wkld->id[14] = POS_SYNC; wkld->minor_blktype[14] = FILE_INODE;
				wkld->id[15] = POS_UNLINK; wkld->minor_blktype[15] = FILE_INODE;
				wkld->id[16] = POS_UNLINK; wkld->minor_blktype[16] = SYMLINK_INODE;
				wkld->id[17] = POS_UMOUNT; wkld->minor_blktype[17] = FILE_INODE;
				wkld->id[18] = POS_UTIMES; wkld->minor_blktype[18] = FILE_INODE;
				wkld->id[19] = POS_UTIMES; wkld->minor_blktype[19] = DIR_INODE;
				wkld->id[20] = POS_UTIMES; wkld->minor_blktype[20] = SYMLINK_INODE;
				wkld->id[21] = POS_WRITE; wkld->minor_blktype[21] = FILE_INODE;
				wkld->id[22] = POS_FSYNC; wkld->minor_blktype[22] = FILE_INODE;
				wkld->id[23] = POS_READ; wkld->minor_blktype[23] = FILE_INODE;
				wkld->id[24] = DIRCRASH_WORKLOAD; wkld->minor_blktype[24] = 0;
				//wkld->id[23] = POS_GETDIRENT; wkld->minor_blktype[23] = FILE_INODE;
			}
		break;

		case SBA_EXT3_DBITMAP:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 6;
				wkld->id[0] = POS_TRUNCATE; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_MKDIR; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_RMDIR; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_SYMLINK; wkld->minor_blktype[3] = SYMLINK_INODE;
				wkld->id[4] = POS_UNLINK; wkld->minor_blktype[4] = FILE_INODE;
				wkld->id[5] = POS_WRITE; wkld->minor_blktype[5] = FILE_INODE;
			}
			else {
				wkld->total = 10;
				wkld->id[0] = POS_TRUNCATE; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_MKDIR; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_RMDIR; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_SYMLINK; wkld->minor_blktype[3] = SYMLINK_INODE;
				wkld->id[4] = POS_SYNC; wkld->minor_blktype[4] = FILE_INODE;
				wkld->id[5] = POS_UNLINK; wkld->minor_blktype[5] = FILE_INODE;
				wkld->id[6] = POS_UMOUNT; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_WRITE; wkld->minor_blktype[7] = FILE_INODE;
				wkld->id[8] = POS_FSYNC; wkld->minor_blktype[8] = FILE_INODE;
				wkld->id[9] = DIRCRASH_WORKLOAD; wkld->minor_blktype[9] = 0;
			}
		break;

		case SBA_EXT3_IBITMAP:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 6;
				wkld->id[0] = POS_CREAT; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_MKDIR; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_CREATOPEN; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_RMDIR; wkld->minor_blktype[3] = DIR_INODE;
				wkld->id[4] = POS_SYMLINK; wkld->minor_blktype[4] = SYMLINK_INODE;
				wkld->id[5] = POS_UNLINK; wkld->minor_blktype[5] = FILE_INODE;
			}
			else {
				wkld->total = 10;
				wkld->id[0] = POS_CREAT; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_MKDIR; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_CREATOPEN; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_RMDIR; wkld->minor_blktype[3] = DIR_INODE;
				wkld->id[4] = POS_SYMLINK; wkld->minor_blktype[4] = SYMLINK_INODE;
				wkld->id[5] = POS_SYNC; wkld->minor_blktype[5] = FILE_INODE;
				wkld->id[6] = POS_UNLINK; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_UMOUNT; wkld->minor_blktype[7] = FILE_INODE;
				wkld->id[8] = POS_FSYNC; wkld->minor_blktype[0] = 0;
				wkld->id[9] = DIRCRASH_WORKLOAD; wkld->minor_blktype[9] = 0;
			}
		break;

		case SBA_EXT3_SUPER:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 1;
				wkld->id[0] = POS_MOUNT; wkld->minor_blktype[0] = DIR_INODE;
			}
			else {
				wkld->total = 3;
				wkld->id[0] = POS_MOUNT; wkld->minor_blktype[0] = DIR_INODE;
				wkld->id[1] = POS_UNLINK; wkld->minor_blktype[1] = FILE_INODE;
				wkld->id[2] = POS_UMOUNT; wkld->minor_blktype[2] = FILE_INODE;
			}
		break;

		case SBA_EXT3_GROUP:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 1;
				wkld->id[0] = POS_MOUNT; wkld->minor_blktype[0] = DIR_INODE;
			}
			else {
				wkld->total = 12;
				wkld->id[0] = POS_CREAT; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_MKDIR; wkld->minor_blktype[1] = DIR_INODE;
				wkld->id[2] = POS_CREATOPEN; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_RMDIR; wkld->minor_blktype[3] = DIR_INODE;
				wkld->id[4] = POS_SYMLINK; wkld->minor_blktype[4] = SYMLINK_INODE;
				wkld->id[5] = POS_SYNC; wkld->minor_blktype[5] = FILE_INODE;
				wkld->id[6] = POS_UNLINK; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_UMOUNT; wkld->minor_blktype[7] = FILE_INODE;
				wkld->id[8] = POS_WRITE; wkld->minor_blktype[8] = FILE_INODE;
				wkld->id[9] = POS_FSYNC; wkld->minor_blktype[9] = FILE_INODE;
				wkld->id[10] = POS_TRUNCATE; wkld->minor_blktype[10] = FILE_INODE;
				wkld->id[11] = FILECRASH_WORKLOAD; wkld->minor_blktype[11] = FILE_INODE;
			}
		break;

		case SBA_EXT3_DATA:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 2;
				wkld->id[0] = POS_READ; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_READLINK; wkld->minor_blktype[1] = SYMLINK_INODE;
			}
			else {
				wkld->total = 5;
				wkld->id[0] = POS_SYMLINK; wkld->minor_blktype[0] = SYMLINK_INODE;
				wkld->id[1] = POS_SYNC; wkld->minor_blktype[1] = FILE_INODE;
				wkld->id[2] = POS_UMOUNT; wkld->minor_blktype[2] = FILE_INODE;
				wkld->id[3] = POS_WRITE; wkld->minor_blktype[3] = FILE_INODE;
				wkld->id[4] = POS_FSYNC; wkld->minor_blktype[4] = FILE_INODE;
			}
		break;

		case SBA_EXT3_REVOKE:
			if (sba_fault->rw == SBA_READ) {
				/*recovery as a workload*/
				wkld->total = 1;
				wkld->id[0] = REVOKE_WORKLOAD; wkld->minor_blktype[0] = 0;
			}
			else {
				wkld->total = 4;
				wkld->id[0] = POS_FSYNC; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_TRUNCATE; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_RMDIR; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_UNLINK; wkld->minor_blktype[3] = FILE_INODE;
			}
		break;

		case SBA_EXT3_DESC:
			if (sba_fault->rw == SBA_READ) {
				/*recovery as a workload*/
				wkld->total = 1;
				wkld->id[0] = REVOKE_WORKLOAD; wkld->minor_blktype[0] = 0;
			}
			else {
				wkld->total = 19;
				wkld->id[0] = POS_CHMOD; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_CHOWN; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_CREAT; wkld->minor_blktype[2] = 0;
				wkld->id[3] = POS_FSYNC; wkld->minor_blktype[3] = 0;
				wkld->id[4] = POS_TRUNCATE; wkld->minor_blktype[4] = 0;
				wkld->id[5] = POS_GETDIRENT; wkld->minor_blktype[5] = DIR_INODE;
				wkld->id[6] = POS_LINK; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_MKDIR; wkld->minor_blktype[7] = DIR_INODE;
				wkld->id[8] = POS_CREATOPEN; wkld->minor_blktype[8] = DIR_INODE;
				wkld->id[9] = POS_READ; wkld->minor_blktype[9] = FILE_INODE;
				wkld->id[10] = POS_READLINK; wkld->minor_blktype[10] = SYMLINK_INODE;
				wkld->id[11] = POS_RENAME; wkld->minor_blktype[11] = DIR_INODE;
				wkld->id[12] = POS_RMDIR; wkld->minor_blktype[12] = DIR_INODE;
				wkld->id[13] = POS_SYMLINK; wkld->minor_blktype[13] = SYMLINK_INODE;
				wkld->id[14] = POS_SYNC; wkld->minor_blktype[14] = FILE_INODE;
				wkld->id[15] = POS_UNLINK; wkld->minor_blktype[15] = FILE_INODE;
				wkld->id[16] = POS_UMOUNT; wkld->minor_blktype[16] = FILE_INODE;
				wkld->id[17] = POS_UTIMES; wkld->minor_blktype[17] = FILE_INODE;
				wkld->id[18] = POS_WRITE; wkld->minor_blktype[18] = FILE_INODE;
				//wkld->id[19] = DIRCRASH_WORKLOAD; wkld->minor_blktype[19] = 0;
			}
		break;

		case SBA_EXT3_COMMIT:
			if (sba_fault->rw == SBA_READ) {
				/*recovery as a workload*/
				wkld->total = 1;
				wkld->id[0] = REVOKE_WORKLOAD; wkld->minor_blktype[0] = 0;
			}
			else {
				wkld->total = 19;
				wkld->id[0] = POS_CHMOD; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_CHOWN; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_CREAT; wkld->minor_blktype[2] = 0;
				wkld->id[3] = POS_FSYNC; wkld->minor_blktype[3] = 0;
				wkld->id[4] = POS_TRUNCATE; wkld->minor_blktype[4] = 0;
				wkld->id[5] = POS_GETDIRENT; wkld->minor_blktype[5] = DIR_INODE;
				wkld->id[6] = POS_LINK; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_MKDIR; wkld->minor_blktype[7] = DIR_INODE;
				wkld->id[8] = POS_CREATOPEN; wkld->minor_blktype[8] = DIR_INODE;
				wkld->id[9] = POS_READ; wkld->minor_blktype[9] = FILE_INODE;
				wkld->id[10] = POS_READLINK; wkld->minor_blktype[10] = SYMLINK_INODE;
				wkld->id[11] = POS_RENAME; wkld->minor_blktype[11] = DIR_INODE;
				wkld->id[12] = POS_RMDIR; wkld->minor_blktype[12] = DIR_INODE;
				wkld->id[13] = POS_SYMLINK; wkld->minor_blktype[13] = SYMLINK_INODE;
				wkld->id[14] = POS_SYNC; wkld->minor_blktype[14] = FILE_INODE;
				wkld->id[15] = POS_UNLINK; wkld->minor_blktype[15] = FILE_INODE;
				wkld->id[16] = POS_UMOUNT; wkld->minor_blktype[16] = FILE_INODE;
				wkld->id[17] = POS_UTIMES; wkld->minor_blktype[17] = FILE_INODE;
				wkld->id[18] = POS_WRITE; wkld->minor_blktype[18] = FILE_INODE;
			}
		break;

		case SBA_EXT3_JSUPER:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 1;
				wkld->id[0] = POS_MOUNT; wkld->minor_blktype[0] = DIR_INODE;
			}
			else {
				wkld->total = 20;
				wkld->id[0] = POS_CHMOD; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_CHOWN; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_CREAT; wkld->minor_blktype[2] = 0;
				wkld->id[3] = POS_FSYNC; wkld->minor_blktype[3] = 0;
				wkld->id[4] = POS_TRUNCATE; wkld->minor_blktype[4] = 0;
				wkld->id[5] = POS_GETDIRENT; wkld->minor_blktype[5] = DIR_INODE;
				wkld->id[6] = POS_LINK; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_MKDIR; wkld->minor_blktype[7] = DIR_INODE;
				wkld->id[8] = POS_CREATOPEN; wkld->minor_blktype[8] = DIR_INODE;
				wkld->id[9] = POS_READ; wkld->minor_blktype[9] = FILE_INODE;
				wkld->id[10] = POS_READLINK; wkld->minor_blktype[10] = SYMLINK_INODE;
				wkld->id[11] = POS_RENAME; wkld->minor_blktype[11] = DIR_INODE;
				wkld->id[12] = POS_RMDIR; wkld->minor_blktype[12] = DIR_INODE;
				wkld->id[13] = POS_SYMLINK; wkld->minor_blktype[13] = SYMLINK_INODE;
				wkld->id[14] = POS_SYNC; wkld->minor_blktype[14] = FILE_INODE;
				wkld->id[15] = POS_UNLINK; wkld->minor_blktype[15] = FILE_INODE;
				wkld->id[16] = POS_UMOUNT; wkld->minor_blktype[16] = FILE_INODE;
				wkld->id[17] = POS_UTIMES; wkld->minor_blktype[17] = FILE_INODE;
				wkld->id[18] = POS_WRITE; wkld->minor_blktype[18] = FILE_INODE;
				wkld->id[19] = DIRCRASH_WORKLOAD; wkld->minor_blktype[19] = 0;
			}
		break;

		case SBA_EXT3_JDATA:
			if (sba_fault->rw == SBA_READ) {
				/*recovery as a workload*/
				wkld->total = 1;
				wkld->id[0] = DIRCRASH_WORKLOAD; wkld->minor_blktype[0] = 0;
			}
			else {
				wkld->total = 19;
				wkld->id[0] = POS_CHMOD; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_CHOWN; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_CREAT; wkld->minor_blktype[2] = 0;
				wkld->id[3] = POS_FSYNC; wkld->minor_blktype[3] = 0;
				wkld->id[4] = POS_TRUNCATE; wkld->minor_blktype[4] = 0;
				wkld->id[5] = POS_GETDIRENT; wkld->minor_blktype[5] = DIR_INODE;
				wkld->id[6] = POS_LINK; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_MKDIR; wkld->minor_blktype[7] = DIR_INODE;
				wkld->id[8] = POS_CREATOPEN; wkld->minor_blktype[8] = DIR_INODE;
				wkld->id[9] = POS_READ; wkld->minor_blktype[9] = FILE_INODE;
				wkld->id[10] = POS_READLINK; wkld->minor_blktype[10] = SYMLINK_INODE;
				wkld->id[11] = POS_RENAME; wkld->minor_blktype[11] = DIR_INODE;
				wkld->id[12] = POS_RMDIR; wkld->minor_blktype[12] = DIR_INODE;
				wkld->id[13] = POS_SYMLINK; wkld->minor_blktype[13] = SYMLINK_INODE;
				wkld->id[14] = POS_SYNC; wkld->minor_blktype[14] = FILE_INODE;
				wkld->id[15] = POS_UNLINK; wkld->minor_blktype[15] = FILE_INODE;
				wkld->id[16] = POS_UMOUNT; wkld->minor_blktype[16] = FILE_INODE;
				wkld->id[17] = POS_UTIMES; wkld->minor_blktype[17] = FILE_INODE;
				wkld->id[18] = POS_WRITE; wkld->minor_blktype[18] = FILE_INODE;
			}
		break;

		case SBA_EXT3_DIR:
			if (sba_fault->rw == SBA_READ) {
				wkld->total = 21;
				wkld->id[0] = POS_ACCESS; wkld->minor_blktype[0] = 0;
				wkld->id[1] = POS_CHDIR; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_CHMOD; wkld->minor_blktype[2] = 0;
				wkld->id[3] = POS_CHROOT; wkld->minor_blktype[3] = 0;
				wkld->id[4] = POS_CREAT; wkld->minor_blktype[4] = 0;
				wkld->id[5] = POS_STAT; wkld->minor_blktype[5] = 0;
				wkld->id[6] = POS_STATFS; wkld->minor_blktype[6] = 0;
				wkld->id[7] = POS_TRUNCATE; wkld->minor_blktype[7] = 0;
				wkld->id[8] = POS_GETDIRENT; wkld->minor_blktype[8] = DIR_INODE;
				wkld->id[9] = POS_LINK; wkld->minor_blktype[9] = FILE_INODE;
				wkld->id[10] = POS_LSTAT; wkld->minor_blktype[10] = SYMLINK_INODE;
				wkld->id[11] = POS_MKDIR; wkld->minor_blktype[11] = DIR_INODE;
				wkld->id[12] = POS_CREATOPEN; wkld->minor_blktype[12] = DIR_INODE;
				wkld->id[13] = POS_OPEN; wkld->minor_blktype[13] = FILE_INODE;
				wkld->id[14] = POS_READLINK; wkld->minor_blktype[14] = SYMLINK_INODE;
				wkld->id[15] = POS_RENAME; wkld->minor_blktype[15] = DIR_INODE;
				wkld->id[16] = POS_RMDIR; wkld->minor_blktype[16] = DIR_INODE;
				wkld->id[17] = POS_SYMLINK; wkld->minor_blktype[17] = SYMLINK_INODE;
				wkld->id[18] = POS_UNLINK; wkld->minor_blktype[18] = FILE_INODE;
				wkld->id[19] = POS_UTIMES; wkld->minor_blktype[19] = FILE_INODE;
				wkld->id[20] = POS_CHOWN; wkld->minor_blktype[20] = DIR_INODE;
			}
			else {
				wkld->total = 12;
				wkld->id[0] = POS_LINK; wkld->minor_blktype[0] = FILE_INODE;
				wkld->id[1] = POS_CREAT; wkld->minor_blktype[1] = 0;
				wkld->id[2] = POS_MKDIR; wkld->minor_blktype[2] = DIR_INODE;
				wkld->id[3] = POS_CREATOPEN; wkld->minor_blktype[3] = DIR_INODE;
				wkld->id[4] = POS_RENAME; wkld->minor_blktype[4] = DIR_INODE;
				wkld->id[5] = POS_SYMLINK; wkld->minor_blktype[5] = SYMLINK_INODE;
				wkld->id[6] = POS_SYNC; wkld->minor_blktype[6] = FILE_INODE;
				wkld->id[7] = POS_UNLINK; wkld->minor_blktype[7] = FILE_INODE;
				wkld->id[8] = POS_UMOUNT; wkld->minor_blktype[8] = FILE_INODE;
				wkld->id[9] = POS_FSYNC; wkld->minor_blktype[9] = DIR_INODE;
				wkld->id[10] = POS_RMDIR; wkld->minor_blktype[10] = DIR_INODE;
				wkld->id[11] = DIRCRASH_WORKLOAD; wkld->minor_blktype[11] = DIR_INODE;
			}
		break;

		case SBA_EXT3_INDIR:
		{
			switch(sba_fault->spec.ext3.indir_blk) {
				case SBA_EXT3_SINDIR:
					if (sba_fault->rw == SBA_READ) {
						wkld->total = 3;
						wkld->id[0] = POS_TRUNCATE; wkld->minor_blktype[0] = 0;
						wkld->id[1] = POS_UNLINK; wkld->minor_blktype[1] = FILE_INODE;
						wkld->id[2] = POS_WRITE; wkld->minor_blktype[2] = FILE_INODE;
					}
					else {
						wkld->total = 5;
						wkld->id[0] = POS_FSYNC; wkld->minor_blktype[0] = FILE_INODE;
						wkld->id[1] = POS_SYNC; wkld->minor_blktype[1] = FILE_INODE;
						wkld->id[2] = POS_WRITE; wkld->minor_blktype[2] = FILE_INODE;
						wkld->id[3] = POS_UMOUNT; wkld->minor_blktype[3] = FILE_INODE;
						wkld->id[4] = FILECRASH_WORKLOAD; wkld->minor_blktype[4] = FILE_INODE;
					}
				break;

				case SBA_EXT3_DINDIR:
					if (sba_fault->rw == SBA_READ) {
						wkld->total = 1;
						wkld->id[0] = DINDIR_WORKLOAD; wkld->minor_blktype[0] = 0;
					}
					else {
						wkld->total = 1;
						wkld->id[0] = FILECRASH_WORKLOAD; wkld->minor_blktype[0] = FILE_INODE;
					}
				break;

				case SBA_EXT3_TINDIR:
				break;
			}
		}
		break;

		default:
			free(wkld);
			wkld = NULL;
	}

	return wkld;
}

int construct_fault(char *path, int blknr, int minor_blktype)
{
	struct stat stat_buf;
	char fault_info[1024];

	if (path) {
		/*ask the SBA to initialize itself with the blocks of this file/directory*/
		if (minor_blktype == SYMLINK_INODE) {
			if (lstat(path, &stat_buf) != 0) {
				perror("stat");
				return -1;
			}
		}
		else {
			if (stat(path, &stat_buf) != 0) {
				perror("stat");
				return -1;
			}
		}
		printf("The inode number of %s is %ld\n", path, stat_buf.st_ino);
		sba_fault->spec.ext3.inodenr = stat_buf.st_ino;
	}
	else {
		sba_fault->spec.ext3.inodenr = -1;
	}

	sba_fault->spec.ext3.logical_blocknr = blknr;

	/*construct a fault specification with this block type*/
	print_fault(sba_fault, fault_info);
	printf("The current fault specification is: %s", fault_info);
	vp_log_error(fault_info, NO_ERROR, !(EXIT_ON_ERROR));

	return 1;
}

int coord_create_symlink(char *old, char *new)
{
	pos_symlink(old, new);

	return 1;
}

int coord_create_dir(char *name, char *size)
{
	/* create_dir parameters: <dir name> <no of subfiles> */
	int argc = 3;
	char *argv[3];

	argv[0] = "create_dir";
	argv[1] = name;
	argv[2] = size;

	create_dir(argc, argv);

	return 1;
}

int coord_append_file(char *name, char *size)
{
	/* append_file parameters: <file name> <size(KB)> */
	int argc = 3;
	char *argv[3];

	argv[0] = "append_file";
	argv[1] = name;
	argv[2] = size;

	append_file(argc, argv);

	return 1;
}

int coord_create_file(char *name, char *size)
{
	/* create_file parameters: <file name> <size(KB)> */
	int argc = 3;
	char *argv[3];

	argv[0] = "create_file";
	argv[1] = name;
	argv[2] = size;

	create_file(argc, argv);

	return 1;
}

int dircrash_workload()
{
	char file1[1024];
	int argc = 3;
	char *argv[3];

	/*reinit the fault*/
	switch(sba_fault->blk_type) {
		case SBA_EXT3_DESC:
			sba_fault->blocknr = 534;
		break;

		case SBA_EXT3_JDATA:
			sba_fault->blocknr = 519;
		break;

		case SBA_EXT3_COMMIT:
			sba_fault->blocknr = 539;
		break;
	}
	
	/*insert a crash that would happen once the commit block
	 *from the next workload gets written*/
	notify_sba("crash_commit");
	
	/*first run some workload - we create a dir with a bunch of files*/
	/* create_dir parameters: <dir name> <no of subfiles> */
	sprintf(file1, "%s/%s", testdir, recoverydir_pre);

	argv[0] = "create_dir";
	argv[1] = file1;
	argv[2] = recoverydir_size;
	
	create_dir(argc, argv);

	/*we have to build the fault before the file system in unmounted*/
	if (sba_fault->blk_type == SBA_EXT3_DIR) {
		construct_fault(testdir, -1, 0);
	}

	/*unmount*/
	unmount_filesystem();

	/*now inject the fault*/
	inject_fault();

	/*clear the crash failures*/
	notify_sba("dont_crash_commit");

	/*and remount to initiate recovery*/
	mount_filesystem();
	wait_for_response();

	return 1;
}

/*for failing single and double indir blks write during recovery*/
int filecrash_workload()
{
	char file1[1024];
	int argc = 3;
	char *argv[3];

	/*first run some workload - we create a large files*/
	/* create_file parameters: <file name> <size> */
	if (sba_fault->spec.ext3.indir_blk == SBA_EXT3_DINDIR) {
		sprintf(file1, "%s/%s", testdir, dindir_file);

		argv[0] = "create_file";
		argv[1] = file1;
		argv[2] = dindir_size;
	}
	else {
		sprintf(file1, "%s/%s", testdir, sindir_file);

		argv[0] = "create_file";
		argv[1] = file1;
		argv[2] = "52";
	}

	create_file(argc, argv);
	flush_from_cache();

	/*create the correct fault specification*/
	construct_fault(file1, -1, 0);

	/*insert a crash that would happen once the commit block
	 *from the next workload gets written*/
	notify_sba("crash_commit");
	
	/*now run some workload that would update the indir blks*/
	argv[0] = "write_file";
	argv[1] = file1;
	if (sba_fault->spec.ext3.indir_blk == SBA_EXT3_DINDIR) {
		argv[2] = "2500"; /*blk# to write to*/
	}
	else {
		argv[2] = "15"; /*blk# to write to*/
	}
	write_file(argc, argv);

	/*unmount*/
	unmount_filesystem();

	/*now inject the fault*/
	inject_fault();

	/*clear the crash failures*/
	notify_sba("dont_crash_commit");

	/*and remount to initiate recovery*/
	mount_filesystem();
	wait_for_response();

	return 1;
}


int recovery_workload()
{
	int crash_commit = 1;
	revoke_workload(crash_commit);

	/*unmount*/
	unmount_filesystem();

	/*clear the crash failures*/
	notify_sba("dont_crash_commit");

	/*and remount to initiate recovery*/
	mount_filesystem();
	wait_for_response();

	return 1;
}

int fsync_workload()
{
	int crash_commit = 0;

	/*all the journal traffic that comes out of this workload 
	 *is dur fsync*/
	revoke_workload(crash_commit);
	
	return 1;
}

int prepare_for_testing(char *filename, int blknr, int minor_blktype)
{
	/*construct a fault specification with info specific to this file*/
	construct_fault(filename, blknr, minor_blktype);

	/*mount the file system anew before testing*/
	flush_from_cache();

	/*pass the fault to the driver*/
	inject_fault();

	/*clear the statistics*/
	notify_sba("clean_stats");

	return 1;
}

int test_system(char *input_file)
{
	int block_type;

	/*get the inputs about blocks to fail*/
	while ((block_type = get_next_block(input_file, sba_fault)) >= 0) {
		/*run workloads to initiate fs traffic to activate the fault*/
		run_all_workloads(block_type);
	}

	return 1;
}

int run_all_workloads(int block_type)
{
	int i;
	char fault_info[1024];
	workload *wkld;

	printf("block type = %s\n", get_block_type_str(block_type));
	wkld = get_all_workloads(block_type);
	if (!wkld) {
		fprintf(stderr, "Error: null workload\n");
		return -1;
	}

	printf("<<<<<<<<<<<<<< FIXME (wkld total %d) >>>>>>>>>>>>>>>>>>>>>>>\n", wkld->total);
	for (i = 11; i < 12; i ++) {
	//for (i = 0; i < wkld->total; i ++) {
		/*build the new file system setup for testing*/
		create_setup(block_type);

		/*run the workload 'i'*/
		if (run_workload(wkld->id[i], block_type, wkld->minor_blktype[i]) < 0) {
			sprintf(fault_info, "Error: workload %d (index %d) for block type %s did not matched the fault specification",
			wkld->id[i], i, get_block_type_str(block_type));
			fprintf(stderr, "%s", fault_info);
			vp_log_error(fault_info, NO_ERROR, !(EXIT_ON_ERROR));

			print_fault(sba_fault, fault_info);
			vp_log_error(fault_info, NO_ERROR, !(EXIT_ON_ERROR));

			vp_log_error(ERR_SEP, NO_ERROR, !(EXIT_ON_ERROR));
		}

		/*before running the next workload clear the statistics*/
		notify_sba("clean_stats");

		/*remove the fault from the driver*/
		remove_fault();
	}

	//free(wkld->id);
	//free(wkld->minor_blktype);
	free(wkld);

	return -1;
}

int run_workload(int workload, int blocktype, int minor_blktype)
{
	int ret = 1;
	char file1[1024];
	char file2[1024];

	printf("in run_workload (wkld %d) ...\n", workload);
	
	switch(workload) {
		/*
		 *read traffic: dir data, file's inode
		 *write traffic:
		 */
		case POS_ACCESS:
		{
			char mode[32];
			int argc = 3;
			char *argv[3];

			/*create the necessary state*/
			if (blocktype == SBA_EXT3_INODE) {
				switch(minor_blktype) {
					case FILE_INODE:
						sprintf(file1, "%s/%s", testdir, smallfile_pre);
						coord_create_file(file1, smallfile_size);
					break;

					case DIR_INODE:
						sprintf(file1, "%s/%s", testdir, smalldir_pre);
						coord_create_dir(file1, smalldir_size);
					break;

					case SYMLINK_INODE:
						sprintf(file1, "%s/%s", testdir, symlink_dir);
						coord_create_symlink(testdir, file1);
					break;

					default:
						printf("Error: returning -1 (minor %d)\n", minor_blktype);
						return -1;
				}
			}
			else {
				sprintf(file1, "%s/%s", testdir, smallfile_pre);
				coord_create_file(file1, smallfile_size);
			}

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					printf("ErroR: returning -1\n");
					return -1;
			}

			/*now run the test*/
			sprintf(mode, "%d", R_OK);
			argc = 3;
			argv[1] = file1;
			argv[2] = mode;

			run_tests(POS_ACCESS, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_ACCESS, argc, (void *)argv);
		}
		break;

		/*traffic: parent dir's data, chdir dir's inode*/
		case POS_CHDIR:
		{
			int argc = 2;
			char *argv[2];

			/*create the necessary state*/
			sprintf(file1, "%s/%s", testdir, smalldir_pre);
			coord_create_dir(file1, smalldir_size);

			/*create the necessary state*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			argc = 2;
			argv[1] = file1;
			run_tests(POS_CHDIR, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CHDIR, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode
		 *write traffic: jsuper, desc, jdata, commit, file's inode
		 */
		case POS_CHMOD:
		{
			char mode[16];
			int argc = 3;
			char *argv[3];

			/*create the necessary state*/
			if (blocktype == SBA_EXT3_INODE) {
				switch(minor_blktype) {
					case FILE_INODE:
						sprintf(file1, "%s/%s", testdir, smallfile_pre);
						coord_create_file(file1, smallfile_size);
					break;

					case DIR_INODE:
						sprintf(file1, "%s/%s", testdir, smalldir_pre);
						coord_create_dir(file1, smalldir_size);
					break;

					case SYMLINK_INODE:
						sprintf(file1, "%s/%s", testdir, symlink_dir);
						coord_create_symlink(testdir, file1);
					break;

					default:
						printf("Error: returning -1 (minor %d)\n", minor_blktype);
						return -1;
				}
			}
			else {
				sprintf(file1, "%s/%s", testdir, smallfile_pre);
				coord_create_file(file1, smallfile_size);
			}

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_JSUPER:
				case SBA_EXT3_DESC:
				case SBA_EXT3_COMMIT:
				case SBA_EXT3_JDATA:
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			sprintf(mode, "%d", S_IRUSR);
			argc = 3;
			argv[1] = file1;
			argv[2] = mode;

			run_tests(POS_CHMOD, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CHMOD, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode
		 *write traffic: jsuper, desc, jdata, commit, file's inode
		 */
		case POS_CHOWN:
		{
			char *owner = "0";
			char *group = "0";
			int argc = 4;
			char *argv[4];

			/*create the necessary state*/
			if (blocktype == SBA_EXT3_INODE) {
				switch(minor_blktype) {
					case FILE_INODE:
						sprintf(file1, "%s/%s", testdir, smallfile_pre);
						coord_create_file(file1, smallfile_size);
					break;

					case DIR_INODE:
						sprintf(file1, "%s/%s", testdir, smalldir_pre);
						coord_create_dir(file1, smalldir_size);
					break;

					case SYMLINK_INODE:
						sprintf(file1, "%s/%s", testdir, symlink_dir);
						coord_create_symlink(testdir, file1);
					break;

					default:
						printf("Error: returning -1 (minor %d)\n", minor_blktype);
						return -1;
				}
			}
			else {
				sprintf(file1, "%s/%s", testdir, smallfile_pre);
				coord_create_file(file1, smallfile_size);
			}

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(file1, -1, minor_blktype);
			}

			/*now run the test*/
			argc = 4;
			argv[1] = file1;
			argv[2] = owner;
			argv[3] = group;

			run_tests(POS_CHOWN, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CHOWN, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, dir's inode
		 *write traffic:
		 */
		case POS_CHROOT:
		{
			int argc = 2;
			char *argv[2];

			/*create the necessary state*/
			sprintf(file1, "%s/%s", testdir, smalldir_pre);
			coord_create_dir(file1, smalldir_size);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			argc = 2;
			argv[1] = file1;

			run_tests(POS_CHROOT, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CHROOT, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode, inode bitmap
		 *write traffic: dir's data, file's inode, inode bitmap, 
		 *jsuper, jdesc, jdata, jcommit, groupdesc
		 */
		case POS_CREAT:
		{
			char mode[16];
			int argc = 3;
			char *argv[3];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_JSUPER:
				case SBA_EXT3_DESC:
				case SBA_EXT3_COMMIT:
				case SBA_EXT3_JDATA:
				case SBA_EXT3_GROUP:
				case SBA_EXT3_IBITMAP:
				case SBA_EXT3_INODE:
					prepare_for_testing(NULL, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			sprintf(mode, "%d", S_IRWXU);
			argc = 3;
			argv[1] = file1;
			argv[2] = mode;

			run_tests(POS_CREAT, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CREAT, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode
		 *write traffic: 
		 */
		case POS_STAT:
		{
			struct stat buf;
			int argc = 3;
			void *argv[3];

			/*create the necessary state*/
			if (blocktype == SBA_EXT3_INODE) {
				switch(minor_blktype) {
					case FILE_INODE:
						sprintf(file1, "%s/%s", testdir, smallfile_pre);
						coord_create_file(file1, smallfile_size);
					break;

					case DIR_INODE:
						sprintf(file1, "%s/%s", testdir, smalldir_pre);
						coord_create_dir(file1, smalldir_size);
					break;

					default:
						printf("Error: returning -1 (minor %d)\n", minor_blktype);
						return -1;
				}
			}
			else {
				sprintf(file1, "%s/%s", testdir, smallfile_pre);
				coord_create_file(file1, smallfile_size);
			}

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			argc = 3;
			argv[1] = (void *)file1;
			argv[2] = (void *)&buf;

			run_tests(POS_STAT, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_STAT, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir inode, dir data (the super and grp desc are already read during mount)
		 *write traffic: 
		 */
		case POS_STATFS:
		{
			struct statfs buf;
			int argc = 3;
			void *argv[3];

			sprintf(file1, "%s/%s", testdir, smalldir_pre);
			coord_create_dir(file1, smalldir_size);

			/*prepare the system for testing*/
			prepare_for_testing(testdir, -1, minor_blktype);

			/*now run the test*/
			argc = 3;
			argv[1] = (void *)file1;
			argv[2] = (void *)&buf;

			run_tests(POS_STATFS, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_STATFS, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: 
		 *write traffic: all journal blk types. other fixed-location blocks 
		 *are written during checkpoint and doesn't come under fsync path
		 *so dont have to include them
		 */
		case POS_FSYNC:
		{
			if (sba_fault->blk_type == SBA_EXT3_INDIR) {
				/* create_file parameters: <file name> <size(KB)> */
				int argc = 3;
				char *argv[3];
				char file1[128];
				char fd_str[16];
				char buf[4096];
				int fd;

				sprintf(file1, "%s/%s", testdir, mediumfile_pre);
				argv[0] = "create_file";
				argv[1] = file1;
				argv[2] = mediumfile_size;
				create_file(argc, argv);
				prepare_for_testing(file1, -1, minor_blktype);

				fd = pos_open(file1, O_RDWR);
				sprintf(fd_str, "%d", fd);
				argc = 2;
				argv[1] = fd_str;

				lseek(fd, 15*4096, SEEK_SET);
				pos_write(fd, buf, 4096);

				/*now run the test*/
				run_tests(POS_FSYNC, argc, (void *)argv);

				/*run it again*/
				pos_write(fd, buf, 4096);
				run_tests(POS_FSYNC, argc, (void *)argv);

				close(fd);
			}
			else {
				/*prepare the system for testing*/
				prepare_for_testing(NULL, -1, minor_blktype);

				/*now run the test*/
				run_tests(POS_FSYNC, 0, NULL);
			}
		}
		break;

		/*
		 *read traffic: dbitmap, inode, dir data, indir blk
		 *write traffic: journal blks, dbitmap, inode, dir data, indir blk
		 *actually the write traffic is during logging and checkpointing and doesn't
		 *come under truncate. so only the read traffic makes sense.
		 */
		case POS_TRUNCATE:
		{
			int argc = 3;
			char *argv[3];

			sprintf(file1, "%s/%s", testdir, mediumfile_pre);
			coord_create_file(file1, mediumfile_size);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(file1, -1, minor_blktype);
			}

			/*now run the test*/
			argc = 3;
			argv[1] = file1;
			argv[2] = mediumfile_size;

			run_tests(POS_TRUNCATE, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_TRUNCATE, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir data (inode blk can't be failed because you 
		 *pass fd as an arg which must be gotten only after reading inode!)
		 *write traffic: jdesc, jdata, jcommit, dir data
		 */
		case POS_GETDIRENT:
		{
			char *nbytes = "5";
			char fd_str[4];
			char buf[256];
			int basep = 0;

			int fd;
			int argc = 5;
			void *argv[5];

			sprintf(file1, "%s/%s", testdir, smalldir_pre);
			coord_create_dir(file1, smalldir_size);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			fd = pos_open(file1, O_RDONLY);

			/*now run the test*/
			argc = 5;
			sprintf(fd_str, "%d", fd);
			argv[1] = fd_str;
			argv[2] = buf;
			argv[3] = nbytes;
			argv[4] = &basep;

			run_tests(POS_GETDIRENT, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_GETDIRENT, argc, (void *)argv);

			/*now close the fd - otherwise you wont be able to unmount*/
			pos_close(fd);
		}
		break;

		/*
		 *read traffic: dir data, file's inode
		 *write traffic: jdesc, jdata, jcommit, dir data, file's inode
		 */
		case POS_LINK:
		{
			int argc = 3;
			void *argv[3];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);
			coord_create_file(file1, smallfile_size);
			sprintf(file2, "%s/%s", testdir, linkfile);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			argc = 3;
			argv[1] = file1;
			argv[2] = file2;

			run_tests(POS_LINK, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_LINK, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir data, link's inode
		 *write traffic: 
		 */
		case POS_LSTAT:
		{
			struct stat buf;
			int argc = 3;
			void *argv[3];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);
			coord_create_file(file1, smallfile_size);
			sprintf(file2, "%s/%s", testdir, linkfile);
			coord_create_symlink(file1, file2);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file2, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			argc = 3;
			argv[1] = (void *)file2;
			argv[2] = (void *)&buf;

			run_tests(POS_LSTAT, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_LSTAT, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir data, inode bitmap, data bitmap
		 *write traffic: journal blks, dir data, inode bitmap, data bitmap
		 */
		case POS_MKDIR:
		{
			char *mode = "0777";
			int argc = 3;
			char *argv[3];

			sprintf(file1, "%s/%s", testdir, smalldir_pre);
			prepare_for_testing(testdir, -1, minor_blktype);

			/*now run the test*/
			argc = 3;
			argv[1] = file1;
			argv[2] = mode;

			run_tests(POS_MKDIR, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_MKDIR, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: super blk, grp desc, root dir inode, journal inode, journal super
		 *write traffic: super blk
		 */
		case POS_MOUNT:
		{
			int argc = 6;
			char *argv[6];

			unmount_filesystem();

			/*construct a fault specification with info specific to this file*/
			construct_fault(NULL, -1, minor_blktype);

			/*pass the fault to the driver*/
			inject_fault();

			/*clear the statistics*/
			notify_sba("clean_stats");

			/*now run the test*/
			argc = 6;
			argv[1] = DEVICE;
			argv[2] = "/mnt/sba/";
			argv[3] = "ext3";
			argv[4] = "0";
			argv[5] = NULL;
			
			run_tests(POS_MOUNT, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_MOUNT, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode, inode bitmap
		 *write traffic: dir's data, file's inode, inode bitmap, 
		 *jsuper, jdesc, jdata, jcommit, groupdesc
		 */
		case POS_CREATOPEN:
		{
			int argc = 4;
			char *argv[4];
			char flags[4];
			char mode[4];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_JSUPER:
				case SBA_EXT3_DESC:
				case SBA_EXT3_COMMIT:
				case SBA_EXT3_JDATA:
				case SBA_EXT3_GROUP:
				case SBA_EXT3_IBITMAP:
				case SBA_EXT3_INODE:
					prepare_for_testing(NULL, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			sprintf(flags, "%d", O_RDWR|O_CREAT);
			sprintf(mode, "%d", S_IRWXU);
			argc = 4;
			argv[1] = file1;
			argv[2] = flags;
			argv[3] = mode;

			run_tests(POS_CREATOPEN, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CREATOPEN, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode
		 *write traffic:
		 */
		case POS_OPEN:
		{
			int argc = 3;
			char *argv[3];
			char flags[4];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);
			coord_create_file(file1, smallfile_size);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_INODE:
					prepare_for_testing(file1, -1, minor_blktype);
				break;

				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					return -1;
			}

			/*now run the test*/
			sprintf(flags, "%d", O_RDWR);
			argc = 3;
			argv[1] = file1;
			argv[2] = flags;

			run_tests(POS_OPEN, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_OPEN, argc, (void *)argv);
		}
		break;

		/* read traffic: data, indir blks
		 * but we look only at data blk here. sindir_workload and
		 * dindir takes care of indir blks. you can't test inode
		 * becuase it would have gotten read during open
		 * write traffic:
		 */
		case POS_READ:
		{
			int fd;
			char fd_str[4];
			char *count = "4096";
			char buf[4096];

			int argc = 4;
			void *argv[4];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);
			coord_create_file(file1, dindir_size);
			//coord_create_file(file1, smallfile_size);
			prepare_for_testing(file1, 0, minor_blktype);

			fd = pos_open(file1, O_RDONLY);
			//lseek(fd, 100*4096, SEEK_SET);

			/*now run the test*/
			sprintf(fd_str, "%d", fd);
			argc = 4;
			argv[1] = fd_str;
			argv[2] = buf;
			argv[3] = count;

			run_tests(POS_READ, argc, (void *)argv);

			#if 0
			pos_close(fd);
			fd = pos_open(file1, O_RDONLY);
			/*now run the test*/
			sprintf(fd_str, "%d", fd);
			argc = 4;
			argv[1] = fd_str;
			argv[2] = buf;
			argv[3] = count;
			#endif

			/*run again to see if fs remembers*/
			run_tests(POS_READ, argc, (void *)argv);

			/*now close the fd - otherwise you wont be able to unmount*/
			pos_close(fd);
		}
		break;

		/* read traffic: dir, inode, data
		 * write traffic: */
		case POS_READLINK:
		{
			char *count = "4096";
			char buf[4096];

			int argc = 4;
			void *argv[4];

			sprintf(file1, "%s/%s", testdir, longfilename);
			coord_create_file(file1, smallfile_size);
			sprintf(file2, "%s/%s", testdir, linkfile);
			coord_create_symlink(file1, file2);

			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(file2, -1, minor_blktype);
			}

			/*now run the test*/
			argc = 4;
			argv[1] = file2;
			argv[2] = buf;
			argv[3] = count;

			run_tests(POS_READLINK, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_READLINK, argc, (void *)argv);
		}
		break;

		/* read traffic: dir, inode
		 * write traffic: journal blks*/
		case POS_RENAME:
		{
			int argc = 3;
			void *argv[3];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);
			coord_create_file(file1, smallfile_size);
			sprintf(file2, "%s/%s", testdir, linkfile);

			/*prepare the system for testing*/
			prepare_for_testing(testdir, -1, minor_blktype);

			/*now run the test*/
			argc = 3;
			argv[1] = file1;
			argv[2] = file2;

			run_tests(POS_RENAME, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_RENAME, argc, (void *)argv);
		}
		break;

		/* read traffic: dir, inode, inode bitmap, data bitmap
		 * write traffic: journal blks*/
		case POS_RMDIR:
		{
			int argc = 2;
			void *argv[2];

			sprintf(file1, "%s/%s", testdir, smalldir_pre);
			coord_create_dir(file1, "0");

			/*prepare the system for testing*/
			prepare_for_testing(file1, -1, minor_blktype);

			#if 0
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(file1, -1, minor_blktype);
			}
			#endif

			/*now run the test*/
			argc = 2;
			argv[1] = file1;

			run_tests(POS_RMDIR, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_RMDIR, argc, (void *)argv);
		}
		break;

		/* read traffic: dir, inode, inode bitmap, data bitmap
		 * write traffic: journal blks*/
		case POS_SYMLINK:
		{
			int argc = 3;
			void *argv[3];

			sprintf(file1, "%s/%s", testdir, longfilename);
			coord_create_file(file1, smallfile_size);
			sprintf(file2, "%s/%s", testdir, linkfile);

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(NULL, -1, minor_blktype);
			}

			/*now run the test*/
			argc = 3;
			argv[1] = file1;
			argv[2] = file2;

			run_tests(POS_SYMLINK, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_SYMLINK, argc, (void *)argv);
		}
		break;

		/* read traffic: 
		 * write traffic: all blks*/
		case POS_SYNC:
		{
			/* create_file parameters: <file name> <size(KB)> */
			int argc = 3;
			char *argv[3];
			char file1[128];

			sprintf(file1, "%s/%s", testdir, mediumfile_pre);
			argv[0] = "create_file";
			argv[1] = file1;
			argv[2] = mediumfile_size;

			if (sba_fault->blk_type == SBA_EXT3_INDIR) {
				create_file(argc, argv);
				prepare_for_testing(file1, -1, minor_blktype);

				argv[2] = "100";
				write_file_async(argc, argv);

				/*now run the test*/
				run_tests(POS_SYNC, 0, NULL);
			}
			else {
				prepare_for_testing(NULL, -1, minor_blktype);
				create_file_async(argc, argv);

				/*now run the test*/
				run_tests(POS_SYNC, 0, NULL);
			}
		}
		break;

		/* read traffic: dir, inode, inode bitmap, data bitmap, indir blk
		 * write traffic: all journal blks*/
		case POS_UNLINK:
		{
			int argc = 2;
			void *argv[2];

			if (blocktype == SBA_EXT3_INODE) {
				switch(minor_blktype) {
					case FILE_INODE:
						sprintf(file1, "%s/%s", testdir, mediumfile_pre);
						coord_create_file(file1, mediumfile_size);
					break;

					case SYMLINK_INODE:
						sprintf(file1, "%s/%s", testdir, longfilename);
						coord_create_file(file1, smallfile_size);
						sprintf(file2, "%s/%s", testdir, linkfile);
						coord_create_symlink(file1, file2);
					break;

					default:
						printf("Error: returning -1 (minor %d)\n", minor_blktype);
						return -1;
				}
			}
			else {
				sprintf(file1, "%s/%s", testdir, mediumfile_pre);
				coord_create_file(file1, mediumfile_size);
			}

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(file1, -1, minor_blktype);
			}

			/*now run the test*/
			argc = 2;
			argv[1] = file1;

			run_tests(POS_UNLINK, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_UNLINK, argc, (void *)argv);
		}
		break;

		/* read traffic: 
		 * write traffic: all blks*/
		case POS_UMOUNT:
		{
			/* create_file parameters: <file name> <size(KB)> */
			int argc = 3;
			char *argv[3];
			char file1[128];

			sprintf(file1, "%s/%s", testdir, mediumfile_pre);
			argv[0] = "create_file";
			argv[1] = file1;
			argv[2] = mediumfile_size;

			if (sba_fault->blk_type == SBA_EXT3_INDIR) {
				create_file(argc, argv);
				prepare_for_testing(file1, -1, minor_blktype);

				argv[0] = "write_file_async";
				argv[1] = file1;
				argv[2] = "100";
				write_file_async(argc, argv);
			}
			else {
				prepare_for_testing(NULL, -1, minor_blktype);
				create_file_async(argc, argv);
			}

			/*now run the test*/
			sprintf(file1, "/mnt/sba");
			argc = 2;
			argv[1] = file1;

			run_tests(POS_UMOUNT, argc, (void *)argv);

			run_tests(POS_UMOUNT, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: dir's data, file's inode
		 *write traffic: jsuper, desc, jdata, commit, file's inode
		 */
		case POS_UTIMES:
		{
			int argc = 3;
			void *argv[3];
			struct timeval tv;

			/*create the necessary state*/
			if (blocktype == SBA_EXT3_INODE) {
				switch(minor_blktype) {
					case FILE_INODE:
						sprintf(file1, "%s/%s", testdir, smallfile_pre);
						coord_create_file(file1, smallfile_size);
					break;

					case DIR_INODE:
						sprintf(file1, "%s/%s", testdir, smalldir_pre);
						coord_create_dir(file1, smalldir_size);
					break;

					case SYMLINK_INODE:
						sprintf(file1, "%s/%s", testdir, symlink_dir);
						coord_create_symlink(testdir, file1);
					break;

					default:
						printf("Error: returning -1 (minor %d)\n", minor_blktype);
						return -1;
				}
			}
			else {
				sprintf(file1, "%s/%s", testdir, smallfile_pre);
				coord_create_file(file1, smallfile_size);
			}

			/*prepare the system for testing*/
			switch (blocktype) {
				case SBA_EXT3_DIR:
					prepare_for_testing(testdir, -1, minor_blktype);
				break;

				default:
					prepare_for_testing(file1, -1, minor_blktype);
			}

			/*now run the test*/
			gettimeofday(&tv, NULL);
			argc = 3;
			argv[1] = file1;
			argv[2] = &tv;

			run_tests(POS_UTIMES, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_UTIMES, argc, (void *)argv);
		}
		break;

		/*
		 *read traffic: indir (inode and dir will be read during open)
		 *write traffic: jsuper, desc, jdata, commit, file's inode
		 *data, inode, data bitmap, indir
		 *we don't test the dir data here because that would've
		 *been tested in open
		 */
		case POS_WRITE:
		{
			int fd;
			char fd_str[4];
			char count_str[16];
			char *buf;
			int blk_no = 10;
			int total_writes = 5;

			int argc = 4;
			void *argv[4];

			buf = malloc(total_writes*4096);
			sprintf(count_str, "%d", total_writes*4096);

			sprintf(file1, "%s/%s", testdir, mediumfile_pre);
			coord_create_file(file1, mediumfile_size);

			/*in sba, we have configured it such that we can specify
			 *only direct blocks of a file and not any indir blocks
			 *so we give 10 here and write a long block that would 
			 *cause indir block to be read and written*/
			prepare_for_testing(file1, blk_no, minor_blktype);

			fd = pos_open(file1, O_RDWR);
			lseek(fd, blk_no*4096, SEEK_SET);

			/*now run the test*/
			sprintf(fd_str, "%d", fd);
			argc = 4;
			argv[1] = fd_str;
			argv[2] = buf;
			argv[3] = count_str;

			run_tests(POS_WRITE, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_WRITE, argc, (void *)argv);

			free(buf);

			/*now close the fd - otherwise you wont be able to unmount*/
			pos_close(fd);
		}
		break;

		/* possible blk types: data, inode, indir, data bitmap blks */
		case POS_CLOSE:
		{
			int fd;
			char fd_str[4];

			int argc = 2;
			void *argv[2];

			sprintf(file1, "%s/%s", testdir, smallfile_pre);
			coord_create_file(file1, smallfile_size);
			prepare_for_testing(file1, -1, minor_blktype);

			fd = pos_open(file1, O_RDWR);

			/*now run the test*/
			sprintf(fd_str, "%d", fd);
			argc = 2;
			argv[1] = fd_str;
			run_tests(POS_CLOSE, argc, (void *)argv);

			/*run again to see if fs remembers*/
			run_tests(POS_CLOSE, argc, (void *)argv);

			/*now close the fd - otherwise you wont be able to unmount*/
			pos_close(fd);
		}
		break;

		case SINDIR_WORKLOAD:
		{
			int blk;
			int argc = 3;
			char *argv[3];
			
			/* we vary the following: number of blocks that follow the block
			 * that is read (this can be done by changing the block that we
			 * read) and prefetch window */

			/* first create a big file - say like 1 MB */
			sprintf(file1, "%s/%s", testdir, sindir_file);
			coord_create_file(file1, sindir_size);

			/*prepare the system for testing*/
			prepare_for_testing(file1, -1, minor_blktype);

			/*now vary the block position and run the test*/
			for (blk = 250; blk < atoi(sindir_size)/4; blk ++) {
				char blk_no[16];
				char cmd[256];

				/*read that block - read_file parameters: <file name> [<block#>, <block#>, ...]*/
				sprintf(blk_no, "%d", blk);

				argc = 3;
				argv[0] = "read_file";
				argv[1] = file1;
				argv[2] = blk_no;

				printf("Running workload \"read_file\" to read %s ...\n", file1);
				run_tests(READ_FILE, argc, (void *)argv);

				/*if everything went correctly, the indir block would have been failed.
				 *now, save the trace elsewhere*/
				sprintf(cmd, "extract_stats > read-sindir-size%s-blk%d", sindir_size, blk);
				notify_sba(cmd);

				/*clear the statistics*/
				flush_from_cache();
				notify_sba("clean_stats");
			}
		}
		break;

		case DINDIR_WORKLOAD:
		{
			int blk;
			int argc = 3;
			char *argv[3];
			
			/* we vary the following: number of blocks that follow the block
			 * that is read (this can be done by changing the block that we
			 * read) and prefetch window */

			/* first create a big file */
			sprintf(file1, "%s/%s", testdir, dindir_file);
			coord_create_file(file1, dindir_size);

			/*prepare the system for testing*/
			prepare_for_testing(file1, -1, minor_blktype);

			/*now vary the block position*/
			for (blk = 950; blk < atoi(dindir_size)/4; blk ++) {
				char blk_no[16];
				char cmd[256];

				/*read that block - read_file parameters: <file name> [<block#>, <block#>, ...]*/
				sprintf(blk_no, "%d", blk);

				argc = 3;
				argv[0] = "read_file";
				argv[1] = file1;
				argv[2] = blk_no;

				printf("Running workload \"read_file\" to read %s ...\n", file1);
				run_tests(READ_FILE, argc, (void *)argv);

				/*if everything went correctly, the indir block would have been failed.
				 *now, save the trace elsewhere*/
				sprintf(cmd, "extract_stats > read-dindir-size%s-blk%d", dindir_size, blk);
				notify_sba(cmd);

				/*clear the statistics*/
				flush_from_cache();
				notify_sba("clean_stats");
			}
		}
		break;

		case REVOKE_WORKLOAD:
		{
			/*prepare the system for testing*/
			prepare_for_testing(NULL, -1, minor_blktype);

			run_tests(REVOKE_WORKLOAD, 0, NULL);
		}
		break;

		case DIRCRASH_WORKLOAD:
		{
			run_tests(DIRCRASH_WORKLOAD, 0, NULL);
		}
		break;

		case FILECRASH_WORKLOAD:
		{
			run_tests(FILECRASH_WORKLOAD, 0, NULL);
		}
		break;

		default:
			fprintf(stderr, "Error: unknown posix workload\n");
			ret = -1;
	}

	/*we have to wait until the file system checkpoints*/
	if (!fault_injected()) {
		wait_for_checkpoint();
	}

	/*check if this initiated the necessary traffic*/
	if (fault_injected()) {
		char cmd[1024];
		
		sprintf(cmd, "Fault is successfully injected\n");
		vp_log_error(cmd, NO_ERROR, !(EXIT_ON_ERROR));

		/*collect the statistics from SBA driver*/
		sprintf(cmd, "extract_stats >> %s", logfile);
		notify_sba(cmd);

		vp_log_error(ERR_SEP, NO_ERROR, !(EXIT_ON_ERROR));
	}
	else {
		ret = -1;
	}

	return ret;
}

int run_tests(int callid, int argc, void *argv[])
{
    int cpid;

    if ((cpid = fork()) == 0) {
		notify_sba("workload_start");

		switch(callid) {

			/*int pos_access(const char *pathname, int mode);*/
			case POS_ACCESS:
				pos_access((char *)argv[1], atoi((char *)argv[2]));
			break;

			/*int pos_chdir(const char *path);*/
			case POS_CHDIR:
				pos_chdir((char *)argv[1]);
			break;

			/*int pos_chmod(const char *path, mode_t mode);*/
			case POS_CHMOD:
				pos_chmod((char *)argv[1], atoi((char *)argv[2]));
			break;

			/*int pos_chown(const char *path, uid_t owner, gid_t group);*/
			case POS_CHOWN:
				pos_chown((char *)argv[1], atoi((char *)argv[2]), atoi((char *)argv[3]));
			break;

			/*int pos_chroot(const char *path);*/
			case POS_CHROOT:
				pos_chroot((char *)argv[1]);
			break;

			/*int pos_creat(const char *pathname, mode_t mode);*/
			case POS_CREAT:
				pos_creat((char *)argv[1], atoi((char *)argv[2]));
			break;

			/*int pos_stat(const char *file_name, struct stat *buf);*/
			case POS_STAT:
				pos_stat((char *)argv[1], (struct stat *)argv[2]);
			break;

			/*int pos_statfs(const char *path, struct statfs *buf);*/
			case POS_STATFS:
				pos_statfs((char *)argv[1], (struct statfs *)argv[2]);
			break;

			/*int pos_fsync(int fd);*/
			case POS_FSYNC:
				if (sba_fault->blk_type == SBA_EXT3_INDIR) {
					pos_fsync(atoi((char *)argv[1]));
				}
				else {
					/*all the write workloads like write/append calls fsync at end*/
					fsync_workload();
				}
			break;

			/*int pos_truncate(const char *path, off_t length);*/
			case POS_TRUNCATE:
				pos_truncate((char *)argv[1], atoi((char *)argv[2]));
			break;

			/*ssize_t pos_getdirentries(int fd, char *buf, size_t  nbytes, off_t *basep);*/
			case POS_GETDIRENT:
				pos_getdirentries(atoi((char *)argv[1]), (char *)argv[2], atoi((char *)argv[3]), (off_t *)argv[4]);
			break;

			/*int pos_link(const char *oldpath, const char *newpath);*/
			case POS_LINK:
				pos_link((char *)argv[1], (char *)argv[2]);
			break;

			/*int pos_lstat(const char *file_name, struct stat *buf)*/
			case POS_LSTAT:
				pos_lstat((char *)argv[1], (struct stat *)argv[2]);
			break;

			/*int pos_mkdir(const char *pathname, mode_t mode);*/
			case POS_MKDIR:
				pos_mkdir((char *)argv[1], atoi((char *)argv[2]));
			break;

			/*int pos_mount(char *src, char *tgt, char *fstype, long mntflags, void *data);*/
			case POS_MOUNT:
				pos_mount((char *)argv[1], (char *)argv[2], (char *)argv[3], atoi((char *)argv[4]), (void *)argv[5]);
			break;

			/*int pos_creat_open(const char *pathname, int flags, mode_t mode);*/
			case POS_CREATOPEN:
				pos_creat_open((char *)argv[1], atoi((char *)argv[2]), atoi((char *)argv[3]));
			break;

			/*int pos_open(const char *pathname, int flags);*/
			case POS_OPEN:
				pos_open((char *)argv[1], atoi((char *)argv[2]));
			break;

			/*int pos_read(int fd, void *buf, size_t count);*/
			case POS_READ:
				pos_read(atoi((char *)argv[1]), (void *)argv[2], atoi((char *)argv[3]));
			break;

			/*int pos_readlink(const char *path, char *buf, size_t bufsiz);*/
			case POS_READLINK:
				pos_readlink((char *)argv[1], (char *)argv[2], atoi((char *)argv[3]));
			break;

			/*int pos_rename(const char *oldpath, const char *newpath);*/
			case POS_RENAME:
				pos_rename((char *)argv[1], (char *)argv[2]);
			break;

			/*int pos_rmdir(const char *pathname);*/
			case POS_RMDIR:
				pos_rmdir((char *)argv[1]);
			break;

			/*int pos_symlink(const char *oldpath, const char *newpath);*/
			case POS_SYMLINK:
				pos_symlink((char *)argv[1], (char *)argv[2]);
			break;

			/*void pos_sync(void);*/
			case POS_SYNC:
				pos_sync();
			break;

			/*mode_t pos_umask(mode_t mask);*/
			case POS_UMASK:
				pos_umask(atoi((char *)argv[1]));
			break;

			/*int pos_unlink(const char *pathname);*/
			case POS_UNLINK:
				pos_unlink((char *)argv[1]);
			break;

			/*int pos_umount(const char *target);*/
			case POS_UMOUNT:
				pos_umount((char *)argv[1]);
			break;

			/*int pos_utimes(char *filename, struct timeval *tvp);*/
			case POS_UTIMES:
				pos_utimes((char *)argv[1], (struct timeval *)argv[2]);
			break;

			/*int pos_write(int fd, const void *buf, size_t count);*/
			case POS_WRITE:
				pos_write(atoi((char *)argv[1]), (void *)argv[2], atoi((char *)argv[3]));
			break;

			/*int pos_close(int fd);*/
			case POS_CLOSE:
				pos_close(atoi((char *)argv[1]));
			break;

			case READ_FILE:
            	read_file(argc, (char **)argv);
			break;

			case REVOKE_WORKLOAD:
            	recovery_workload();
			break;

			case DIRCRASH_WORKLOAD:
            	dircrash_workload();
			break;

			case FILECRASH_WORKLOAD:
            	filecrash_workload();
			break;

			default:
            	fprintf(stderr, "Error: Invalid Workload in run_tests\n");
		}

        exit(1);
    }
    else {
        waitpid(cpid, NULL, 0);
		notify_sba("workload_end");
    }

	if (sba_fault->rw == SBA_WRITE) {
		/*if this is for write, wait until the checkpoint happens
		 *so that we can run the next test*/
		wait_for_checkpoint();
	}

    return 1;
}
