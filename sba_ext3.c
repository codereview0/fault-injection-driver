#include "sba_ext3.h"

/*do we have a separate journal device*/
extern int jour_dev;

//this is where we store the faults issued
extern fault *sba_fault;

/*the journaling mode under which we work*/
extern int journaling_mode;

/*inode start table lists the starting block of the inode table
 *for each cyl group*/
hash_table *h_ext3_inode_table_start = NULL;

/*tables for inode and data bitmaps*/
hash_table *h_ext3_inode_bitmap = NULL;
hash_table *h_ext3_data_bitmap = NULL;

/*We need a journal table - journal is not contiguous!*/
hash_table *h_sba_ext3_journal = NULL;

/*a hash table to keep track of the journaled blocks*/
hash_table *h_ext3_journaled_blocks = NULL;

/*hash table to keep the dir blocks*/
hash_table *h_ext3_dir_blocks = NULL;

/*hash table to keep the indir blocks*/
hash_table *h_ext3_indir_blocks = NULL;

/*hash table to keep the journal indir blocks*/
hash_table *h_ext3_journal_indir_blocks = NULL;

/*hash table to keep the journal to real block mapping.
 *this table will be constructed during journal desc
 *block read during recovery.*/
hash_table *h_ext3_journal_2_real = NULL;

/*inode blocks per group*/
int sba_ext3_inode_blks_per_group = SBA_EXT3_INODE_BLKS_PER_GROUP;

int sba_ext3_init()
{
	sba_debug(1, "Initializing the ext3 data structures\n");
	ht_create(&h_ext3_inode_table_start, "ext3inotab");
	ht_create(&h_ext3_inode_bitmap, "ext3inobm");
	ht_create(&h_ext3_data_bitmap, "ext3databm");
	ht_create(&h_sba_ext3_journal, "ext3 journal");
	ht_create(&h_ext3_journaled_blocks, "ext3 journal");
	ht_create(&h_ext3_dir_blocks, "ext3 dirs");
	ht_create(&h_ext3_indir_blocks, "ext3 indirs");
	ht_create(&h_ext3_journal_indir_blocks, "ext3jindirs");
	ht_create(&h_ext3_journal_2_real, "journal2real");

	return 1;
}

int sba_ext3_cleanup()
{
	ht_destroy(h_ext3_inode_table_start);
	ht_destroy(h_ext3_inode_bitmap);
	ht_destroy(h_ext3_data_bitmap);
	ht_destroy(h_sba_ext3_journal);
	ht_destroy(h_ext3_journaled_blocks);
	ht_destroy(h_ext3_dir_blocks);
	ht_destroy(h_ext3_indir_blocks);
	ht_destroy(h_ext3_journal_indir_blocks);
	ht_destroy(h_ext3_journal_2_real);

	return 1;
}

int sba_ext3_clean_stats()
{
	/*first destroy everything that was stored previously*/
	sba_ext3_cleanup();

	/*and now, create new hash tables*/
	sba_ext3_init();

	return 1;
}

char *sba_ext3_get_block_type_str(int btype)
{
	switch(btype) {
		case SBA_EXT3_INODE:
			return "E_INO";
		break;

		case SBA_EXT3_DBITMAP:
			return "E_DBM";
		break;

		case SBA_EXT3_IBITMAP:
			return "E_IBM";
		break;

		case SBA_EXT3_SUPER:
			return "E_SUP";
		break;

		case SBA_EXT3_GROUP:
			return "E_GRP";
		break;

		case SBA_EXT3_DATA:
			return "E_DAT";
		break;

		case SBA_EXT3_REVOKE:
			return "E_REV";
		break;

		case SBA_EXT3_DESC:
			return "E_DES";
		break;

		case SBA_EXT3_COMMIT:
			return "E_COM";
		break;

		case SBA_EXT3_JSUPER:
			return "E_JSB";
		break;

		case SBA_EXT3_JDATA:
			return "E_JDB";
		break;

		case SBA_EXT3_DIR:
			return "E_DIR";
		break;

		case SBA_EXT3_SINDIR:
			return "E_SIB";
		break;

		case SBA_EXT3_DINDIR:
			return "E_DIB";
		break;

		case SBA_EXT3_TINDIR:
			return "E_TIB";
		break;

		case SBA_EXT3_JINDIR:
			return "E_JIB";
		break;

		default:
			return "UNKON";
	}
}

int sba_ext3_inode_block(long sector)
{
	int group = SBA_GP_NO(sector);
	int blk = SBA_SECTOR_TO_BLOCK(sector);
	int inode_start = 0;
	
	/*128 because each grp desc is of size 32 bytes*/
	if (group < 128) {
		if (ht_lookup_val(h_ext3_inode_table_start, group, &inode_start)) {
		}
		else {
			sba_debug(1, "Error: unable to find the inode table start for grp# %d\n", group);
			return -1;
		}
	}
	else {
		sba_debug(1, "Error: grp greater than 128\n");
		return -1;
	}

	if ((blk >= inode_start) && (blk < inode_start + sba_ext3_inode_blks_per_group)) {
		/* for every group, we track the two bitmap blks, 
		 * two possible superblocks and the inode blks */
		int pos = (sba_ext3_inode_blks_per_group + 4)*group + (blk - inode_start);
		return pos;
  	}

	return -1;
}

/*The inodenr passed here is the one that is returned by fstat*/
int sba_ext3_inodenr_2_blocknr(int inodenr, int *blocknr, int *ioffset)
{
	int group;
	int inodes_per_gp;
	int inode_start;
	int offset_inodes;

	/*calculate the inode block number and the inode offset*/
	inodenr --;

	inodes_per_gp = (sba_ext3_inode_blks_per_group * SBA_NR_INODES_PER_BLK);
	group = inodenr / inodes_per_gp;

	/*128 because each grp desc is of size 32 bytes*/
	if (group < 128) {
		if (ht_lookup_val(h_ext3_inode_table_start, group, &inode_start)) {
		}
		else {
			sba_debug(1, "Error: unable to find the inode table start for grp# %d\n", group);
			return -1;
		}
	}
	else {
		sba_debug(1, "Error: grp greater than 128\n");
		return -1;
	}

	offset_inodes = inodenr - (group * inodes_per_gp);

	*blocknr = inode_start + offset_inodes/SBA_NR_INODES_PER_BLK;
	*ioffset = offset_inodes % SBA_NR_INODES_PER_BLK;

	sba_debug(1, "inodenr = %d blocknr = %d ioffset = %d\n", inodenr, *blocknr, *ioffset);

	return 1;
}


/* 
 * given a group, this method returns the block number of 
 * the inode bitmap block for that group
 */
int sba_ext3_group_2_inode_bitmap(int group)
{
	int inobm_blk = -1;
	ht_lookup_val(h_ext3_inode_bitmap, group, &inobm_blk);

	return inobm_blk;
}

int sba_ext3_get_inode_bitmap_blk(long sector)
{
	int group = SBA_GP_NO(sector);
	return sba_ext3_group_2_inode_bitmap(group);
}

/*
 * Returns the byte offset of the inode
 * block in the inode bitmap.
 */

int sba_ext3_get_inode_bitmap_offset(long sector)
{
	int group = SBA_GP_NO(sector);
	int blk = SBA_SECTOR_TO_BLOCK(sector);
	int inode_start;

	/*128 because each grp desc is of size 32 bytes*/
	if (group < 128) {
		if (ht_lookup_val(h_ext3_inode_table_start, group, &inode_start)) {
		}
		else {
			sba_debug(1, "Error: unable to find the inode table start for grp# %d\n", group);
			return -1;
		}
	}
	else {
		sba_debug(1, "Error: grp greater than 128\n");
		return -1;
	}

	if (!((blk >= inode_start) && (blk < inode_start+ sba_ext3_inode_blks_per_group))) {
		sba_debug(1, "Error: Invalid inode block %d specified", blk);
		return -1;
  	}

	/* 32 bitmap bits (4 bytes) for every inode block */
	return (blk - inode_start)*4; 
}

/* 
 * given a group, this method returns the block number of 
 * the data bitmap block for that group
 */
int sba_ext3_group_2_data_bitmap(int group)
{
	int databm_blk = -1;
	ht_lookup_val(h_ext3_data_bitmap, group, &databm_blk);

	return databm_blk;
}

int sba_ext3_get_data_bitmap_blk(long sector)
{
	int group = SBA_GP_NO(sector);
	return sba_ext3_group_2_data_bitmap(group);
}

int sba_ext3_data_bitmap_block(long sector)
{
	int blk = SBA_SECTOR_TO_BLOCK(sector);

	if (blk == sba_ext3_get_data_bitmap_blk(sector)) {
		return 1;
	}
	return -1;
}

int sba_ext3_inode_bitmap_block(long sector)
{
	int blk = SBA_SECTOR_TO_BLOCK(sector);

	if (blk == sba_ext3_get_inode_bitmap_blk(sector)) {
		return 1;
  	}
	return -1;
}

int sba_ext3_bitmap_block(long sector)
{
	int blk = SBA_SECTOR_TO_BLOCK(sector);

	if (blk == sba_ext3_get_data_bitmap_blk(sector)) {
		return 1;
	}
	if (blk == sba_ext3_get_inode_bitmap_blk(sector)) {
		return 1;
  	}
	return -1;
}

int sba_ext3_super_block(long sector, int size)
{
	int blk = SBA_SECTOR_TO_BLOCK(sector);

	sba_debug(0, "sector = %ld, blk = %d size = %d\n", sector, blk, size);

	if (blk == 0) {
		sba_debug(0, "This is an ext3 super block\n");
		return 1;
	}
	return -1;
}
 
int sba_ext3_group_desc_block(long sector)
{
	int group;
	int blk;

    group = SBA_GP_NO(sector);
    blk = SBA_SECTOR_TO_BLOCK(sector);

	if (group < 4096/sizeof(struct ext3_group_desc)) {
		/*grp desc is in block 1*/
		if (blk == 1) {
			return 1;
		}
	}
	else {
		/*we don't handle this case*/
		sba_debug(1, "ERROR: We cannot handle FS with more than 128 groups for now\n");
		sba_debug(1, "ERROR: But we got %d groups. Fix this problem and continue\n", group);
	}

	return -1;
}

int sba_ext3_journal_request(struct bio *sba_bio)
{
	if (jour_dev) {
		if (iminor(sba_bio->bi_bdev->bd_inode) == JOUR_MINOR) {
			return 1;
		}
	}

	return 0;
}

int sba_ext3_journal_block(struct bio *sba_bio, sector_t sector)
{
	if (jour_dev) {
		if (sba_ext3_journal_request(sba_bio)) {
			int blk = SBA_SECTOR_TO_BLOCK(sector);

			if (ht_lookup(h_sba_ext3_journal, blk)) {
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

		if (ht_lookup(h_sba_ext3_journal, blk)) {
			return 1;
		}
		else {
			return 0;
		}
	}
}

int sba_ext3_mkfs_write(struct bio *sba_bio)
{
	int blk = SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector);
	sba_debug(1, "Received WRITE for blk# %d\n", blk);

	/*if a separate device is used for the journal, then return*/
	if (jour_dev) {
		if (sba_ext3_journal_request(sba_bio)) {
			ht_add(h_sba_ext3_journal, blk);
		}	
	}
	else {
		/*don't have to cache anything. the new code reads the blocks
		 *of the journal and builds the h_sba_ext3_journal*/
		sba_debug(0, "Not adding %d to h_sba_ext3_journal\n", blk);
	}

	return 1;
}

int sba_ext3_print_journal()
{
	ht_print(h_sba_ext3_journal);
	return 1;
}

int sba_ext3_start()
{
	char *data;
	int sb_block = 1; /*copied from ext3_read_super*/
	int blocksize = 4096;

	sba_ext3_find_journal_entries();

	/*initialize the sba_ext3_inode_blks_per_group*/
	if ((data = read_block(0)) != NULL) {
		struct ext3_super_block *es;
		int offset = (sb_block * EXT3_MIN_BLOCK_SIZE) % blocksize;

		es = (struct ext3_super_block *)(data + offset);

		if (es->s_magic == EXT3_SUPER_MAGIC) {
			sba_debug(1, "Correctly identified ext3 super block\n");

			sba_ext3_inode_blks_per_group = (es->s_inodes_per_group * EXT3_GOOD_OLD_INODE_SIZE)/blocksize;
			sba_debug(1, "inodes per gp = %d\n", es->s_inodes_per_group);
			sba_debug(1, "inode blks per gp = %d\n", sba_ext3_inode_blks_per_group);
		}
		else {
			sba_debug(1, "Error: ext3 super block magic number does not match\n");
		}
		
		free_page((int)data);
	}
	else {
		sba_debug(1, "Error: unable to read the ext3 super block\n");
	}

	if ((data = read_block(1)) != NULL) {
		int i;
		struct ext3_group_desc *gd;
	
		for (i = 0; i < 128; i ++) {
			gd = ((struct ext3_group_desc *)data) + i;

			if (gd->bg_block_bitmap == 0) {
				break;
			}

			sba_debug(1, "GDesc %d: blockbitmap = %d, inodebitmap = %d, inodetable = %d\n", 
			i, gd->bg_block_bitmap, gd->bg_inode_bitmap, gd->bg_inode_table);

			ht_add_val(h_ext3_inode_table_start, i, gd->bg_inode_table);
			ht_add_val(h_ext3_inode_bitmap, i, gd->bg_inode_bitmap);
			ht_add_val(h_ext3_data_bitmap, i, gd->bg_block_bitmap);
		}
		free_page((int)data);
	}
	else {
		sba_debug(1, "Error: unable to read the ext3 grp desc\n");
	}


	return 1;
}

/*we read the journal blocks and build the h_sba_ext3_journal*/
int sba_ext3_find_journal_entries()
{
	char *data;
	
	if ((data = read_block(JOURNAL_INODE_BLK)) != NULL) {
		char *data2;
		char *data3;
		char *data4;
		char *buf;
		int i;
		struct ext3_inode *inode;

		buf = data + JOURNAL_INODE_NO*sizeof(struct ext3_inode);
		inode = (struct ext3_inode*)buf;

		for (i = 0; i < 15; i ++) {
			if (inode->i_block[i]) {

				if (i == 0) {
					sba_debug(1, "First journal block = %d\n", inode->i_block[i]);
				}

				if (!ht_lookup(h_sba_ext3_journal, inode->i_block[i])) {
					ht_add(h_sba_ext3_journal, inode->i_block[i]);
				}
				else {
					sba_debug(1, "Error: duplicate entry %d in h_sba_ext3_journal\n", inode->i_block[i]);
					return -1;
				}
			}
		}

		//separately add the indir blocks
		for (i = 12; i < 15; i ++) {
			if (inode->i_block[i]) {
				ht_add(h_ext3_journal_indir_blocks, inode->i_block[i]);
			}
		}


		//adding the values from in-direct indir pointer
		if ((data2 = read_block(inode->i_block[12])) != NULL) {
			int *blkno = (int *)data2;
			
			for(i = 0; i< 1024; i++) {
				int blk = *(int *)(blkno + i);

				if (blk) {
					ht_add(h_sba_ext3_journal, blk);
				}	
			}

			free_page((int)data2);
		}
		else {
			sba_debug(1, "Error: unable to read the journal indir block\n");
			return -1;
		}

		if (inode->i_block[13]) {
			//adding the values from double in-direct pointer
			if ((data3 = read_block(inode->i_block[13])) != NULL) {
				int *blkno1 = (int *)data3;
				
				for(i = 0; i< 1024; i++) {
					int blk1 = *(int *)(blkno1 + i);

					if (blk1) {

						//separately add the indir blocks
						if (inode->i_block[i]) {
							ht_add(h_ext3_journal_indir_blocks, blk1);
						}

						ht_add(h_sba_ext3_journal, blk1);

						if (blk1) {
							//adding values from single indir pointers
							if ((data4 = read_block(blk1)) != NULL) {
								int j;
								int *blkno2 = (int *)data4;

								for(j = 0; j< 1024; j++) {

									int blk2 = *(int *)(blkno2 + j);

									if (blk2) {
										ht_add(h_sba_ext3_journal, blk2);
									}	
								}

								free_page((int)data4);
							}
							else {
								sba_debug(1, "Error: unable to read the journal indir block %d\n", blk1);
								return -1;
							}
						}
					}
				}

				free_page((int)data3);
			}
			else {
				sba_debug(1, "Error: unable to read the journal indir block %d\n",inode->i_block[13]);
				return -1;
			}
		}

		free_page((int)data);
	}
	else {
		sba_debug(1, "Error: unable to find the journal inode block\n");
		sba_debug(1, "Setting journal begin and end to -1\n");
	}

	return 1;
}

int sba_ext3_insert_journaled_blocks(int key, int blocknr)
{
	if (ht_lookup(h_ext3_journaled_blocks, key)) {
		sba_debug(1, "Key %d is being journaled again\n", key);
		return -1;
	}

	ht_add_val(h_ext3_journaled_blocks, key, blocknr);
	return 1;
}

int sba_ext3_remove_journaled_blocks(int key)
{
	int blocknr = -1;

	if (ht_lookup_val(h_ext3_journaled_blocks, key, &blocknr)) {
		if (ht_remove(h_ext3_journaled_blocks, key) < 0) {
			sba_debug(1, "Error removing the key %d\n", key);
			return blocknr;
		}
	}

	return blocknr;
}

int sba_ext3_journaled_block(int blocknr)
{
	if (ht_lookup(h_ext3_journaled_blocks, blocknr)) {
		return 1;
	}

	return 0;
}

int sba_ext3_handle_descriptor_block(char *data, sector_t sector, int rw)
{
	int i = 0;
	int desc_blknr = SBA_SECTOR_TO_BLOCK(sector);
	unsigned long blocknr;
	char *tagp = NULL;
	journal_block_tag_t *tag = NULL;

	/*add all the tags to the hash table*/
	tagp = &data[sizeof(journal_header_t)];
	while ((tagp - data + sizeof(journal_block_tag_t)) <= SBA_BLKSIZE) {

		tag = (journal_block_tag_t *) tagp;
		blocknr = ntohl(tag->t_blocknr);

		i++;

		sba_debug(0, "Descriptor tag: block number = %ld\n", blocknr);

		if (sba_ext3_insert_journaled_blocks(desc_blknr + i, blocknr) == -1) {
			sba_debug(1, "Error: inserting blk=%ld into journaled blocks list\n", blocknr);
		}

		sba_common_add_desc_stats(blocknr);

		/*go to the next tag*/
		tagp += sizeof(journal_block_tag_t);

		/* If the flags doesn't have the same uuid
		 * skip to the next tag. */
		if (!(tag->t_flags & htonl(JFS_FLAG_SAME_UUID))) {
			tagp += 16;
		}		

		if (tag->t_flags & htonl(JFS_FLAG_LAST_TAG)) {
			break;
		}
	}

	return 1;
}

int sba_ext3_journal_indir_block(sector_t sector)
{
	int blocknr = SBA_SECTOR_TO_BLOCK(sector);

	if (ht_lookup(h_ext3_journal_indir_blocks, blocknr)) {
		return 1;
	}
	
	return -1;

}

int sba_ext3_indir_block(int sector)
{
	int blocknr = SBA_SECTOR_TO_BLOCK(sector);

	if (ht_lookup(h_ext3_indir_blocks, blocknr)) {
		return 1;
	}
	
	return -1;
}

int sba_ext3_dir_block(int sector)
{
	int blocknr = SBA_SECTOR_TO_BLOCK(sector);

	if (ht_lookup(h_ext3_dir_blocks, blocknr)) {
		return 1;
	}
	
	return -1;
}

int sba_ext3_unjournaled_block_type(int blocknr)
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


int sba_ext3_non_journal_block_type(long sector, char *type, int size)
{
	int ret;

    if (!type) {
        ret = UNKNOWN_BLOCK;
    }
    else
    if (sba_ext3_inode_block(sector) >= 0) {
        ret = SBA_EXT3_INODE;
    }
    else
    if (sba_ext3_super_block(sector, size) >= 0) {
        ret =  SBA_EXT3_SUPER;
    }
    else
    if (sba_ext3_inode_bitmap_block(sector) >= 0) {
        ret =  SBA_EXT3_IBITMAP;
    }
    else
    if (sba_ext3_data_bitmap_block(sector) >= 0) {
        ret =  SBA_EXT3_DBITMAP;
    }
    else
    if (sba_ext3_group_desc_block(sector) >= 0) {
        ret =  SBA_EXT3_GROUP;
    }
    else
    if (sba_ext3_dir_block(sector) >= 0) {
        ret =  SBA_EXT3_DIR;
    }
    else
    if (sba_ext3_indir_block(sector) >= 0) {
        ret =  SBA_EXT3_INDIR;
    }
	else {
		ret =  SBA_EXT3_DATA;
	}

	strcpy(type, sba_ext3_get_block_type_str(ret));
	return ret;
}

int sba_ext3_block_type(char *data, sector_t sector, char *type, struct bio *sba_bio)
{
	journal_header_t *header = NULL;
	int ret = UNKNOWN_BLOCK;

	if (sba_ext3_journal_block(sba_bio, sector)) {

		/*before checking other things, first see if this is 
		 *the journal indir blocks. if so, return*/
		if (sba_ext3_journal_indir_block(sector) >= 0) {
			ret = SBA_EXT3_JINDIR;
			strcpy(type, sba_ext3_get_block_type_str(ret));
		}
		else {
			header = (journal_header_t *)data;

			if (header->h_magic == htonl(JFS_MAGIC_NUMBER)) {
				int desc_btype = ntohl(header->h_blocktype);

				switch(desc_btype) {
					case JFS_REVOKE_BLOCK:
						ret = SBA_EXT3_REVOKE;
						strcpy(type, sba_ext3_get_block_type_str(ret));

						{
							journal_revoke_header_t *header;
							int offset, max;

							header = (journal_revoke_header_t *) data;
							offset = sizeof(journal_revoke_header_t);
							max = ntohl(header->r_count);

							while (offset < max) {
								unsigned long blocknr;

								blocknr = ntohl(* ((unsigned int *) (data+offset)));
								offset += 4;
								sba_debug(1, "Revoke block# %ld\n", blocknr);		
							}
						}					
						break;

					case JFS_DESCRIPTOR_BLOCK:
						ret = SBA_EXT3_DESC;
						strcpy(type, sba_ext3_get_block_type_str(ret));
						
						/* 
						 * Find out the real block numbers from this desc block.
						 * sba_ext3_handle_descriptor_block() will also add the 
						 * real blocknr to a hash_table so that when the checkpt
						 * write comes, it can be identified properly.
						 */
						sba_debug(0, "Block %ld is a journal desc block\n", SBA_SECTOR_TO_BLOCK(sector));
						sba_ext3_handle_descriptor_block(data, sector, SBA_WRITE);

						break;

					case JFS_COMMIT_BLOCK:
						ret = SBA_EXT3_COMMIT;
						strcpy(type, sba_ext3_get_block_type_str(ret));

						break;

					case JFS_SUPERBLOCK_V1:
					case JFS_SUPERBLOCK_V2:
						ret = SBA_EXT3_JSUPER;
						strcpy(type, sba_ext3_get_block_type_str(ret));

						break;

					default:
						{
						int ref_blocknr = -1;

						ret = SBA_EXT3_JDATA;
						strcpy(type, sba_ext3_get_block_type_str(ret));

						ref_blocknr = sba_ext3_remove_journaled_blocks(SBA_SECTOR_TO_BLOCK(sector));
						sba_debug(1, "%d is %ldth journaled block\n", ref_blocknr, SBA_SECTOR_TO_BLOCK(sector));
						}
				}
			}
			else {
				int ref_blocknr = -1;

				ret = SBA_EXT3_JDATA;
				strcpy(type, sba_ext3_get_block_type_str(ret));

				ref_blocknr = sba_ext3_remove_journaled_blocks(SBA_SECTOR_TO_BLOCK(sector));
				sba_debug(0, "%d is %ldth journaled block\n", ref_blocknr, SBA_SECTOR_TO_BLOCK(sector));
			}
		}
	}
	else {
		ret = sba_ext3_non_journal_block_type(sector, type, bio_sectors(sba_bio)*SBA_HARDSECT);
	}

	sba_debug(0, "returning block type %d\n", ret);
	return ret;
}

/* input is the inodenr of the file. we read the inode 
 * and find the indir blocks that are allocated to the file*/
int sba_ext3_init_indir_blocks(unsigned long inodenr)
{
	/*we check if this is the inode block with the specified inode*/
	int blocknr, offset;
	char *data;
	struct ext3_inode *ei;

	if (inodenr > 0) {

		sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);

		if ((data = read_block(blocknr)) != NULL) {
			int blocks;

			offset *= sizeof(struct ext3_inode);

			ei = (struct ext3_inode *)(data + offset);
			blocks = (ei->i_size%4096) ? (ei->i_size/4096 + 1) : (ei->i_size/4096);

			sba_debug(1, "Number of blocks in file = %d\n", blocks);

			if (ei->i_block[EXT3_IND_BLOCK]) {
				sba_debug(1, "Single indir block = %d\n", ei->i_block[EXT3_IND_BLOCK]);
				ht_add_val(h_ext3_indir_blocks, ei->i_block[EXT3_IND_BLOCK], inodenr);
			}

			if (ei->i_block[EXT3_DIND_BLOCK]) {
				sba_debug(1, "Double indir block = %d\n", ei->i_block[EXT3_DIND_BLOCK]);
				ht_add_val(h_ext3_indir_blocks, ei->i_block[EXT3_DIND_BLOCK], inodenr);
			}

			if (ei->i_block[EXT3_TIND_BLOCK]) {
				sba_debug(1, "Triple indir block = %d\n", ei->i_block[EXT3_TIND_BLOCK]);
				ht_add_val(h_ext3_indir_blocks, ei->i_block[EXT3_TIND_BLOCK], inodenr);
			}

			free_page((int)data);
		}
	}

	return 1;
}

/* input is the inodenr of the dir block. we read the inode 
 * and find the blocks that are allocated to the dir */
int sba_ext3_init_dir_blocks(unsigned long inodenr)
{
	/*we check if this is the inode block with the specified inode*/
	int blocknr, offset;
	char *data;
	struct ext3_inode *ei;
	int i;

	if (inodenr > 0) {

		sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);

		if ((data = read_block(blocknr)) != NULL) {
			int blocks;

			offset *= sizeof(struct ext3_inode);

			ei = (struct ext3_inode *)(data + offset);
			blocks = (ei->i_size%4096) ? (ei->i_size/4096 + 1) : (ei->i_size/4096);

			sba_debug(0, "Size of dir (blk %d) = %d\n", blocknr, ei->i_size);
			sba_debug(0, "Blks of dir (blk %d) = %d\n", blocknr, ei->i_blocks);
			sba_debug(0, "Number of blocks in dir (blk %d) = %d\n", blocknr, blocks);

			if (blocks > EXT3_N_BLOCKS) {
				sba_debug(1, "Error: we dont handle large dirs for now\n");
				return -1;
			}

			for (i = 0; i < blocks; i ++) {
				sba_debug(1, "Adding block %d as the dir data block\n", ei->i_block[i]);
				ht_add_val(h_ext3_dir_blocks, ei->i_block[i], inodenr);
			}

			free_page((int)data);
		}
	}

	return 1;
}

/* 
 * when this function is called, the block type of the fault has already 
 * been matched by the sba_common. so, we need not again check for block
 * type match. we need to look for specific matches that are file system
 * specific. for eg., if an inode number is specified in the fault specification 
 * then we have to see if the inode block that we got here has the inode
 * that is specified in the fault
 */
int sba_ext3_fault_match(char *data, sector_t sector, fault *sba_fault)
{
	sba_debug(0, "Block type of %ld for fault match is %s\n", 
	SBA_SECTOR_TO_BLOCK(sector), sba_ext3_get_block_type_str(sba_fault->blk_type));

	switch(sba_fault->blk_type) {
		case SBA_EXT3_SUPER:
		case SBA_EXT3_GROUP:
			return 1;
		break;
		
		case SBA_EXT3_JSUPER:
			#if 0
			{
				journal_superblock_t *ext3_jsb;
				ext3_jsb = (journal_superblock_t *)data;
				sba_debug(1, "Value of start = %d\n", ext3_jsb->s_start);
				
				/* 
				 * if this is non-zero value, then ext3 fs is setting 
				 * the journal to be unclean. we can allow this write 
				 * to go and fail the writes where ext3 is setting the 
				 * journal to be unclean, so that we can force ext3 to 
				 * take recovery
				 */
				if (ext3_jsb->s_start) {
					//return 0; //allow this write to go !
				}
			}
			#endif

			return 1;
		break;
		
		case SBA_EXT3_INODE:
		{
			/*we check if this is the inode block with the specified inode*/
			int blocknr, offset;
			int inodenr = sba_fault->spec.ext3.inodenr;

			if (inodenr > 0) {

				sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);

				if (sector != SBA_BLOCK_TO_SECTOR(blocknr)) {
					return 0;
				}
				else {
					return 1;
				}
			}
			else {
				return 1;
			}

		}
		break;

		case SBA_EXT3_REVOKE:
		case SBA_EXT3_COMMIT:
		case SBA_EXT3_DESC:
		case SBA_EXT3_DBITMAP:
		case SBA_EXT3_IBITMAP:
		case SBA_EXT3_DATA:
		case SBA_EXT3_JDATA:
		case SBA_EXT3_DIR:
		case SBA_EXT3_INDIR:
			if (sba_fault->blocknr != -1) {
				if (SBA_SECTOR_TO_BLOCK(sector) == sba_fault->blocknr) {
					sba_debug(0, "Returning one (fault blk# %d, actual blocknr %ld\n", sba_fault->blocknr, SBA_SECTOR_TO_BLOCK(sector));
					return 1;
				}
				else {
					sba_debug(0, "Returning zero (fault blk# %d, actual blocknr %ld\n", sba_fault->blocknr, SBA_SECTOR_TO_BLOCK(sector));
					return 0;
				}
			}
			else {
				return 1;
			}
		break;
		
		default:
			sba_debug(1, "Returning zero\n");
			return 0;
	}
	
	sba_debug(1, "Returning zero\n");
	return 0;
}

int sba_ext3_process_fault(fault *sba_fault)
{
	switch(sba_fault->blk_type) {
		case SBA_EXT3_DESC:
		case SBA_EXT3_REVOKE:
		case SBA_EXT3_COMMIT:
		case SBA_EXT3_SUPER:
		case SBA_EXT3_GROUP:
		case SBA_EXT3_JSUPER:
		case SBA_EXT3_DBITMAP:
		case SBA_EXT3_IBITMAP:
		case SBA_EXT3_JDATA:
			/* currently we dont have anything to process for these faults */
		break;
		
		case SBA_EXT3_INODE:
			if (sba_fault->spec.ext3.inodenr > 0) {
				int blocknr, offset;
				int inodenr = sba_fault->spec.ext3.inodenr;

				sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);
				sba_fault->blocknr = blocknr;
				sba_debug(1, "Inode block to be failed = %d\n", sba_fault->blocknr);
			}
		break;

		case SBA_EXT3_DATA:
			if (sba_fault->spec.ext3.inodenr > 0) {
				char *data;
				int blocknr, offset;
				struct ext3_inode *ei;
				int log_blknr;
				int inodenr = sba_fault->spec.ext3.inodenr;

				sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);

				/*initialize the sba_ext3_inode_blks_per_group*/
				if ((data = read_block(blocknr)) != NULL) {

					offset *= sizeof(struct ext3_inode);

					ei = (struct ext3_inode *)(data + offset);
					sba_debug(1, "Size of the file = %d\n", ei->i_size);

					log_blknr = sba_fault->spec.ext3.logical_blocknr;

					if ((log_blknr >= 0) && (log_blknr < EXT3_N_BLOCKS)) {
						sba_fault->blocknr = ei->i_block[log_blknr];
					}
					else {
						sba_fault->blocknr = ei->i_block[0];
					}

					sba_debug(1, "Data block to be failed = %d\n", sba_fault->blocknr);

					free_page((int)data);
				}
			}
			break;

		case SBA_EXT3_DIR:
			if (sba_fault->spec.ext3.inodenr > 0) {
				char *data;
				int blocknr, offset;
				struct ext3_inode *ei;
				int inodenr = sba_fault->spec.ext3.inodenr;

				sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);

				/*initialize the sba_ext3_inode_blks_per_group*/
				if ((data = read_block(blocknr)) != NULL) {

					offset *= sizeof(struct ext3_inode);

					ei = (struct ext3_inode *)(data + offset);
					sba_debug(1, "Size of the dir = %d\n", ei->i_size);

					sba_fault->blocknr = ei->i_block[0];

					free_page((int)data);
				}

				/*Also initialize all the dir blocks*/
				sba_ext3_init_dir_blocks(inodenr);
			}
		break;

		case SBA_EXT3_INDIR:
			if (sba_fault->spec.ext3.inodenr > 0) {
				char *data;
				int blocknr, offset;
				struct ext3_inode *ei;
				int inodenr = sba_fault->spec.ext3.inodenr;

				sba_ext3_inodenr_2_blocknr(inodenr, &blocknr, &offset);

				/*initialize the sba_ext3_inode_blks_per_group*/
				if ((data = read_block(blocknr)) != NULL) {

					offset *= sizeof(struct ext3_inode);

					ei = (struct ext3_inode *)(data + offset);
					sba_debug(1, "Size of the file = %d\n", ei->i_size);

					switch(sba_fault->spec.ext3.indir_blk) {
						case SBA_EXT3_SINDIR:
							sba_fault->blocknr = ei->i_block[EXT3_IND_BLOCK];
						break;

						case SBA_EXT3_DINDIR:
							sba_fault->blocknr = ei->i_block[EXT3_DIND_BLOCK];
						break;

						case SBA_EXT3_TINDIR:
							sba_fault->blocknr = ei->i_block[EXT3_TIND_BLOCK];
						break;

						default:
							sba_fault->blocknr = -1;
					}

					free_page((int)data);
				}

				/*Also initialize all the indir blocks*/
				sba_ext3_init_indir_blocks(inodenr);
			}
		break;
	}

	return 1;
}
