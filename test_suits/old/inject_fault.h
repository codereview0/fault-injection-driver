#ifndef __INCLUDE_INJECT_FAULT_H__
#define __INCLUDE_INJECT_FAULT_H__

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
#include "sba_ext3_defs.h"
#include "colony.h"

#define SCRIPTS		"./scripts"
#define TEST_SUITS	"./test_suits"
#define FLUSH		"flush_and_mount.sh"
#define COLONY		"colony"

typedef struct _tests {
	char *name;
	int total_tests;
	int *test;
} tests;

typedef struct _bug_bag {
	int field_type;
	int total_values;
	void **values;
	tests file_tests;
	tests dir_tests;
} bug_bag;

#define LARGE_MIN		-1
#define	ZERO			0
#define	LARGE_MAX		((2^31)-1)

int init_system();
int cleanup_system();
int unmount_filesystem(int which_fs);
int mount_filesystem(int filesystem, int which_fs);
int clean_dir(char *dirname);
int move_to_clean_state(int filesystem, int state, int which_fs);
char *get_block_type_str(int btype);
int print_fault(fault *sba_fault);
int inject_fault(int dev_fd, int request, void *data);
int wait_for_response();
int mount_after_crash(int fd, int filesystem, int which_fs);
int flush_from_cache(int filesystem, int which_fs);
int move_sba_to_start_state(int dev_fd);
int stop_verifying(int dev_fd);
int start_verifying(int dev_fd);
char *read_block(int dev_fd, int blocknr);
int zero_block(int dev_fd, int blocknr);
int squash_writes(int dev_fd);
int allow_writes(int dev_fd);
int zero_ext3_journal(int dev_fd);
int compare_filesystems(char *f_root, char *c_root);
int prepare_system(int dev_fd, fault *sba_fault, int state, int inject);
int fail_reiserfs_super_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_jfs_super_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_super_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_unordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_reiserfs_commit_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_jfs_commit_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_commit_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int revoke_ok(int filesystem);
int fail_revoke_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_reiserfs_desc_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_desc_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_reiserfs_data_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_data_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_jfs_checkpoint_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_checkpoint_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_reiserfs_ordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_jfs_ordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int fail_ordered_block(int dev_fd, int nf_dev_fd, fault *sba_fault);
int issue_fail_fault(int dev_fd, int nf_dev_fd, fault *sba_fault);
int issue_fault(int dev_fd, int nf_dev_fd, fault *sba_fault);
int set_jfs_params(char *btype, fault *sba_fault);
int set_ext3_reiserfs_params(char *btype, fault *sba_fault);

#endif
