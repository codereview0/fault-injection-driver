/*
 * This file contains semantics of jfs
 */

#include "sba_jfs.h"
#include "sba_common.h"

/*jfs journal info*/
int jfs_jour_start;
int jfs_jour_size;

/*do we have a separate journal device*/
extern int jour_dev;

//this is where we store the faults issued
extern fault *sba_fault;

/*the journaling mode under which we work*/
extern int journaling_mode;

/*We need a journal table - journal is not contiguous!*/
hash_table *h_sba_jfs_journal = NULL;

/*We need a hash table to keep track of the journaled blocks*/
hash_table *h_jfs_journaled_blocks = NULL;

int sba_jfs_init(void)
{
	sba_debug(1, "Initializing the jfs data structures\n");
	ht_create(&h_sba_jfs_journal, "jfs_journal");
	ht_create(&h_jfs_journaled_blocks, "jfs_journal");

	return 1;
}

int sba_jfs_cleanup(void)
{
	ht_destroy(h_sba_jfs_journal);
	ht_destroy(h_jfs_journaled_blocks);
	return 1;
}

int sba_jfs_journal_request(struct bio *sba_bio)
{
	if (jour_dev) {
		if (iminor(sba_bio->bi_bdev->bd_inode) == JOUR_MINOR) {
			return 1;
		}
	}

	return 0;
}

int sba_jfs_journal_block(struct bio *sba_bio, sector_t sector)
{
	if (jour_dev) {
		if (sba_jfs_journal_request(sba_bio)) {
			int blk = SBA_SECTOR_TO_BLOCK(sector);

			if (ht_lookup(h_sba_jfs_journal, blk)) {
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

		if (ht_lookup(h_sba_jfs_journal, blk)) {
			return 1;
		}
		else {
			return 0;
		}
	}
}

int sba_jfs_mkfs_write(struct bio *sba_bio)
{
	int blk = SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector);
	sba_debug(1, "Mkfs write: Received WRITE for blk# %d\n", blk);

	/*if a separate device is used for the journal, then return*/
	if (jour_dev) {
		if (sba_jfs_journal_request(sba_bio)) {
			ht_add(h_sba_jfs_journal, blk);
		}	
	}
	else {
		/*don't have to cache anything. the new code reads the blocks
		 *of the journal and builds the h_sba_jfs_journal*/
		sba_debug(0, "Not adding %d to h_sba_jfs_journal\n", blk);
	}

	return 1;
}

int sba_jfs_print_journal(void)
{
	ht_print(h_sba_jfs_journal);
	return 1;
}

int sba_jfs_start(void)
{
	sba_jfs_find_journal_entries();

	return 1;
}

int sba_jfs_find_journal_entries(void)
{
	char *data;
	
	if ((data = read_block(JFS_SUPER)) != NULL) {
		int i;

		struct jfs_superblock *jfs_sb = (struct jfs_superblock *)data;
		sba_debug(1, "magic number = %s\n", jfs_sb->s_magic);
		sba_debug(1, "block size = %d\n", jfs_sb->s_bsize);

		jfs_jour_start = addressPXD(&jfs_sb->s_logpxd);
		jfs_jour_size = lengthPXD(&jfs_sb->s_logpxd);

		sba_debug(1, "log address = %d\n", jfs_jour_start);
		sba_debug(1, "log size = %d\n", jfs_jour_size);

		for (i = 0; i < jfs_jour_size; i ++) {
			ht_add(h_sba_jfs_journal, i+jfs_jour_start);
		}

		sba_debug(1, "Added log blocks from %d to %d\n", jfs_jour_start, jfs_jour_start + jfs_jour_size - 1);
		free_page((int)data);
	}

	return 1;
}

int sba_jfs_print_journaled_blocks(void)
{
	int blocknr;
	ht_open_scan(h_jfs_journaled_blocks);

	while (ht_scan(h_jfs_journaled_blocks, &blocknr) > 0) {
		sba_debug(1, "Block %d is journaled\n", blocknr);
	}

	return 1;
}

int sba_jfs_insert_journaled_blocks(int blocknr)
{
	if (ht_lookup(h_jfs_journaled_blocks, blocknr)) {
		sba_debug(0, "Block %d is being journaled again\n", blocknr);
		return 1;
	}

	ht_add_val(h_jfs_journaled_blocks, blocknr, 0);
	return 1;
}

int sba_jfs_remove_journaled_blocks(int blocknr)
{
	if (ht_remove(h_jfs_journaled_blocks, blocknr) < 0) {
		sba_debug(1, "Error removing the blocknr %d\n", blocknr);
		return -1;
	}

	return 1;
}

int sba_jfs_journaled_block(int blocknr)
{
	if (ht_lookup(h_jfs_journaled_blocks, blocknr)) {
		return 1;
	}

	return 0;
}

int sba_jfs_unjournaled_block_type(int blocknr)
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

int sba_jfs_non_journal_block_type(char *data, long sector, char *type, int size)
{
	/* Check if this blocknr is in the list of journaled blocks. 
	 * If so, return CHECKPOINT_BLOCK. Else return ORDERED_BLOCK 
	 * or UNORDERED_BLOCK depending upon the journaling mode.*/
	
	int blocknr = SBA_SECTOR_TO_BLOCK(sector);

	if (sba_jfs_journaled_block(blocknr)) {
		sba_jfs_remove_journaled_blocks(blocknr);

		return CHECKPOINT_BLOCK;
	}
	else {
		return sba_jfs_unjournaled_block_type(blocknr);
	}
}

int sba_jfs_get_next_log_record(int nwords, int *target, char *data, int *offset)
{
	struct logpage *lp;
	int n, j, words;
	int *ptr;

	/* apart from having the log record and (possibly) log data, the 
	 * log page contains a header and trailer structure */

	lp = (struct logpage *)data;
	j = (*offset - LOGPHDRSIZE) / 4 - 1;	/* index in log page data area of first word to move      */
	words = min(nwords, j + 1);	/* words on this page to move */
	ptr = target + nwords - 1;	/* last word of target */
	for (n = 0; n < words; n++) {
		*ptr = lp->data[j];
		j = j - 1;
		ptr = ptr - 1;
	}
	*offset = *offset - 4 * words;

	if (words != nwords) {
		return -1;
	}

	return 1;
}

int sba_jfs_journal_super_block(int sector)
{
	if (jfs_jour_start + LOGSUPER_B == SBA_SECTOR_TO_BLOCK(sector)) {
		return 1;
	}

	return 0;
}

/* look at jfs_logredo() and logRead() from logredo in jfsutils */
int sba_jfs_handle_journal_block(int sector, char *data)
{
	int off;
	int ret;
	struct logpage *lp;
	struct lrd *ld;
	char *log_data_ptr;
	void *ptr;
	int ld_flag;
	int nwords;
	int type = JOURNAL_DATA_BLOCK;

	if (sba_jfs_journal_super_block(sector)) {
		return JOURNAL_SUPER_BLOCK;
	}

	/* each log page that is written is of the form "struct logpage" */
	lp = (struct logpage *)data;
	off = lp->h.eor & (LOGPSIZE - 1);

	sba_debug(0, "The end of log offset of last record write = %d (h), %d (t)\n", lp->h.eor, lp->t.eor);
	sba_debug(0, "offset is %d head %u tail %u\n", off, (unsigned int)&(lp->h), (unsigned int)&(lp->t));

	ld = kmalloc(sizeof(struct logpage), GFP_KERNEL);
	log_data_ptr = kmalloc(4096, GFP_KERNEL);
	
	nwords = LOGRDSIZE/4;
	ret = sba_jfs_get_next_log_record(nwords, (int *)ld, data, &off);
	ld_flag = 1;

	while (ret >= 0) {

		if (ld_flag) {
			sba_debug(0, "Length = %d , Backchain = %d\n", ld->length, ld->backchain);

			long long blocknr = -1;

			switch (ld->type) {
				case LOG_COMMIT:
					sba_debug(0, "Found a LOG_COMMIT record\n");
					type = JOURNAL_COMMIT_BLOCK;
					break;

				case LOG_MOUNT:
					sba_debug(0, "Found a LOG_MOUNT record\n");
					break;

				case LOG_SYNCPT:
					sba_debug(0, "Found a LOG_SYNCPT record\n");
					break;

				case LOG_REDOPAGE:
					sba_debug(0, "Found a LOG_REDOPAGE record (address %lld, length %d)\n", 
					addressPXD(&ld->log.redopage.pxd), lengthPXD(&ld->log.redopage.pxd));
					blocknr = addressPXD(&ld->log.redopage.pxd);
					break;

				case LOG_NOREDOPAGE:
					sba_debug(0, "Found a LOG_NOREDOPAGE record (address %lld length %d)\n", 
					addressPXD(&ld->log.noredopage.pxd), lengthPXD(&ld->log.noredopage.pxd));
					blocknr = addressPXD(&ld->log.noredopage.pxd);
					break;

				case LOG_NOREDOINOEXT:
					sba_debug(0, "Found a LOG_NOREDOINOEXT record (address %lld length %d)\n", 
					addressPXD(&ld->log.noredoinoext.pxd), lengthPXD(&ld->log.noredoinoext.pxd));
					blocknr = addressPXD(&ld->log.noredoinoext.pxd);
					break;

				case LOG_UPDATEMAP:
					sba_debug(0, "Found a LOG_UPDATEMAP record (address %lld length %d nxd %d)\n", 
					addressPXD(&ld->log.updatemap.pxd), lengthPXD(&ld->log.updatemap.pxd), ld->log.updatemap.nxd);
					blocknr = addressPXD(&ld->log.updatemap.pxd);
					break;

				default:
					sba_debug(1, "Found an UNKNOWN record\n");
					break;
			}

			if (blocknr != -1) {
				if (sba_jfs_insert_journaled_blocks(blocknr) == -1) {
					sba_debug(1, "Error: inserting blk=%lld into journaled blocks list\n", blocknr);
				}
			}

			sba_debug(0, "Next offset = %d\n", off);
		}
		else {
			sba_debug(0, "Skipping data = %d words\n", nwords);
		}

		if (ld_flag) {
			if (ld->length > 0) {
				nwords = (ld->length + 3) / 4;
				ptr = log_data_ptr;
				ld_flag = 0;
			}
			else {
				nwords = LOGRDSIZE/4;
				ptr = ld;
				ld_flag = 1;
			}
		}
		else {
			nwords = LOGRDSIZE/4;
			ptr = ld;
			ld_flag = 1;
		}

		ret = sba_jfs_get_next_log_record(nwords, (int *)ptr, data, &off);
	}

	return type;
}

int sba_jfs_block_type(char *data, sector_t sector, char *type, struct bio *sba_bio)
{
	int ret = UNKNOWN_BLOCK;

	sba_debug(0, "Got block %ld\n", SBA_SECTOR_TO_BLOCK(sector));

	if (sba_jfs_journal_block(sba_bio, sector)) {
		sba_debug(0, "Block %ld is a journal block\n", SBA_SECTOR_TO_BLOCK(sector));
		
		ret = sba_jfs_handle_journal_block(sector, data);
	}
	else {
		sba_debug(0, "Block %ld is a non-journal block\n", SBA_SECTOR_TO_BLOCK(sector));

		ret = sba_jfs_non_journal_block_type(data, sector, type, bio_sectors(sba_bio)*SBA_HARDSECT);
	}

	sba_debug(0, "returning block type %d\n", ret);
	return ret;
}

int sba_jfs_fault_match(char *data, sector_t sector, fault *sba_fault)
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
