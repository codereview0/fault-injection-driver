#ifndef __INCLUDE_SBA_COMMON_H__
#define __INCLUDE_SBA_COMMON_H__

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/completion.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/timer.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include "sba_common_defs.h"
#include "sba_common_model.h"

#ifdef INC_EXT3
#include "sba_ext3.h"
#endif

#ifdef INC_REISERFS
#include "sba_reiserfs.h"
#endif

#ifdef INC_JFS
#include "sba_jfs.h"
#endif

/*
 * This structure stores the trace information
 */
typedef struct _stat_info{
	char btype[6];
	long blocknr;
	long ref_blocknr;
	struct timeval stv;
	struct timeval etv;
	int rw;
	struct _stat_info *prev;
	struct _stat_info *next;
} stat_info;

/*
 * This structure is stored as private in buffer heads
 * and passed to sba_end_io.
 */
typedef struct _sba_request {
	/*the original bio from the fs*/
	struct bio *sba_bio;

	/*nature of req*/
	int rw;

	/* this is an array of records because the sba_bio 
	 * can hold more than one block each of diff type*/
	stat_info **record;

	/*number of records*/
	int count; 
} sba_request;


typedef void (*bh_endio_t)(struct bio *sba_bio, int uptodate);

long long sba_common_diff_time(struct timeval st, struct timeval et);
int sba_common_journal_block(struct bio *sba_bio, sector_t sector);
int sba_common_build_writeback_journaling_model(void);
int sba_common_build_ordered_journaling_model(void);
int sba_common_build_data_journaling_model(void);
int sba_common_build_model(void);
int sba_common_destroy_model(void);
int sba_common_print_model(void);
int sba_common_init(void);
int sba_common_cleanup(void);
int sba_common_zero_stat(sba_stat *ss);
int sba_common_print_stat(sba_stat *ss);
int sba_common_print_journal(void);
int sba_common_handle_mkfs_write(int sector);
struct bio *sba_common_alloc_and_init_bio(struct block_device *dev, int block, bio_end_io_t end_io_func, int rw);
struct bio *alloc_bio_for_read(struct block_device *dev, int block, bio_end_io_t end_io_func);
char *read_block(int block);
int sba_common_init_indir_blocks(unsigned long inodenr);
int sba_common_init_dir_blocks(unsigned long inodenr);
int sba_common_process_fault(void);
int reinit_fault(fault *f);
int add_fault(fault *f);
//int sba_common_add_fault_correction(int blocknr, int offset, int size, void *original);
int remove_fault(int force);
char *sba_common_get_block_type_str(hash_table *h_btype, int sector);
int sba_common_print_fault(void);
int sba_common_fault_match(char *data, sector_t sector, struct bio *sba_bio, hash_table *h_this);
int sba_common_commit_block(hash_table *h_this, int sector);
int sba_common_inject_fault(struct bio *sba_bio, sba_request *sba_req, int *uptodate);
int sba_common_execute_fault(struct bio *sba_bio, int *uptodate, hash_table *h_this);
int sba_common_print_block(struct bio *sba_bio);
char *sba_common_get_btype_str(int btype);
int sba_common_move_to_start(void);
sba_state *sba_common_get_current_state(void);
int sba_common_get_block_type(hash_table *h_this, sector_t sector);
int sba_common_destroy_block_types_table(hash_table *h_this);
hash_table *sba_common_build_block_types_table(struct bio *sba_bio);
int sba_common_find_last_block_type(struct bio *sba_bio, hash_table *h_this);
int sba_common_edge_match(sba_state_input e1, sba_state_input e2);
int sba_common_move(int btype, int response);
int sba_common_is_valid_move(sba_state *s, int btype);
int sba_common_model_checker(struct bio *sba_bio, hash_table *h_this);
int sba_common_print_all_blocks(struct bio *sba_bio);
int sba_common_report_error(struct bio *sba_bio, hash_table *h_this);
int sba_common_print_journaled_blocks(void);
int sba_common_add_desc_stats(int blocknr);
int sba_common_add_workload_end(void);
int sba_common_add_workload_start(void);
int sba_common_add_crash_stats(void);
int sba_common_add_fault_injection_stats(hash_table *h_this, int sector);
int sba_common_add_stats(stat_info *si, stat_info **list);
int sba_common_get_start_timestamp(sba_request *sba_req);
int sba_common_get_end_timestamp(sba_request *sba_req);
int sba_common_collect_stats(sba_request *sba_req, hash_table *h_this);
int sba_common_clean_stats(void);
int sba_common_clean_all_stats(void);
int my_div(int a, int b);
int my_mod(int a, int b);
int sba_common_extract_stats(char *ubuf, struct timeval start_time);

#endif
