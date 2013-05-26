#ifndef __INCLUDE_SBA_JFS_H__
#define __INCLUDE_SBA_JFS_H__

#include <linux/fs.h>
#include <jfs/jfs_types.h>
#include <jfs/jfs_logmgr.h>
#include <jfs/jfs_superblock.h>
#include "sba.h"
#include "ht_at_wrappers.h"

#define JFS_SUPER	8	/* 8th block (block size = 4KB) */

int sba_jfs_init(void);
int sba_jfs_cleanup(void);
int sba_jfs_journal_request(struct bio *sba_bio);
int sba_jfs_journal_block(struct bio *sba_bio, sector_t sector);
int sba_jfs_mkfs_write(struct bio *sba_bio);
int sba_jfs_print_journal(void);
int sba_jfs_start(void);
int sba_jfs_find_journal_entries(void);
int sba_jfs_print_journaled_blocks(void);
int sba_jfs_insert_journaled_blocks(int blocknr);
int sba_jfs_remove_journaled_blocks(int blocknr);
int sba_jfs_journaled_block(int blocknr);
int sba_jfs_unjournaled_block_type(int blocknr);
int sba_jfs_non_journal_block_type(char *data, long sector, char *type, int size);
int sba_jfs_get_next_log_record(int nwords, int *target, char *data, int *offset);
int sba_jfs_handle_journal_block(int sector, char *data);
int sba_jfs_block_type(char *data, sector_t sector, char *type, struct bio *sba_bio);
int sba_jfs_fault_match(char *data, sector_t sector, fault *sba_fault);

#endif
