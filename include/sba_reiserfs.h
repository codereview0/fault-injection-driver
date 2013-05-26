#ifndef __INCLUDE_SBA_REISERFS_H__
#define __INCLUDE_SBA_REISERFS_H__

#include <linux/reiserfs_fs.h>
#include <linux/reiserfs_fs_sb.h>
//#include "sba_common.h"
#include "sba.h"
#include "ht_at_wrappers.h"

#define REISER_SUPER (REISERFS_DISK_OFFSET_IN_BYTES/SBA_BLKSIZE)

int sba_reiserfs_init(void);
int sba_reiserfs_cleanup(void);
int sba_reiserfs_journal_request(struct bio *sba_bio);
int sba_reiserfs_journal_block(struct bio *sba_bio, sector_t sector);
int sba_reiserfs_mkfs_write(struct bio *sba_bio);
int sba_reiserfs_print_journal(void);
int sba_reiserfs_start(void);
int sba_reiserfs_is_reiserfs_3_5(struct reiserfs_super_block * rs);
int sba_reiserfs_is_reiserfs_3_6(struct reiserfs_super_block * rs);
int sba_reiserfs_is_reiserfs_jr(struct reiserfs_super_block * rs);
int sba_reiserfs_is_any_reiserfs_magic_string (struct reiserfs_super_block *rs);
int sba_reiserfs_find_journal_entries(void);
int sba_reiserfs_insert_journaled_blocks(int blocknr);
int sba_reiserfs_remove_journaled_blocks(int blocknr);
int sba_reiserfs_journaled_block(int blocknr);
int sba_reiserfs_handle_descriptor_block(char *data);
int sba_reiserfs_unjournaled_block_type(int blocknr);
int sba_reiserfs_non_journal_block_type(char *data, long sector, char *type, int size);
int sba_reiserfs_journal_commit_block(char *data);
char *sba_reiserfs_get_journal_desc_magic(char *data);
int sba_reiserfs_journal_desc_block(char *data);
int sba_reiserfs_journal_super_block(char *data);
int sba_reiserfs_block_type(char *data, sector_t sector, char *type, struct bio *sba_bio);
int sba_reiserfs_fault_match(char *data, sector_t sector, fault *sba_fault);

#endif
