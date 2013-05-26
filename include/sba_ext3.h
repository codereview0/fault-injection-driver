#ifndef __INCLUDE_SBA_EXT3_H__
#define __INCLUDE_SBA_EXT3_H__

#include <linux/ext3_jbd.h>
#include <linux/ext3_fs.h>
#include "sba.h"
#include "sba_ext3_defs.h"
#include "ht_at_wrappers.h"

/* Function declarations */
int sba_ext3_init(void);
int sba_ext3_cleanup(void);
int sba_ext3_clean_stats(void);
char *sba_ext3_get_block_type_str(int btype);
int sba_ext3_inode_block(long sector);
int sba_ext3_inodenr_2_blocknr(int inodenr, int *blocknr, int *ioffset);
int sba_ext3_group_2_inode_bitmap(int group);
int sba_ext3_get_inode_bitmap_blk(long sector);
int sba_ext3_get_inode_bitmap_offset(long sector);
int sba_ext3_group_2_data_bitmap(int group);
int sba_ext3_get_data_bitmap_blk(long sector);
int sba_ext3_data_bitmap_block(long sector);
int sba_ext3_inode_bitmap_block(long sector);
int sba_ext3_bitmap_block(long sector);
int sba_ext3_super_block(long sector, int size);
int sba_ext3_group_desc_block(long sector);
int sba_ext3_journal_request(struct bio *sba_bio);
int sba_ext3_journal_block(struct bio *sba_bio, sector_t sector);
int sba_ext3_mkfs_write(struct bio *sba_bio);
int sba_ext3_start(void);
int sba_ext3_print_journal(void);
int sba_ext3_find_journal_entries(void);
int sba_ext3_indir_block(int sector);
int sba_ext3_dir_block(int sector);
int sba_ext3_unjournaled_block_type(int blocknr);
int sba_ext3_non_journal_block_type(long sector, char *type, int size);
int sba_ext3_block_type(char *data, sector_t sector, char *type, struct bio *sba_bio);
int sba_ext3_init_indir_blocks(unsigned long inodenr);
int sba_ext3_init_dir_blocks(unsigned long inodenr);
int sba_ext3_fault_match(char *data, sector_t sector, fault *sba_fault);
int sba_ext3_process_fault(fault *sba_fault);

#endif /* __INCLUDE_SBA_EXT3_H__ */
