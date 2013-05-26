/*
 * This file contains semantics of reiserfs
 */

#include "sba_reiserfs.h"
#include "sba_common.h"

/*reiserfs journal info*/
int reiser_jour_start;
int reiser_jour_size;
int reiserfs_trans_id; /*to keep track of commit block*/

/*do we have a separate journal device*/
extern int jour_dev;

//this is where we store the faults issued
extern fault *sba_fault;

/*the journaling mode under which we work*/
extern int journaling_mode;

/*We need a journal table - journal is not contiguous!*/
hash_table *h_sba_reiserfs_journal = NULL;

/*We need a hash table to keep track of the journaled blocks*/
hash_table *h_reiserfs_journaled_blocks = NULL;

int sba_reiserfs_init(void)
{
	sba_debug(1, "Initializing the reiserfs data structures\n");
	ht_create(&h_sba_reiserfs_journal, "reiserfs_journal");
	ht_create(&h_reiserfs_journaled_blocks, "reiserfs_journal");

	return 1;
}

int sba_reiserfs_cleanup(void)
{
	ht_destroy(h_sba_reiserfs_journal);
	ht_destroy(h_reiserfs_journaled_blocks);
	return 1;
}

int sba_reiserfs_journal_request(struct bio *sba_bio)
{
	if (jour_dev) {
		if (iminor(sba_bio->bi_bdev->bd_inode) == JOUR_MINOR) {
			return 1;
		}
	}

	return 0;
}

int sba_reiserfs_journal_block(struct bio *sba_bio, sector_t sector)
{
	if (jour_dev) {
		if (sba_reiserfs_journal_request(sba_bio)) {
			int blk = SBA_SECTOR_TO_BLOCK(sector);

			if (ht_lookup(h_sba_reiserfs_journal, blk)) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
	else {
		int blk = SBA_SECTOR_TO_BLOCK(sector);

		if (ht_lookup(h_sba_reiserfs_journal, blk)) {
			return 1;
		}
		else {
			return 0;
		}
	}
}

int sba_reiserfs_mkfs_write(struct bio *sba_bio)
{
	int blk = SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector);
	sba_debug(1, "Received WRITE for blk# %d\n", blk);

	/*if a separate device is used for the journal, then return*/
	if (jour_dev) {
		if (sba_reiserfs_journal_request(sba_bio)) {
			ht_add(h_sba_reiserfs_journal, blk);
		}	
	}
	else {
		/*don't have to cache anything. the new code reads the blocks
		 *of the journal and builds the h_sba_reiserfs_journal*/
		sba_debug(0, "Not adding %d to h_sba_reiserfs_journal\n", blk);
	}

	return 1;
}

int sba_reiserfs_print_journal(void)
{
	ht_print(h_sba_reiserfs_journal);
	return 1;
}

int sba_reiserfs_start(void)
{
	sba_reiserfs_find_journal_entries();

	return 1;
}

int sba_reiserfs_is_reiserfs_3_5(struct reiserfs_super_block * rs)
{
	return !strncmp (rs->s_v1.s_magic,  REISERFS_SUPER_MAGIC_STRING, strlen(REISERFS_SUPER_MAGIC_STRING));
}

int sba_reiserfs_is_reiserfs_3_6(struct reiserfs_super_block * rs)
{
	return !strncmp (rs->s_v1.s_magic, REISER2FS_SUPER_MAGIC_STRING, strlen(REISER2FS_SUPER_MAGIC_STRING));
}

int sba_reiserfs_is_reiserfs_jr(struct reiserfs_super_block * rs)
{
	return !strncmp (rs->s_v1.s_magic, REISER2FS_JR_SUPER_MAGIC_STRING, strlen(REISER2FS_JR_SUPER_MAGIC_STRING));
}

int sba_reiserfs_is_any_reiserfs_magic_string (struct reiserfs_super_block *rs)
{
	return (sba_reiserfs_is_reiserfs_3_5 (rs) || sba_reiserfs_is_reiserfs_3_6 (rs) || sba_reiserfs_is_reiserfs_jr (rs));
}

int sba_reiserfs_find_journal_entries(void)
{
	int i;
	char *data;
	
	if ((data = read_block(REISER_SUPER)) != NULL) {
		struct reiserfs_super_block *rsb = (struct reiserfs_super_block *)data;

		if (sba_reiserfs_is_any_reiserfs_magic_string (rsb)) {
			sba_debug(1,"SUCCESS ! Found reiserfs superblock\n");

			sba_debug(1,"Journal begin = %u Journal size = %u\n", 
			rsb->s_v1.s_journal.jp_journal_1st_block, rsb->s_v1.s_journal.jp_journal_size);

			sba_debug(1,"Trans max = %u Max batch = %u\n", 
			rsb->s_v1.s_journal.jp_journal_trans_max, rsb->s_v1.s_journal.jp_journal_max_batch);

			sba_debug(1,"Commit age = %u Trans age = %u\n", 
			rsb->s_v1.s_journal.jp_journal_max_commit_age, rsb->s_v1.s_journal.jp_journal_max_trans_age);

			reiser_jour_start = rsb->s_v1.s_journal.jp_journal_1st_block;
			reiser_jour_size = rsb->s_v1.s_journal.jp_journal_size;
		}

		free_page((int)data);
	}

	
	for (i = 0; i < reiser_jour_size; i ++) {
		ht_add(h_sba_reiserfs_journal, reiser_jour_start + i);
	}

	sba_debug(1, "Begin = %d End = %d\n", reiser_jour_start, reiser_jour_start + reiser_jour_size - 1);

	return 1;
}

int sba_reiserfs_insert_journaled_blocks(int blocknr)
{
	if (ht_lookup(h_reiserfs_journaled_blocks, blocknr)) {
		sba_debug(1, "Block %d is being journaled again\n", blocknr);
		return 1;
	}

	ht_add_val(h_reiserfs_journaled_blocks, blocknr, 0);
	return 1;
}

int sba_reiserfs_remove_journaled_blocks(int blocknr)
{
	if (ht_remove(h_reiserfs_journaled_blocks, blocknr) < 0) {
		sba_debug(1, "Error removing the blocknr %d\n", blocknr);
		return -1;
	}

	return 1;
}

int sba_reiserfs_journaled_block(int blocknr)
{
	if (ht_lookup(h_reiserfs_journaled_blocks, blocknr)) {
		return 1;
	}

	return 0;
}

int sba_reiserfs_handle_descriptor_block(char *data)
{
	int i;
	int blocknr;
	int trans_half;
	struct reiserfs_journal_desc *desc;
	desc = (struct reiserfs_journal_desc *)(data) ;

	trans_half = journal_trans_half (4096);

	for (i = 0 ; i < le32_to_cpu(desc->j_len) ; i++) {
		if (i < trans_half) {

			blocknr = le32_to_cpu(desc->j_realblock[i]);
			sba_debug(1, "Descriptor tag: block number = %d\n", blocknr);

			if (sba_reiserfs_insert_journaled_blocks(blocknr) == -1) {
				sba_debug(1, "Error: inserting blk=%d into journaled blocks list\n", blocknr);
			}
		}
	}

	return 1;
}

int sba_reiserfs_unjournaled_block_type(int blocknr)
{
	if (journaling_mode == ORDERED_JOURNALING) {
		return ORDERED_BLOCK;
	}
	else
	if (journaling_mode == WRITEBACK_JOURNALING) {
		return UNORDERED_BLOCK;
	}
	else {
		sba_debug(1, "Error: Seeing an unjournaled block %d in data journaling\n", blocknr);
		return UNORDERED_BLOCK;
	}
}

int sba_reiserfs_non_journal_block_type(char *data, long sector, char *type, int size)
{
	/* Check if this blocknr is in the list of journaled blocks. 
	 * If so, return CHECKPOINT_BLOCK. Else return ORDERED_BLOCK 
	 * or UNORDERED_BLOCK depending upon the journaling mode.*/
	
	int blocknr = SBA_SECTOR_TO_BLOCK(sector);

	if (sba_reiserfs_journaled_block(blocknr)) {
		sba_reiserfs_remove_journaled_blocks(blocknr);

		#if 0
		/*
		 * check if this is a super block. if so,
		 * return journal super block
		 */
		if (sba_reiserfs_journal_super_block(data)){
			return JOURNAL_SUPER_BLOCK;
		}
		else {
			return CHECKPOINT_BLOCK;
		}
		#else
			return CHECKPOINT_BLOCK;
		#endif
	}
	else {
		if (reiser_jour_start + reiser_jour_size == blocknr) {
			int oldest_start;
			struct reiserfs_journal_header * jh;
			jh = (struct reiserfs_journal_header *)data;

			oldest_start = reiser_jour_start + le32_to_cpu(jh->j_first_unflushed_offset);
			sba_debug(1, "first unflushed offset = %d oldest start = %d\n",
			le32_to_cpu(jh->j_first_unflushed_offset), oldest_start);

			return JOURNAL_SUPER_BLOCK;
		}
		else {
			return sba_reiserfs_unjournaled_block_type(blocknr);
		}
	}
}

int sba_reiserfs_journal_commit_block(char *data)
{
	struct reiserfs_journal_commit *commit;
	commit = (struct reiserfs_journal_commit *)(data) ;

	if (commit->j_trans_id == reiserfs_trans_id) {
		reiserfs_trans_id = -1;
	 	return 1;
	}

	return 0;
}

/* look at get_journal_desc_magic in linux src code */
char *sba_reiserfs_get_journal_desc_magic(char *data)
{
	return (data + 4096 - 12);
}

int sba_reiserfs_journal_desc_block(char *data)
{
	struct reiserfs_journal_desc *desc;
	desc = (struct reiserfs_journal_desc *)(data) ;

	if (memcmp(sba_reiserfs_get_journal_desc_magic(data), JOURNAL_DESC_MAGIC, 8) == 0) {
		reiserfs_trans_id = desc->j_trans_id;
	 	return 1;
	}

	return 0;
}

int sba_reiserfs_journal_super_block(char *data)
{
	struct reiserfs_super_block *rsb = (struct reiserfs_super_block *)data;

	if (sba_reiserfs_is_any_reiserfs_magic_string (rsb)) {
		return 1;
	}

	return 0;
}

int sba_reiserfs_block_type(char *data, sector_t sector, char *type, struct bio *sba_bio)
{
	int ret = UNKNOWN_BLOCK;

	sba_debug(0, "Got block %ld\n", SBA_SECTOR_TO_BLOCK(sector));

	if (sba_reiserfs_journal_block(sba_bio, sector)) {
	
		sba_debug(0, "Block %ld is a journal block\n", SBA_SECTOR_TO_BLOCK(sector));
		
		/* FIXME: what about revoke blocks ?? */
		if (sba_reiserfs_journal_desc_block(data)) {
			strcpy(type, "J_D");
			ret = JOURNAL_DESC_BLOCK;

			/* 
			 * Find out the real block numbers from this desc block.
			 * sba_reiserfs_handle_descriptor_block() will add the 
			 * real blocknr to a hash_table so that when the checkpt
			 * write comes, it can be identified properly.
			 */

			sba_reiserfs_handle_descriptor_block(data);
		}
		else
		if (sba_reiserfs_journal_commit_block(data)) {
			strcpy(type, "J_C");
			ret = JOURNAL_COMMIT_BLOCK;
		}
		else 
		if (sba_reiserfs_journal_super_block(data)){
			#if 1
			/* 
			 * there is no separate location for journal 
			 * super block. this is just another journal 
			 * data block
			 */
			strcpy(type, "J_O");
			ret = JOURNAL_DATA_BLOCK;
			#else
				strcpy(type, "J_S");
				ret = JOURNAL_SUPER_BLOCK;
			#endif
		}
		else {
			strcpy(type, "J_O");
			ret = JOURNAL_DATA_BLOCK;
		}
	}
	else {
		sba_debug(0, "Block %ld is a non-journal block\n", SBA_SECTOR_TO_BLOCK(sector));

		ret = sba_reiserfs_non_journal_block_type(data, sector, type, bio_sectors(sba_bio)*SBA_HARDSECT);
	}

	sba_debug(0, "returning block type %d\n", ret);
	return ret;
}

int sba_reiserfs_fault_match(char *data, sector_t sector, fault *sba_fault)
{
	switch(sba_fault->blk_type) {
		case JOURNAL_DESC_BLOCK:
		case JOURNAL_REVOKE_BLOCK:
		case JOURNAL_COMMIT_BLOCK:
			return 1;
		break;
		
		case JOURNAL_SUPER_BLOCK:
			return 1;
		break;
		
		case ORDERED_BLOCK:
		case UNORDERED_BLOCK:
		case CHECKPOINT_BLOCK:
		case JOURNAL_DATA_BLOCK:
			if (sba_fault->blocknr != -1) {
				if (SBA_SECTOR_TO_BLOCK(sector) == sba_fault->blocknr) {
					sba_debug(1, "Returning true for block# %d ... \n", sba_fault->blocknr);
					return 1;
				}
				else {
					return 0;
				}
			}
			else {
				return 1;
			}
		break;
		
		default:
			return 0;
	}
	
	return 0;
}
