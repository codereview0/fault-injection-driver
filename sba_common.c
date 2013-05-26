/*
 * This file contains the common sba routines
 * 
 * IMPORTANT: Note that we have to call the sba_ext3_block_type() function only once.
 * The reason is that the state that is used to find the block type of a dynamically
 * typed block is destroyed once its type is asked for. Therefore, we maintain a 
 * hash table h_block_types in which we store the block types of the blocks.
 */

#include "sba_common.h"

/*holds the statistics*/
sba_stat ss;

/*to emulate system crash*/
int crash_system = 0;

/*to crash a system after commit and before checkpointing to initiate recovery*/
int crash_after_commit = 0;


/*used to limit one fault at a time*/
int fault_on_queue = 0;

/*type of file system*/
#ifdef INC_EXT3
	int filesystem = EXT3;
#endif
#ifdef INC_REISERFS
	int filesystem = REISERFS;
#endif
#ifdef INC_JFS
	int filesystem = JFS;
#endif

/*this holds the fault*/
fault *sba_fault = NULL;

/*for the f_dev structure*/
sba_dev sba_device;

/*Is journal in a separate device ?*/
int jour_dev = SAME_DEV;

/*the journaling mode under which we work*/
//int journaling_mode = DATA_JOURNALING;
int journaling_mode = ORDERED_JOURNALING;
//int journaling_mode = WRITEBACK_JOURNALING;

/* this global value represents the current state of the system */
journaling_model *sba_current_model = NULL;

static bio_end_io_t sba_common_end_io;

/*to control addition and removal of statistics record*/
stat_info *stat_list;
spinlock_t stat_lock;

/*this flag indicates if the fault has been successfully injected*/
int fault_injected = 0;

/*statistics will be collected by a series of calls. 
 *prev_count stores the amount of data copied on the
 *previous call*/
int prev_count = 0;

/*--------------------------------------------------------------------------*/

/* Return time diff in micro seconds */
long long sba_common_diff_time(struct timeval st, struct timeval et)
{
	long long diff;

	if (et.tv_usec < st.tv_usec) {
		et.tv_usec += 1000000;
		et.tv_sec --;
	}

	diff = (et.tv_sec - st.tv_sec)*1000000 + (et.tv_usec - st.tv_usec);
	return diff;
}

int sba_common_journal_block(struct bio *sba_bio, sector_t sector)
{
	switch(filesystem) {
		#ifdef INC_EXT3
		case EXT3:
			return sba_ext3_journal_block(sba_bio, sector);
		break;
		#endif

		#ifdef INC_REISERFS
		case REISERFS:
			return sba_reiserfs_journal_block(sba_bio, sector);
		break;
		#endif

		#ifdef INC_JFS
		case JFS:
			return sba_jfs_journal_block(sba_bio, sector);
		break;
		#endif

		default: return 0;
	}
}

int sba_common_build_writeback_journaling_model(void)
{
	/* states for writeback journaling 
	 * 
	 * Note 1: For now, we're not concerned about bad block remapping.
	 * It is much more complex and requires some more thinking.
	 * 
	 * Note 2: S2 is the final state and S3 represents aborted state
	 * 
	 * Note 3: J represents the journal block. It includes journal descriptor,
	 * journal revoke and journal data blocks. U means unordered blocks.
	 * 
	 * Note 4: F represents the write failure. 
	 * 
	 * 
     *        J    U    C    K    S    F
     *-----------------------------------
	 *  S0    S1   S0                  S3
	 *  S1    S1   S1   S2             S3
	 *  S2    S1   S2        S2   S2   S3
	 *  S3                          
	 * 
	 */
	
	journaling_model *writeback_journaling = NULL;
	sba_state **wbackj = NULL;
	sba_edge *s0_s1_jdesc, *s0_s1_jrevk, *s0_s1_jdata, *s0_s0_unord, *s0_s3_fail;
	sba_edge *s1_s1_jdesc, *s1_s1_jrevk, *s1_s1_jdata, *s1_s1_unord, *s1_s2_commit, *s1_s3_fail;
	sba_edge *s2_s2_chkpt, *s2_s2_jsuper, *s2_s2_unord, *s2_s1_jdesc, *s2_s1_jrevk, *s2_s1_jdata, *s2_s3_fail;
	int wbackj_states = 4;
	int i = 0;

	writeback_journaling = kmalloc(sizeof(journaling_model), GFP_KERNEL);
	if (!writeback_journaling) {
		goto ret_err;
	}

	writeback_journaling->mode = WRITEBACK_JOURNALING;
	writeback_journaling->total_states = wbackj_states;

	wbackj = kmalloc(sizeof(sba_state *)*wbackj_states, GFP_KERNEL);
	if (!wbackj) {
		goto ret_err;
	}

	for (i = 0; i < wbackj_states; i ++) {
		wbackj[i] = kmalloc(sizeof(sba_state), GFP_KERNEL);
		if (!wbackj[i]) {
			goto ret_err;
		}

		sprintf(wbackj[i]->name, "S%d", i);
		ht_create(&wbackj[i]->h_out_edges, "outedges");
	}

	/* -------------------------------------------------------------- */
	/*initialize edge s0-s1*/
	s0_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jdesc) {
		goto ret_err;
	}
	s0_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s0_s1_jdesc->input.response = WRITE_SUCCESS;
	s0_s1_jdesc->dest = wbackj[1];

	s0_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jrevk) {
		goto ret_err;
	}
	s0_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s0_s1_jrevk->input.response = WRITE_SUCCESS;
	s0_s1_jrevk->dest = wbackj[1];

	s0_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jdata) {
		goto ret_err;
	}
	s0_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s0_s1_jdata->input.response = WRITE_SUCCESS;
	s0_s1_jdata->dest = wbackj[1];

	/*initialize edge s0-s0*/
	s0_s0_unord = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s0_unord) {
		goto ret_err;
	}
	s0_s0_unord->input.block_type = UNORDERED_BLOCK;
	s0_s0_unord->input.response = WRITE_SUCCESS;
	s0_s0_unord->dest = wbackj[0];
	
	/*initialize edge s0-s3*/
	s0_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s3_fail) {
		goto ret_err;
	}
	s0_s3_fail->input.block_type = ANY_BLOCK;
	s0_s3_fail->input.response = WRITE_FAILURE;
	s0_s3_fail->dest = wbackj[3];
	
	/*initialize state s0*/
	wbackj[0]->outdegree = 5;
	ht_add_val(wbackj[0]->h_out_edges, 0, (int)s0_s1_jdesc);
	ht_add_val(wbackj[0]->h_out_edges, 1, (int)s0_s1_jrevk);
	ht_add_val(wbackj[0]->h_out_edges, 2, (int)s0_s1_jdata);
	ht_add_val(wbackj[0]->h_out_edges, 3, (int)s0_s0_unord);
	ht_add_val(wbackj[0]->h_out_edges, 4, (int)s0_s3_fail);
	
	/* -------------------------------------------------------------- */
	/*initialize edge s1-s1*/
	s1_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jdesc) {
		goto ret_err;
	}
	s1_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s1_s1_jdesc->input.response = WRITE_SUCCESS;
	s1_s1_jdesc->dest = wbackj[1];

	s1_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jrevk) {
		goto ret_err;
	}
	s1_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s1_s1_jrevk->input.response = WRITE_SUCCESS;
	s1_s1_jrevk->dest = wbackj[1];

	s1_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jdata) {
		goto ret_err;
	}
	s1_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s1_s1_jdata->input.response = WRITE_SUCCESS;
	s1_s1_jdata->dest = wbackj[1];

	s1_s1_unord = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_unord) {
		goto ret_err;
	}
	s1_s1_unord->input.block_type = UNORDERED_BLOCK;
	s1_s1_unord->input.response = WRITE_SUCCESS;
	s1_s1_unord->dest = wbackj[1];

	/*initialize edge s1-s2*/
	s1_s2_commit = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s2_commit) {
		goto ret_err;
	}
	s1_s2_commit->input.block_type = JOURNAL_COMMIT_BLOCK;
	s1_s2_commit->input.response = WRITE_SUCCESS;
	s1_s2_commit->dest = wbackj[2];

	/*initialize edge s1-s3*/
	s1_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s3_fail) {
		goto ret_err;
	}
	s1_s3_fail->input.block_type = ANY_BLOCK;
	s1_s3_fail->input.response = WRITE_FAILURE;
	s1_s3_fail->dest = wbackj[3];
	
	/*initialize state s1*/
	wbackj[1]->outdegree = 6;
	ht_add_val(wbackj[1]->h_out_edges, 0, (int)s1_s1_jdesc);
	ht_add_val(wbackj[1]->h_out_edges, 1, (int)s1_s1_jrevk);
	ht_add_val(wbackj[1]->h_out_edges, 2, (int)s1_s1_jdata);
	ht_add_val(wbackj[1]->h_out_edges, 3, (int)s1_s1_unord);
	ht_add_val(wbackj[1]->h_out_edges, 4, (int)s1_s2_commit);
	ht_add_val(wbackj[1]->h_out_edges, 5, (int)s1_s3_fail);

	/* -------------------------------------------------------------- */
	/*initialize edge s2-s2*/
	s2_s2_chkpt = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_chkpt) {
		goto ret_err;
	}
	s2_s2_chkpt->input.block_type = CHECKPOINT_BLOCK;
	s2_s2_chkpt->input.response = WRITE_SUCCESS;
	s2_s2_chkpt->dest = wbackj[2];

	s2_s2_jsuper = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_jsuper) {
		goto ret_err;
	}
	s2_s2_jsuper->input.block_type = JOURNAL_SUPER_BLOCK;
	s2_s2_jsuper->input.response = WRITE_SUCCESS;
	s2_s2_jsuper->dest = wbackj[2];

	s2_s2_unord = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_unord) {
		goto ret_err;
	}
	s2_s2_unord->input.block_type = UNORDERED_BLOCK;
	s2_s2_unord->input.response = WRITE_SUCCESS;
	s2_s2_unord->dest = wbackj[0];
	
	/*initialize edge s2-s1*/
	s2_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jdesc) {
		goto ret_err;
	}
	s2_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s2_s1_jdesc->input.response = WRITE_SUCCESS;
	s2_s1_jdesc->dest = wbackj[1];

	s2_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jrevk) {
		goto ret_err;
	}
	s2_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s2_s1_jrevk->input.response = WRITE_SUCCESS;
	s2_s1_jrevk->dest = wbackj[1];

	s2_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jdata) {
		goto ret_err;
	}
	s2_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s2_s1_jdata->input.response = WRITE_SUCCESS;
	s2_s1_jdata->dest = wbackj[1];

	/*initialize edge s2-s3*/
	s2_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s3_fail) {
		goto ret_err;
	}
	s2_s3_fail->input.block_type = ANY_BLOCK;
	s2_s3_fail->input.response = WRITE_FAILURE;
	s2_s3_fail->dest = wbackj[3];
	
	/*initialize state s2*/
	wbackj[2]->outdegree = 7;
	ht_add_val(wbackj[2]->h_out_edges, 0, (int)s2_s2_chkpt);
	ht_add_val(wbackj[2]->h_out_edges, 1, (int)s2_s2_jsuper);
	ht_add_val(wbackj[2]->h_out_edges, 2, (int)s2_s1_jdesc);
	ht_add_val(wbackj[2]->h_out_edges, 3, (int)s2_s1_jrevk);
	ht_add_val(wbackj[2]->h_out_edges, 4, (int)s2_s1_jdata);
	ht_add_val(wbackj[2]->h_out_edges, 5, (int)s2_s2_unord);
	ht_add_val(wbackj[2]->h_out_edges, 6, (int)s2_s3_fail);

	/* -------------------------------------------------------------- */
	/*initialize state s3*/
	wbackj[3]->outdegree = 0;

	/* -------------------------------------------------------------- */

	writeback_journaling->states = wbackj;
	writeback_journaling->current_state = wbackj[0];
	sba_current_model = writeback_journaling;

	return 1;

ret_err:
	sba_debug(1, "Error: unable to allocate memory ... also leaking memory !\n");

	return 0;
}


int sba_common_build_ordered_journaling_model(void)
{
	/* states for ordered journaling 
	 * 
	 * Note 1: For now, we're not concerned about bad block remapping.
	 * It is much more complex and requires some more thinking.
	 * 
	 * Note 2: S2 is the final state and S3 represents aborted state
	 * 
	 * Note 3: J represents the journal block. It includes journal descriptor,
	 * journal revoke and journal data blocks. O means ordered blocks.
	 * 
	 * Note 4: F represents the write failure. 
	 * 
	 * 
     *        J    O    C    K    S    F
     *-----------------------------------
	 *  S0    S1   S0                  S3
	 *  S1    S1   S1   S2             S3
	 *  S2    S1   S0        S2   S2   S3
	 *  S3                          
	 * 
	 */
	
	journaling_model *ordered_journaling = NULL;
	sba_state **ordj = NULL;
	sba_edge *s0_s1_jdesc, *s0_s1_jrevk, *s0_s1_jdata, *s0_s0_ord, *s0_s3_fail;
	sba_edge *s1_s1_jdesc, *s1_s1_jrevk, *s1_s1_jdata, *s1_s1_ord, *s1_s2_commit, *s1_s3_fail;
	sba_edge *s2_s2_chkpt, *s2_s2_jsuper, *s2_s0_ord, *s2_s1_jdesc, *s2_s1_jrevk, *s2_s1_jdata, *s2_s3_fail;
	int ordj_states = 4;
	int i = 0;

	ordered_journaling = kmalloc(sizeof(journaling_model), GFP_KERNEL);
	if (!ordered_journaling) {
		goto ret_err;
	}

	ordered_journaling->mode = ORDERED_JOURNALING;
	ordered_journaling->total_states = ordj_states;

	ordj = kmalloc(sizeof(sba_state *)*ordj_states, GFP_KERNEL);
	if (!ordj) {
		goto ret_err;
	}

	for (i = 0; i < ordj_states; i ++) {
		ordj[i] = kmalloc(sizeof(sba_state), GFP_KERNEL);
		if (!ordj[i]) {
			goto ret_err;
		}

		sprintf(ordj[i]->name, "S%d", i);
		ht_create(&ordj[i]->h_out_edges, "outedges");
	}

	/* -------------------------------------------------------------- */
	/*initialize edge s0-s1*/
	s0_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jdesc) {
		goto ret_err;
	}
	s0_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s0_s1_jdesc->input.response = WRITE_SUCCESS;
	s0_s1_jdesc->dest = ordj[1];

	s0_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jrevk) {
		goto ret_err;
	}
	s0_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s0_s1_jrevk->input.response = WRITE_SUCCESS;
	s0_s1_jrevk->dest = ordj[1];

	s0_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jdata) {
		goto ret_err;
	}
	s0_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s0_s1_jdata->input.response = WRITE_SUCCESS;
	s0_s1_jdata->dest = ordj[1];

	/*initialize edge s0-s0*/
	s0_s0_ord = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s0_ord) {
		goto ret_err;
	}
	s0_s0_ord->input.block_type = ORDERED_BLOCK;
	s0_s0_ord->input.response = WRITE_SUCCESS;
	s0_s0_ord->dest = ordj[0];
	
	/*initialize edge s0-s3*/
	s0_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s3_fail) {
		goto ret_err;
	}
	s0_s3_fail->input.block_type = ANY_BLOCK;
	s0_s3_fail->input.response = WRITE_FAILURE;
	s0_s3_fail->dest = ordj[3];
	
	/*initialize state s0*/
	ordj[0]->outdegree = 5;
	ht_add_val(ordj[0]->h_out_edges, 0, (int)s0_s1_jdesc);
	ht_add_val(ordj[0]->h_out_edges, 1, (int)s0_s1_jrevk);
	ht_add_val(ordj[0]->h_out_edges, 2, (int)s0_s1_jdata);
	ht_add_val(ordj[0]->h_out_edges, 3, (int)s0_s0_ord);
	ht_add_val(ordj[0]->h_out_edges, 4, (int)s0_s3_fail);
	
	/* -------------------------------------------------------------- */
	/*initialize edge s1-s1*/
	s1_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jdesc) {
		goto ret_err;
	}
	s1_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s1_s1_jdesc->input.response = WRITE_SUCCESS;
	s1_s1_jdesc->dest = ordj[1];

	s1_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jrevk) {
		goto ret_err;
	}
	s1_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s1_s1_jrevk->input.response = WRITE_SUCCESS;
	s1_s1_jrevk->dest = ordj[1];

	s1_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jdata) {
		goto ret_err;
	}
	s1_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s1_s1_jdata->input.response = WRITE_SUCCESS;
	s1_s1_jdata->dest = ordj[1];

	s1_s1_ord = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_ord) {
		goto ret_err;
	}
	s1_s1_ord->input.block_type = ORDERED_BLOCK;
	s1_s1_ord->input.response = WRITE_SUCCESS;
	s1_s1_ord->dest = ordj[1];

	/*initialize edge s1-s2*/
	s1_s2_commit = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s2_commit) {
		goto ret_err;
	}
	s1_s2_commit->input.block_type = JOURNAL_COMMIT_BLOCK;
	s1_s2_commit->input.response = WRITE_SUCCESS;
	s1_s2_commit->dest = ordj[2];

	/*initialize edge s1-s3*/
	s1_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s3_fail) {
		goto ret_err;
	}
	s1_s3_fail->input.block_type = ANY_BLOCK;
	s1_s3_fail->input.response = WRITE_FAILURE;
	s1_s3_fail->dest = ordj[3];
	
	/*initialize state s1*/
	ordj[1]->outdegree = 6;
	ht_add_val(ordj[1]->h_out_edges, 0, (int)s1_s1_jdesc);
	ht_add_val(ordj[1]->h_out_edges, 1, (int)s1_s1_jrevk);
	ht_add_val(ordj[1]->h_out_edges, 2, (int)s1_s1_jdata);
	ht_add_val(ordj[1]->h_out_edges, 3, (int)s1_s1_ord);
	ht_add_val(ordj[1]->h_out_edges, 4, (int)s1_s2_commit);
	ht_add_val(ordj[1]->h_out_edges, 5, (int)s1_s3_fail);

	/* -------------------------------------------------------------- */
	/*initialize edge s2-s2*/
	s2_s2_chkpt = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_chkpt) {
		goto ret_err;
	}
	s2_s2_chkpt->input.block_type = CHECKPOINT_BLOCK;
	s2_s2_chkpt->input.response = WRITE_SUCCESS;
	s2_s2_chkpt->dest = ordj[2];

	s2_s2_jsuper = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_jsuper) {
		goto ret_err;
	}
	s2_s2_jsuper->input.block_type = JOURNAL_SUPER_BLOCK;
	s2_s2_jsuper->input.response = WRITE_SUCCESS;
	s2_s2_jsuper->dest = ordj[2];

	/*initialize edge s2-s1*/
	s2_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jdesc) {
		goto ret_err;
	}
	s2_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s2_s1_jdesc->input.response = WRITE_SUCCESS;
	s2_s1_jdesc->dest = ordj[1];

	s2_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jrevk) {
		goto ret_err;
	}
	s2_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s2_s1_jrevk->input.response = WRITE_SUCCESS;
	s2_s1_jrevk->dest = ordj[1];

	s2_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jdata) {
		goto ret_err;
	}
	s2_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s2_s1_jdata->input.response = WRITE_SUCCESS;
	s2_s1_jdata->dest = ordj[1];

	/*initialize edge s2-s0*/
	s2_s0_ord = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s0_ord) {
		goto ret_err;
	}
	s2_s0_ord->input.block_type = ORDERED_BLOCK;
	s2_s0_ord->input.response = WRITE_SUCCESS;
	s2_s0_ord->dest = ordj[0];
	
	/*initialize edge s2-s3*/
	s2_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s3_fail) {
		goto ret_err;
	}
	s2_s3_fail->input.block_type = ANY_BLOCK;
	s2_s3_fail->input.response = WRITE_FAILURE;
	s2_s3_fail->dest = ordj[3];
	
	/*initialize state s2*/
	ordj[2]->outdegree = 7;
	ht_add_val(ordj[2]->h_out_edges, 0, (int)s2_s2_chkpt);
	ht_add_val(ordj[2]->h_out_edges, 1, (int)s2_s2_jsuper);
	ht_add_val(ordj[2]->h_out_edges, 2, (int)s2_s1_jdesc);
	ht_add_val(ordj[2]->h_out_edges, 3, (int)s2_s1_jrevk);
	ht_add_val(ordj[2]->h_out_edges, 4, (int)s2_s1_jdata);
	ht_add_val(ordj[2]->h_out_edges, 5, (int)s2_s0_ord);
	ht_add_val(ordj[2]->h_out_edges, 6, (int)s2_s3_fail);

	/* -------------------------------------------------------------- */
	/*initialize state s3*/
	ordj[3]->outdegree = 0;

	/* -------------------------------------------------------------- */

	ordered_journaling->states = ordj;
	ordered_journaling->current_state = ordj[0];
	sba_current_model = ordered_journaling;

	return 1;

ret_err:
	sba_debug(1, "Error: unable to allocate memory ... also leaking memory !\n");

	return 0;
}


int sba_common_build_data_journaling_model(void)
{
	/* states for data journaling 
	 * 
	 * Note 1: For now, we're not concerned about bad block remapping.
	 * It is much more complex and requires some more thinking.
	 * 
	 * Note 2: S2 is the final state and S3 represents aborted state
	 * 
	 * Note 3: J represents the journal block. It includes journal descriptor,
	 * journal revoke and journal data blocks.
	 * 
	 * Note 4: F represents the write failure. 
	 * 
	 * 
     *        J    C    K    S    F
     *-------------------------------
	 *  S0    S1                  S3
	 *  S1    S1   S2             S3
	 *  S2    S1        S2   S2   S3
	 *  S3                          
	 * 
	 */
	
	journaling_model *data_journaling = NULL;
	sba_state **dataj = NULL;
	sba_edge *s0_s1_jdesc, *s0_s1_jrevk, *s0_s1_jdata, *s0_s3_fail;
	sba_edge *s1_s1_jdesc, *s1_s1_jrevk, *s1_s1_jdata, *s1_s2_commit, *s1_s3_fail;
	sba_edge *s2_s2_chkpt, *s2_s2_jsuper, *s2_s1_jdesc, *s2_s1_jrevk, *s2_s1_jdata, *s2_s3_fail;
	int dataj_states = 4;
	int i = 0;

	data_journaling = kmalloc(sizeof(journaling_model), GFP_KERNEL);
	if (!data_journaling) {
		goto ret_err;
	}

	data_journaling->mode = DATA_JOURNALING;
	data_journaling->total_states = dataj_states;

	dataj = kmalloc(sizeof(sba_state *)*dataj_states, GFP_KERNEL);
	if (!dataj) {
		goto ret_err;
	}

	for (i = 0; i < dataj_states; i ++) {
		dataj[i] = kmalloc(sizeof(sba_state), GFP_KERNEL);
		if (!dataj[i]) {
			goto ret_err;
		}

		sprintf(dataj[i]->name, "S%d", i);
		ht_create(&dataj[i]->h_out_edges, "outedges");
	}

	/* -------------------------------------------------------------- */
	/*initialize edge s0-s1*/
	s0_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jdesc) {
		goto ret_err;
	}
	s0_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s0_s1_jdesc->input.response = WRITE_SUCCESS;
	s0_s1_jdesc->dest = dataj[1];

	s0_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jrevk) {
		goto ret_err;
	}
	s0_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s0_s1_jrevk->input.response = WRITE_SUCCESS;
	s0_s1_jrevk->dest = dataj[1];

	s0_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s1_jdata) {
		goto ret_err;
	}
	s0_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s0_s1_jdata->input.response = WRITE_SUCCESS;
	s0_s1_jdata->dest = dataj[1];

	/*initialize edge s0-s3*/
	s0_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s0_s3_fail) {
		goto ret_err;
	}
	s0_s3_fail->input.block_type = ANY_BLOCK;
	s0_s3_fail->input.response = WRITE_FAILURE;
	s0_s3_fail->dest = dataj[3];
	
	/*initialize state s0*/
	dataj[0]->outdegree = 4;
	ht_add_val(dataj[0]->h_out_edges, 0, (int)s0_s1_jdesc);
	ht_add_val(dataj[0]->h_out_edges, 1, (int)s0_s1_jrevk);
	ht_add_val(dataj[0]->h_out_edges, 2, (int)s0_s1_jdata);
	ht_add_val(dataj[0]->h_out_edges, 3, (int)s0_s3_fail);
	
	/* -------------------------------------------------------------- */
	/*initialize edge s1-s1*/
	s1_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jdesc) {
		goto ret_err;
	}
	s1_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s1_s1_jdesc->input.response = WRITE_SUCCESS;
	s1_s1_jdesc->dest = dataj[1];

	s1_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jrevk) {
		goto ret_err;
	}
	s1_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s1_s1_jrevk->input.response = WRITE_SUCCESS;
	s1_s1_jrevk->dest = dataj[1];

	s1_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s1_jdata) {
		goto ret_err;
	}
	s1_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s1_s1_jdata->input.response = WRITE_SUCCESS;
	s1_s1_jdata->dest = dataj[1];

	/*initialize edge s1-s2*/
	s1_s2_commit = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s2_commit) {
		goto ret_err;
	}
	s1_s2_commit->input.block_type = JOURNAL_COMMIT_BLOCK;
	s1_s2_commit->input.response = WRITE_SUCCESS;
	s1_s2_commit->dest = dataj[2];

	/*initialize edge s1-s3*/
	s1_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s1_s3_fail) {
		goto ret_err;
	}
	s1_s3_fail->input.block_type = ANY_BLOCK;
	s1_s3_fail->input.response = WRITE_FAILURE;
	s1_s3_fail->dest = dataj[3];
	
	/*initialize state s1*/
	dataj[1]->outdegree = 5;
	ht_add_val(dataj[1]->h_out_edges, 0, (int)s1_s1_jdesc);
	ht_add_val(dataj[1]->h_out_edges, 1, (int)s1_s1_jrevk);
	ht_add_val(dataj[1]->h_out_edges, 2, (int)s1_s1_jdata);
	ht_add_val(dataj[1]->h_out_edges, 3, (int)s1_s2_commit);
	ht_add_val(dataj[1]->h_out_edges, 4, (int)s1_s3_fail);

	/* -------------------------------------------------------------- */
	/*initialize edge s2-s2*/
	s2_s2_chkpt = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_chkpt) {
		goto ret_err;
	}
	s2_s2_chkpt->input.block_type = CHECKPOINT_BLOCK;
	s2_s2_chkpt->input.response = WRITE_SUCCESS;
	s2_s2_chkpt->dest = dataj[2];

	s2_s2_jsuper = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s2_jsuper) {
		goto ret_err;
	}
	s2_s2_jsuper->input.block_type = JOURNAL_SUPER_BLOCK;
	s2_s2_jsuper->input.response = WRITE_SUCCESS;
	s2_s2_jsuper->dest = dataj[2];

	/*initialize edge s2-s1*/
	s2_s1_jdesc = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jdesc) {
		goto ret_err;
	}
	s2_s1_jdesc->input.block_type = JOURNAL_DESC_BLOCK;
	s2_s1_jdesc->input.response = WRITE_SUCCESS;
	s2_s1_jdesc->dest = dataj[1];

	s2_s1_jrevk = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jrevk) {
		goto ret_err;
	}
	s2_s1_jrevk->input.block_type = JOURNAL_REVOKE_BLOCK;
	s2_s1_jrevk->input.response = WRITE_SUCCESS;
	s2_s1_jrevk->dest = dataj[1];

	s2_s1_jdata = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s1_jdata) {
		goto ret_err;
	}
	s2_s1_jdata->input.block_type = JOURNAL_DATA_BLOCK;
	s2_s1_jdata->input.response = WRITE_SUCCESS;
	s2_s1_jdata->dest = dataj[1];

	/*initialize edge s2-s3*/
	s2_s3_fail = kmalloc(sizeof(sba_edge), GFP_KERNEL);
	if (!s2_s3_fail) {
		goto ret_err;
	}
	s2_s3_fail->input.block_type = ANY_BLOCK;
	s2_s3_fail->input.response = WRITE_FAILURE;
	s2_s3_fail->dest = dataj[3];
	
	/*initialize state s2*/
	dataj[2]->outdegree = 6;
	ht_add_val(dataj[2]->h_out_edges, 0, (int)s2_s2_chkpt);
	ht_add_val(dataj[2]->h_out_edges, 1, (int)s2_s2_jsuper);
	ht_add_val(dataj[2]->h_out_edges, 2, (int)s2_s1_jdesc);
	ht_add_val(dataj[2]->h_out_edges, 3, (int)s2_s1_jrevk);
	ht_add_val(dataj[2]->h_out_edges, 4, (int)s2_s1_jdata);
	ht_add_val(dataj[2]->h_out_edges, 5, (int)s2_s3_fail);

	/* -------------------------------------------------------------- */
	/*initialize state s3*/
	dataj[3]->outdegree = 0;

	/* -------------------------------------------------------------- */

	data_journaling->states = dataj;
	data_journaling->current_state = dataj[0];
	sba_current_model = data_journaling;

	return 1;

ret_err:
	sba_debug(1, "Error: unable to allocate memory ... also leaking memory !\n");

	return 0;
}

int sba_common_build_model(void)
{
	switch(journaling_mode) {
		case DATA_JOURNALING:
			sba_common_build_data_journaling_model();
		break;

		case ORDERED_JOURNALING:
			sba_common_build_ordered_journaling_model();
		break;

		case WRITEBACK_JOURNALING:
			sba_common_build_writeback_journaling_model();
		break;

		default:
			sba_debug(1, "Error: unknown journaling mode\n");
			return -1;
	}
	
	sba_common_print_model();

	return 1;
}

int sba_common_destroy_model(void)
{
	int i;
	sba_state **model = sba_current_model->states;
	
	for (i = 0; i < sba_current_model->total_states; i ++) {
		int j;

		/* free all the edges from this state */
		for (j = 0; j < model[i]->outdegree; j ++) {
			sba_edge *e;
			if (ht_lookup_val(model[i]->h_out_edges, j, (int*)&e)) {
				kfree(e);
			}
			else {
				sba_debug(1, "Error: Cannot find edge %d on state %s\n", j, model[i]->name);
			}
		}

		/* destroy the hash table */
		ht_destroy(model[i]->h_out_edges);
		
		/* free this state */
		kfree(model[i]);
	}

	kfree(sba_current_model);
	
	return 1;
}

int sba_common_print_model(void)
{
	int i;
	sba_state **states = sba_current_model->states;

	switch(sba_current_model->mode) {
		case DATA_JOURNALING:
			printk("<<-----------Data journaling mode----------->>\n");
		break;

		case ORDERED_JOURNALING:
			printk("<<-----------Ordered journaling mode----------->>\n");
		break;

		case WRITEBACK_JOURNALING:
			printk("<<-----------Writeback journaling mode----------->>\n");
		break;
	}
	
	for (i = 0; i < sba_current_model->total_states; i ++) {
		int j;

		printk("%s  ", states[i]->name);

		/* free all the edges from this state */
		for (j = 0; j < states[i]->outdegree; j ++) {
			sba_edge *e;
			if (ht_lookup_val(states[i]->h_out_edges, j, (int*)&e)) {
				char input_str[32] = "";
				
				switch(e->input.block_type) {
					case JOURNAL_DESC_BLOCK: 
						sprintf(input_str, "D/");
					break;

					case JOURNAL_REVOKE_BLOCK: 
						sprintf(input_str, "R/");
					break;

					case JOURNAL_DATA_BLOCK: 
						sprintf(input_str, "J/");
					break;

					case JOURNAL_COMMIT_BLOCK: 
						sprintf(input_str, "C/");
					break;

					case JOURNAL_SUPER_BLOCK: 
						sprintf(input_str, "S/");
					break;

					case CHECKPOINT_BLOCK: 
						sprintf(input_str, "K/");
					break;

					case ORDERED_BLOCK: 
						sprintf(input_str, "O/");
					break;

					case UNORDERED_BLOCK: 
						sprintf(input_str, "U/");
					break;

					case ANY_BLOCK: 
						sprintf(input_str, "A/");
					break;
				}
				
				switch(e->input.response) {
					case WRITE_FAILURE: 
						strcat(input_str, "F");
					break;

					case WRITE_SUCCESS: 
						strcat(input_str, "S");
					break;
				}
				
				printk("%s (%s)  ", e->dest->name, input_str);
			}
			else {
				sba_debug(1, "Error: Cannot find edge %d on state %s\n", j, states[i]->name);
			}
		}
		printk("\n");
	}

	printk("<<----------------------------------------------->>\n");
	
	return 1;
}

/*initialize some of the common data structures*/
int sba_common_init(void)
{
	if (!sba_common_build_model()) {
		return -1;
	}

	sba_fault = kmalloc(sizeof(fault), GFP_KERNEL);
	if (!sba_fault) {
		sba_debug(1, "Error: cannot allocate memory\n");
		return -1;
	}
	memset(sba_fault, 0, sizeof(fault));

	ss.total_reads = 0;
	ss.total_writes = 0;

	SBA_LOCK_INIT(&(stat_lock));

	switch(filesystem) {
		#ifdef INC_EXT3
		case EXT3:
			sba_ext3_init();
		break;
		#endif

		#ifdef INC_REISERFS
		case REISERFS:
			sba_reiserfs_init();
		break;
		#endif

		#ifdef INC_JFS
		case JFS:
			sba_jfs_init();
		break;
		#endif
	}

	return 1;
}

int sba_common_cleanup(void)
{
	kfree(sba_fault);

	switch(filesystem) {
		#ifdef INC_EXT3
		case EXT3:
			sba_ext3_cleanup();
		break;
		#endif

		#ifdef INC_REISERFS
		case REISERFS:
			sba_reiserfs_cleanup();
		break;
		#endif

		#ifdef INC_JFS
		case JFS:
			sba_jfs_cleanup();
		break;
		#endif
	}

	sba_common_destroy_model();

	return 1;
}

int sba_common_zero_stat(sba_stat *ss)
{
	ss->total_reads = ss->total_writes = 0;
	return 1;
}

int sba_common_print_stat(sba_stat *ss)
{
	printk("reads %d writes %d\n", ss->total_reads, ss->total_writes);
	return 1;
}

int sba_common_print_journal(void)
{
	switch(filesystem) {
		#ifdef INC_EXT3
		case EXT3:
			sba_ext3_print_journal();
		break;
		#endif

		#ifdef INC_REISERFS
		case REISERFS:
			sba_reiserfs_print_journal();
		break;
		#endif

		#ifdef INC_JFS
		case JFS:
			sba_jfs_print_journal();
		break;
		#endif
	}
	
	return 1;
}

int sba_common_handle_mkfs_write(int sector)
{
	sba_debug(0, "Received WRITE for blk# %d\n", SBA_SECTOR_TO_BLOCK(sector));

	/*if a separate device is used for the journal, then return*/
	if (jour_dev) {
	
		#if 0
		if (is_journal_request(bh)) {
			ht_add(h_journal, bh->b_blocknr);
		}
		#endif

		sba_debug(1, "Error: separate device for journal is not yet supported\n");	
		return 1;
	}
	else {
		/*don't have to cache anything. the new code reads the blocks
		 *of the journal and builds the h_journal*/
	}

	return 1;
}


struct bio *sba_common_alloc_and_init_bio
(struct block_device *dev, int block, bio_end_io_t end_io_func, int rw)
{
	struct bio *sba_bio = NULL;
	struct page *sba_page = NULL;

	sba_bio = bio_alloc(GFP_KERNEL, 1);
	if (!sba_bio) {
		sba_debug(1, "Error: mem allocation\n");
		return NULL;
	}

	sba_page = alloc_page(GFP_KERNEL);
	if (!sba_page) {
		sba_debug(1, "Error: mem allocation\n");
		bio_put(sba_bio);
		return NULL;
	}

	sba_bio->bi_io_vec[0].bv_page = sba_page;
	sba_bio->bi_io_vec[0].bv_len = SBA_BLKSIZE;
	sba_bio->bi_io_vec[0].bv_offset = 0;
	//FIXME sba_bio->bi_io_vec[0].bv_offset = ((unsigned long)page_address(sba_page) & ~PAGE_MASK);
	sba_bio->bi_vcnt = 1;
	sba_bio->bi_idx = 0;
	sba_bio->bi_size = SBA_BLKSIZE;
	sba_bio->bi_bdev = dev;
	sba_bio->bi_sector = SBA_BLOCK_TO_SECTOR(block);
	sba_bio->bi_end_io = end_io_func;
	sba_bio->bi_rw = rw;

	BIO_BUG_ON(!sba_bio->bi_size);
	BIO_BUG_ON(!sba_bio->bi_io_vec);
	
	return sba_bio;
}

struct bio *alloc_bio_for_read
(struct block_device *dev, int block, bio_end_io_t end_io_func)
{
	struct bio *sba_bio = NULL;

	sba_bio = sba_common_alloc_and_init_bio(dev, block, sba_common_end_io, READ_SYNC);
	return sba_bio;
}

/*This method is called once the request is over.*/
int sba_common_end_io(struct bio *sba_bio, unsigned int bytes, int error)
{
	int uptodate = test_bit(BIO_UPTODATE, &sba_bio->bi_flags);
	struct completion *event = (struct completion *)sba_bio->bi_private;

	if (sba_bio->bi_size) {
		return 1;
	}

	if (!uptodate) {
		sba_debug(1, "Error: Read sec %ld\n", sba_bio->bi_sector);
	}
	else {
		sba_debug(0, "Success: Read sec %ld\n", sba_bio->bi_sector);
	}

	complete(event);

	return 0;
}

char *read_block(int block)
{
	char *ret = NULL;
	struct bio *sba_bio = NULL;
	struct completion *event = NULL;

	sba_debug(0, "Reading block %d\n", block);

	//this is used to wait for the completion of the io
	event = kmalloc(sizeof(struct completion), GFP_KERNEL);
	if (!event) {
		sba_debug(1, "Error: mem allocation\n");
		return NULL;
	}
	init_completion(event);


	if (!(sba_bio = alloc_bio_for_read(sba_device.f_dev, block, sba_common_end_io))) {
		kfree(event);
		return NULL;
	}
	sba_bio->bi_private = event;
	ret = bio_data(sba_bio);

	generic_make_request(sba_bio);
	//blk_run_queues(); //only for 2.6.5

	wait_for_completion(event);

	/*io over - so free the event*/
	kfree(event);

	if (test_bit(BIO_UPTODATE, &sba_bio->bi_flags)) {
		bio_put(sba_bio);
	}
	else {
		sba_debug(1, "Error: IO error reading block %d\n", block);
		free_page((int)ret);
		bio_put(sba_bio);
		return NULL;
	}

	sba_debug(0, "Returning block %d\n", block);
	return ret;
}

int sba_common_init_indir_blocks(unsigned long inodenr)
{
	switch(filesystem) {
		#ifdef INC_EXT3
		case EXT3:
			sba_ext3_init_indir_blocks(inodenr);
		break;
		#endif

		#ifdef INC_REISERFS
		case REISERFS:
			sba_reiserfs_init_indir_blocks(inodenr);
		break;
		#endif

		#ifdef INC_JFS
		case JFS:
			sba_jfs_init_indir_blocks(inodenr);
		break;
		#endif
	}

	return 1;
}

int sba_common_init_dir_blocks(unsigned long inodenr)
{
	switch(filesystem) {
		#ifdef INC_EXT3
		case EXT3:
			sba_ext3_init_dir_blocks(inodenr);
		break;
		#endif

		#ifdef INC_REISERFS
		case REISERFS:
			sba_reiserfs_init_dir_blocks(inodenr);
		break;
		#endif

		#ifdef INC_JFS
		case JFS:
			sba_jfs_init_dir_blocks(inodenr);
		break;
		#endif
	}

	return 1;
}

/* 
 * looks at the fault specification and collects more information
 * for fault injection 
 */
int sba_common_process_fault()
{
	if (sba_fault) {
		if (fault_on_queue) {
			switch(filesystem) {
				#ifdef INC_EXT3
				case EXT3:
					sba_ext3_process_fault(sba_fault);
				break;
				#endif

				#ifdef INC_REISERFS
				case REISERFS:
					sba_reiserfs_process_fault(sba_fault);
				break;
				#endif

				#ifdef INC_JFS
				case JFS:
					sba_jfs_process_fault(sba_fault);
				break;
				#endif
			}
		}
	}

	return 1;
}

int reinit_fault(fault *f)
{
	if (f) {
		if (fault_on_queue > 0) {
			if (filesystem != f->filesystem) {
				sba_debug(1, "Error: wrong file system specified\n");
				return 0;
			}
			
			memcpy(sba_fault, f, sizeof(fault));

			fault_on_queue = 1;
			sba_common_print_fault();

			/*set this flag to indicate a new fault has been 
			 *added that has not yet been injected*/
			fault_injected = 0;
		}
	}
	
	return 1;
}

/*adds the user issued fault to sba_fault structure*/
//TODO: change this to accept a list of faults to emulate
int add_fault(fault *f)
{
	if (f) {
		if (fault_on_queue > 0) {
			sba_debug(1, "Warning: Already a fault is on queue\n");
			return 0;
		}
		else {
			if (filesystem != f->filesystem) {
				sba_debug(1, "Error: wrong file system specified\n");
				return 0;
			}
			
			memcpy(sba_fault, f, sizeof(fault));

			fault_on_queue = 1;
			sba_common_print_fault();

			/*set this flag to indicate a new fault has been 
			 *added that has not yet been injected*/
			fault_injected = 0;
		}
	}
	
	return 1;
}

int remove_fault(int force)
{
	if ((force == REM_FORCE) || (sba_fault->fault_mode != STICKY)) { 
		sba_debug(1, "Removing the fault ...\n");
		fault_on_queue = 0;
	}

	return 1;
}

char *sba_common_get_block_type_str(hash_table *h_btype, int sector)
{
	int blk_type = sba_common_get_block_type(h_btype, sector);

	switch(sba_fault->filesystem) {
	#ifdef INC_EXT3
		case EXT3:
			return sba_ext3_get_block_type_str(blk_type);
		break;
	#endif

	#ifdef INC_REISERFS
		case REISERFS:
			return sba_reiserfs_get_block_type_str(blk_type);
		break;
	#endif

	#ifdef INC_JFS
		case JFS:
			return sba_jfs_get_block_type_str(blk_type);
		break;
	#endif

		default:
			return "UNK";
	}
}

int sba_common_print_fault(void)
{
	char *filesystem = "";
	char *rw = "";
	char *ftype = "";
	char *fmode = "";
	char *btype = "unknown";

	sba_debug(1, "<=========Fault Type=========>\n");

	/*Filesystem*/
	switch(sba_fault->filesystem) {
		case EXT3:
			filesystem = "Ext3";

			/*block type*/
			btype = sba_ext3_get_block_type_str(sba_fault->blk_type);
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

	sba_debug(1, "FS: %s RW: %s TYPE: %s MODE: %s BLK: %s NR: %d\n", filesystem, rw, ftype, fmode, btype, sba_fault->blocknr);

	return 1;
}

int sba_common_fault_match(char *data, sector_t sector, struct bio *sba_bio, hash_table *h_this)
{
	if (sba_fault->rw == SBA_WRITE) {
		if ((sba_bio->bi_rw == WRITE) || (sba_bio->bi_rw == WRITE_SYNC)) {
		}
		else {
			return 0;
		}
	}
	else
	if (sba_fault->rw == SBA_READ) {
		if ((sba_bio->bi_rw == READ) || (sba_bio->bi_rw == READA) || (sba_bio->bi_rw == READ_SYNC)) {
		}
		else {
			return 0;
		}
	}
	else
	if (sba_fault->rw == SBA_READ_WRITE) {
		/*this matches for both read and write*/
	}
	else {
		return 0;
	}

	if (filesystem == sba_fault->filesystem) {
		switch(sba_fault->filesystem) {
			#ifdef INC_EXT3
			case EXT3:
				if (sba_common_get_block_type(h_this, sector) == sba_fault->blk_type) {
					if (sba_ext3_fault_match(data, sector, sba_fault)) {
						sba_debug(1, "Block %ld MATCHES with sba_fault\n", SBA_SECTOR_TO_BLOCK(sector));
						return 1;
					}
				}
				else {
					sba_debug(0, "block %ld in ht %x didn't match the fault\n", SBA_SECTOR_TO_BLOCK(sector), (int)h_this);
				}
			break;
			#endif

			#ifdef INC_REISERFS
			case REISERFS:
				if (sba_common_get_block_type(h_this, sector) == sba_fault->blk_type) {
					if (sba_reiserfs_fault_match(data, sector, sba_fault)) {
						sba_debug(1, "Block %ld MATCHES with sba_fault\n", SBA_SECTOR_TO_BLOCK(sector));
						return 1;
					}
				}
			break;
			#endif

			#ifdef INC_JFS
			case JFS:
				if (sba_common_get_block_type(h_this, sector) == sba_fault->blk_type) {
					sba_debug(0, "sba_common_get_block_type matches with type %s\n", sba_common_get_btype_str(sba_fault->blk_type));
					if (sba_jfs_fault_match(data, sector, sba_fault)) {
						sba_debug(1, "Block %ld MATCHES with sba_fault\n", SBA_SECTOR_TO_BLOCK(sector));
						return 1;
					}
					else {
						sba_debug(0, "sba_jfs_fault_match failed to match\n");
					}
				}
				else {
					sba_debug(0, "sba_common_get_block_type failed to match\n");
				}
			break;
			#endif

			default:
				sba_debug(1, "Error: unknown file system\n");
				return 0;
		}
	}

	sba_debug(0, "sba_bio %x doesn't match with the sba_fault\n", (int)sba_bio);

	return 0;
}

int sba_common_commit_block(hash_table *h_this, int sector)
{
	switch (filesystem) {
		case EXT3:
			if (sba_common_get_block_type(h_this, sector) == SBA_EXT3_COMMIT) {
				return 1;
			}
		break;

		case REISERFS:
			return 0;
		break;

		case JFS:
			return 0;
		break;

		default:
			return 0;
	}

	return 0;
}

int sba_common_inject_fault(struct bio *sba_bio, sba_request *sba_req, int *uptodate)
{
	int proceed = 1;
	hash_table *h_this_block_types = NULL;

	/* 
	 * Before doing anything, first build the table that 
	 * stores the block types for the set of blocks in 
	 * this request. This is important as we will use that 
	 * table for future references in this function for 
	 * this request. We create one hash_table for each new
	 * request and destroy it at the end of the request 
	 * because if we use just one table for all the request 
	 * then we may get into potential concurrency problems.
	 */
	
	h_this_block_types = sba_common_build_block_types_table(sba_bio);
	sba_common_collect_stats(sba_req, h_this_block_types);

	/*we can administer the fault now, if any*/
	proceed = sba_common_execute_fault(sba_bio, uptodate, h_this_block_types);

	/* Now clear the table. */
	sba_common_destroy_block_types_table(h_this_block_types);

	return proceed;
}

int sba_common_execute_fault(struct bio *sba_bio, int *uptodate, hash_table *h_this)
{
	int i;
	char *data;
	struct bio_vec *bvl;
	int rem_fault = 1;

	/*to know if the request need to be passed to the actual device*/
	int proceed = 1; 

	int size = bio_sectors(sba_bio)*SBA_HARDSECT;
	sba_debug(0, "rw %ld blk %ld size %d\n", sba_bio->bi_rw, SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector), size);

	bio_for_each_segment(bvl, sba_bio, i) {

		data = (page_address(bio_iovec_idx(sba_bio, i)->bv_page) + bio_iovec_idx(sba_bio, i)->bv_offset);

		/*is there a fault to cause ?*/
		if (fault_on_queue > 0) {

			/*does this block matches the fault criterion ?*/
			if (sba_common_fault_match(data, sba_bio->bi_sector + i*8, sba_bio, h_this)) {

				sba_debug(1, "rw %ld blk %ld size %d\n", sba_bio->bi_rw, SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector + i*8), size);
				sba_common_print_fault();

				/*add statistics about the fault*/
				sba_common_add_fault_injection_stats(h_this, sba_bio->bi_sector + i*8);

				if (sba_fault->fault_type == SBA_FAIL) {
					*uptodate = 0;
					proceed = 0;
				}

				/*we can remove the fault*/
				if (rem_fault) {
					sba_debug(1, "Removing the fault ...\n");
					remove_fault(REM_NOFORCE);
				}
				else {
					sba_debug(1, "Not removing the fault ...\n");
				}

				/*set this flag to indicate that the fault has been injected*/
				fault_injected = 1;
			}
		}
		else {
			sba_debug(0, "There is no fault on queue\n");
		}

		/* if this is a commit block, and if we are asked to crash after 
		 * commit, set the corresponding flags */
		if (crash_after_commit) {
			if (sba_common_commit_block(h_this, sba_bio->bi_sector + i*8)) {
				crash_system = 1;
				sba_common_add_crash_stats();
			}
		}
	}

	return proceed;
}

/*Prints information about the block*/
int sba_common_print_block(struct bio *sba_bio)
{
	if (sba_bio->bi_rw == READ) {
		sba_debug(1, "R sec %ld\n", sba_bio->bi_sector);
	}
	else {
		sba_debug(1, "W sec %ld\n", sba_bio->bi_sector);
	}
	sba_debug(1, "<------------------------------------------------>\n");
	
	return 1;
}

/* ============================================================================= */

char *sba_common_get_btype_str(int btype)
{
	char *ret = "unknown";
	
	switch(btype) {
		case JOURNAL_DESC_BLOCK:
			ret = "journal_desc";
		break;

		case JOURNAL_REVOKE_BLOCK:
			ret = "journal_revoke";
		break;

		case JOURNAL_DATA_BLOCK:
			ret = "journal_data";
		break;

		case JOURNAL_COMMIT_BLOCK:
			ret = "journal_commit";
		break;

		case JOURNAL_SUPER_BLOCK:
			ret = "journal_super";
		break;

		case CHECKPOINT_BLOCK:
			ret = "checkpoint";
		break;

		case ORDERED_BLOCK:
			ret = "ordered";
		break;

		case UNORDERED_BLOCK:
			ret = "unordered";
		break;
	}

	return ret;
}

int sba_common_move_to_start(void)
{
	sba_current_model->current_state = sba_current_model->states[0];
	return 1;
}

sba_state *sba_common_get_current_state(void)
{
	return sba_current_model->current_state;
}

/* this routine will get the block type for a particular block */
int sba_common_get_block_type(hash_table *h_this, sector_t sector)
{
	int ret;
	
	if (!ht_lookup_val(h_this, sector, &ret)) {
		sba_debug(1, "Error: block type not found for block %ld (ht %x)\n", SBA_SECTOR_TO_BLOCK(sector), (int)h_this);
		ret = UNKNOWN_BLOCK;
	}
	
	return ret;
}

/* this method removes all the block numbers from the hash table */
int sba_common_destroy_block_types_table(hash_table *h_this)
{
	int sector;
	ht_open_scan(h_this);

	while (ht_scan(h_this, &sector) > 0) {
		ht_remove(h_this, sector);
	}

	ht_destroy(h_this);

	return 1;
}

/* this method constructs a table with block numbers and their types */
hash_table *sba_common_build_block_types_table(struct bio *sba_bio)
{
	int i;
	struct bio_vec *bvl;
	int sector;
	char *data;
	int btype;
	char type[12];
	hash_table *h_this = NULL;

	ht_create(&h_this, "this_hashtable");

	bio_for_each_segment(bvl, sba_bio, i) {
		data = (page_address(bio_iovec_idx(sba_bio, i)->bv_page) + bio_iovec_idx(sba_bio, i)->bv_offset);
		sector = sba_bio->bi_sector + i*8;

		switch(filesystem) {
			#ifdef INC_EXT3
			case EXT3:
				btype = sba_ext3_block_type(data, sector, type, sba_bio);

				//if (btype == UNKNOWN_BLOCK) {
					//sba_debug(1, "block = %d btype = %s\n", SBA_SECTOR_TO_BLOCK(sector), sba_ext3_get_block_type_str(btype));
				//}
			break;
			#endif

			#ifdef INC_REISERFS
			case REISERFS:
				btype = sba_reiserfs_block_type(data, sector, type, sba_bio);
			break;
			#endif

			#ifdef INC_JFS
			case JFS:
				btype = sba_jfs_block_type(data, sector, type, sba_bio);
				sba_debug(1, "JFS block %d is of type %s\n", 
				SBA_SECTOR_TO_BLOCK(sector), sba_common_get_btype_str(btype));
			break;
			#endif

			default:
				btype = UNKNOWN_BLOCK;
		}

		sba_debug(0, "adding block = %d to ht %x from bio %x\n", SBA_SECTOR_TO_BLOCK(sector), (int)h_this, (int)sba_bio);
		ht_add_val(h_this, sector, btype);
	}

	return h_this;
}

/* this routine will get the block type of the last block in the entire bio */
int sba_common_find_last_block_type(struct bio *sba_bio, hash_table *h_this)
{
	int i;
	struct bio_vec *bvl;
	int sector;
	int btype = UNKNOWN_BLOCK;

	bio_for_each_segment(bvl, sba_bio, i) {
		sector = sba_bio->bi_sector + i*8;
		btype = sba_common_get_block_type(h_this, sector);
	}

	return btype;
}

int sba_common_edge_match(sba_state_input e1, sba_state_input e2)
{
	int ret = 0;

	if ((e1.block_type == ANY_BLOCK) || (e2.block_type == ANY_BLOCK)) {
		//just compare their responses
		if (e1.response == e2.response) {
			return 1;
		}
	}
	else {
		if ((e1.block_type == e2.block_type) && (e1.response == e2.response)) {
			return 1;
		}
	}
	
	return ret;
}

int sba_common_move(int btype, int response)
{
	int ret = INVALID_STATE;
	sba_state *s = sba_current_model->current_state;


	if (s) {
		sba_state_input this_input;
		int i;

		this_input.block_type = btype;
		this_input.response = response;

		sba_debug(0, "block type %s, response %d\n", sba_common_get_btype_str(btype), response);

		for (i = 0; i < s->outdegree; i ++) {
			sba_edge *e;
			if (ht_lookup_val(s->h_out_edges, i, (int*)&e)) {
				if (sba_common_edge_match(e->input, this_input)) {
					/* set the current state to the new value */
					sba_current_model->current_state = e->dest;

					sba_debug(0, "Match found ... moving to the next state %s\n", e->dest->name);
					return VALID_STATE;
				}
			}
			else {
				sba_debug(1, "Error: Cannot find edge %d on state %s\n", i, s->name);
			}
		}
	}

	return ret;
}

int sba_common_is_valid_move(sba_state *s, int btype)
{
	int ret = INVALID_STATE;

	if (s) {
		/* assume the response to be WRITE_SUCCESS */
		sba_state_input this_input;
		int i;

		this_input.block_type = btype;
		this_input.response = WRITE_SUCCESS;

		sba_debug(0, "block type %s, response WRITE_SUCCESS\n", sba_common_get_btype_str(btype));

		for (i = 0; i < s->outdegree; i ++) {
			sba_edge *e;
			if (ht_lookup_val(s->h_out_edges, i, (int*)&e)) {
				if (sba_common_edge_match(e->input, this_input)) {
					sba_debug(0, "Match found ... the next state is %s\n", e->dest->name);
					return VALID_STATE;
				}
			}
			else {
				sba_debug(1, "Error: Cannot find edge %d on state %s\n", i, s->name);
			}
		}
	}
	
	return ret;
}

int sba_common_model_checker(struct bio *sba_bio, hash_table *h_this)
{
	int i;
	int sector;
	char *data;
	struct bio_vec *bvl;
	int match = 0;
	int btype = UNKNOWN_BLOCK;
	sba_state *s = NULL;

	bio_for_each_segment(bvl, sba_bio, i) {
		
		data = (page_address(bio_iovec_idx(sba_bio, i)->bv_page) + bio_iovec_idx(sba_bio, i)->bv_offset);
		sector = sba_bio->bi_sector + i*8;

		btype = sba_common_get_block_type(h_this, sector);
		sba_debug(1, "Write block %d (%s)\n", SBA_SECTOR_TO_BLOCK(sector), sba_common_get_btype_str(btype)); 
		
		s = sba_common_get_current_state();

		if (s) {
			if (sba_common_is_valid_move(s, btype) == VALID_STATE) {
				/* we continue to go thro' the entire list of blocks */
				sba_debug(0, "block %d (%s) matches with the state %s\n", 
				SBA_SECTOR_TO_BLOCK(sector), sba_common_get_btype_str(btype), s->name);
			}
			else {
				return match;
			}
		}
		else {
			return match;
		}
	}

	match = 1;
	return match;
}

int sba_common_print_all_blocks(struct bio *sba_bio)
{
	int i;
	struct bio_vec *bvl;
	int size = bio_sectors(sba_bio)*SBA_HARDSECT;

	bio_for_each_segment(bvl, sba_bio, i) {
		if (sba_bio->bi_rw == 0) {
			//if (SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector+i*8) == 8209) {
				sba_debug(1, "rw %ld blk %ld size %d\n", 
				sba_bio->bi_rw, SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector+i*8), size);
			//}
		}
		else {
			sba_debug(1, "rw %ld blk %ld size %d\n", 
			sba_bio->bi_rw, SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector+i*8), size);
		}
	}

	return 1;
}

int sba_common_report_error(struct bio *sba_bio, hash_table *h_this)
{
	int i;
	int btype;
	struct bio_vec *bvl;
	int size = bio_sectors(sba_bio)*SBA_HARDSECT;

	bio_for_each_segment(bvl, sba_bio, i) {
		btype = sba_common_get_block_type(h_this, sba_bio->bi_sector + i*8);
		sba_debug(1, "Error: rw %ld blk %ld size %d type %s\n", 
		sba_bio->bi_rw, SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector+i*8), size, sba_common_get_btype_str(btype));
	}

	sba_debug(0, "Error: file system request does not match the model\n");
	return 1;
}

int sba_common_print_journaled_blocks(void)
{
	switch(filesystem) {
		#ifdef INC_JFS
		case JFS:
			sba_jfs_print_journaled_blocks();
		break;
		#endif
	}

	return 1;
}

int sba_common_add_desc_stats(int blocknr)
{
	stat_info *record;

	record = kmalloc(sizeof(stat_info), GFP_KERNEL);
	if (!record) {
		sba_debug(1, "Error: unable to allocate memory to stat_info\n");
		return -1;
	}

	record->rw = SBA_DESC;
	record->prev = record->next = NULL;
	record->blocknr = blocknr;
	record->ref_blocknr = -1;
	strcpy(record->btype, "DDATA");
	do_gettimeofday(&(record->stv));
	do_gettimeofday(&(record->etv));

	SBA_LOCK(&stat_lock);
	sba_common_add_stats(record, &stat_list);
	SBA_UNLOCK(&stat_lock);

	return 1;
}

int sba_common_add_workload_end()
{
	stat_info *record;

	record = kmalloc(sizeof(stat_info), GFP_KERNEL);
	if (!record) {
		sba_debug(1, "Error: unable to allocate memory to stat_info\n");
		return -1;
	}

	record->rw = SBA_WKLOAD_END;
	record->prev = record->next = NULL;
	record->blocknr = -1;
	record->ref_blocknr = -1;
	strcpy(record->btype, "WEND");
	do_gettimeofday(&(record->stv));
	do_gettimeofday(&(record->etv));

	SBA_LOCK(&stat_lock);
	sba_common_add_stats(record, &stat_list);
	SBA_UNLOCK(&stat_lock);

	return 1;
}

int sba_common_add_workload_start()
{
	stat_info *record;

	record = kmalloc(sizeof(stat_info), GFP_KERNEL);
	if (!record) {
		sba_debug(1, "Error: unable to allocate memory to stat_info\n");
		return -1;
	}

	record->rw = SBA_WKLOAD_START;
	record->prev = record->next = NULL;
	record->blocknr = -1;
	record->ref_blocknr = -1;
	strcpy(record->btype, "WSTRT");
	do_gettimeofday(&(record->stv));
	do_gettimeofday(&(record->etv));

	SBA_LOCK(&stat_lock);
	sba_common_add_stats(record, &stat_list);
	SBA_UNLOCK(&stat_lock);

	return 1;
}

int sba_common_add_crash_stats()
{
	stat_info *record;

	record = kmalloc(sizeof(stat_info), GFP_KERNEL);
	if (!record) {
		sba_debug(1, "Error: unable to allocate memory to stat_info\n");
		return -1;
	}

	record->rw = SBA_CRASH;
	record->prev = record->next = NULL;
	record->blocknr = -1;
	record->ref_blocknr = -1;
	strcpy(record->btype, "CRASH");
	do_gettimeofday(&(record->stv));
	do_gettimeofday(&(record->etv));

	SBA_LOCK(&stat_lock);
	sba_common_add_stats(record, &stat_list);
	SBA_UNLOCK(&stat_lock);

	return 1;
}

int sba_common_add_fault_injection_stats(hash_table *h_this, int sector)
{
	stat_info *record;

	record = kmalloc(sizeof(stat_info), GFP_KERNEL);
	if (!record) {
		sba_debug(1, "Error: unable to allocate memory to stat_info\n");
		return -1;
	}

	record->rw = SBA_FAIL;
	record->prev = record->next = NULL;
	record->blocknr = SBA_SECTOR_TO_BLOCK(sector);
	record->ref_blocknr = -1;
	strcpy(record->btype, sba_common_get_block_type_str(h_this, sector));
	do_gettimeofday(&(record->stv));
	do_gettimeofday(&(record->etv));

	SBA_LOCK(&stat_lock);
	sba_common_add_stats(record, &stat_list);
	SBA_UNLOCK(&stat_lock);

	return 1;
}

int sba_common_add_stats(stat_info *si, stat_info **list)
{
    if ((!si) || (!list)) {
        sba_debug(1, "Error: invalid si/list\n");
        return -1;
    }

    si->next = NULL;
    si->prev = NULL;

    if ((*list)) {
        si->next = (*list);
        (*list)->prev = si;
    }

    *list = si;

    return 1;
}

int sba_common_get_start_timestamp(sba_request *sba_req)
{
	int i;
	stat_info **record = sba_req->record;

	for (i = 0; i < sba_req->count; i ++) {
		do_gettimeofday(&record[i]->stv);
	}

	return 1;
}

int sba_common_get_end_timestamp(sba_request *sba_req)
{
	int i;
	stat_info **record = sba_req->record;

	for (i = 0; i < sba_req->count; i ++) {
		do_gettimeofday(&record[i]->etv);
	}

	return 1;
}

int sba_common_collect_stats(sba_request *sba_req, hash_table *h_this)
{
	int i;
	stat_info **record;
	struct bio *sba_bio;
	int sector;
	struct bio_vec *bvl;

	record = sba_req->record;
	sba_bio = sba_req->sba_bio;

	bio_for_each_segment(bvl, sba_bio, i) {

		sector = sba_bio->bi_sector + i*8;

		record[i]->rw = sba_req->rw;
		record[i]->prev = record[i]->next = NULL;
		record[i]->blocknr = SBA_SECTOR_TO_BLOCK(sector);
		record[i]->ref_blocknr = -1;

		if ((sba_bio->bi_rw == WRITE) || (sba_bio->bi_rw == WRITE_SYNC)) {
			strcpy(record[i]->btype, sba_common_get_block_type_str(h_this, sector));
		}
		else {
			strcpy(record[i]->btype, sba_common_get_block_type_str(h_this, sector));
		}

		SBA_LOCK(&stat_lock);
		sba_debug(0, "Adding record %x for block %d rw = %d\n", (int)record[i], SBA_SECTOR_TO_BLOCK(sector), record[i]->rw);
		sba_common_add_stats(record[i], &stat_list);
		SBA_UNLOCK(&stat_lock);
	}

	return 1;
}

int sba_common_clean_stats()
{
	stat_info **list = &stat_list;
	stat_info *temp;
	stat_info *freeme;

	sba_debug(1, "Clearing the statistics\n");

	if ((!list) || (!(*list))) {
		sba_debug(1, "Error: invalid list\n");
		return -1;
	}

	temp = *list;

	while (temp) {
		freeme = temp;
		temp = temp->next;
		sba_debug(0, "Freeing record %x\n", (int)freeme);
		kfree(freeme);
	}

	*list = NULL;

	return 1;
}

int sba_common_clean_all_stats()
{
	sba_common_clean_stats();

	/*now clean the file system specific statistics*/
	switch(filesystem) {
		#ifdef INC_EXT3
			case EXT3:
				sba_ext3_clean_stats();
			break;
		#endif

		#ifdef INC_REISERFS
			case REISERFS:
				sba_reiserfs_clean_stats();
			break;
		#endif

		#ifdef INC_JFS
			case JFS:
				sba_jfs_clean_stats();
			break;
		#endif
	}

	return 1;
}

int my_div(int a, int b)
{
	return (int)(a/b);
}

int my_mod(int a, int b)
{
	return (int)(a%b);
}


int sba_common_extract_stats(char *ubuf, struct timeval start_time)
{
	stat_info *temp;
	stat_info *tail;
	char print_stmt[512];
	int pos;
	int temp_count = 1;
	long long timediff1, timediff2;
	stat_info **list = &stat_list;

	if ((!list) || (!(*list))) {
		pos = 0;
		sba_debug(1, "Error: invalid list\n");
		sprintf(print_stmt, "COPY COMPLETED");
		if ((pos + strlen(print_stmt)) < MAX_UBUF_SIZE) {
			copy_to_user(ubuf+pos, print_stmt, strlen(print_stmt));
			pos += strlen(print_stmt);
			ubuf[pos] = '\0';
		}
		return -1;
	}

	SBA_LOCK(&stat_lock);

	temp = *list;

	while (temp->next) {
		temp = temp->next;
	}

	tail = temp;
	pos = 0;

	//sds_print("Going to start the while loop\n");
	while (tail) {

		//sds_print("Inside while loop %d\n", temp_count);

		if (prev_count < temp_count) {

			timediff1 = sba_common_diff_time(start_time, tail->stv);
			timediff2 = sba_common_diff_time(start_time, tail->etv);
			
			if ((tail->rw == READ) || (tail->rw == READA) || (tail->rw == READ_SYNC)) {
				sprintf(print_stmt,"R %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}
			else 
			if ((tail->rw == WRITE) || (tail->rw == WRITE_SYNC)) {
				sprintf(print_stmt,"W %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}
			else
			if (tail->rw == SBA_FAIL) {
				sprintf(print_stmt,"F %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}
			else
			if (tail->rw == SBA_CRASH) {
				sprintf(print_stmt,"C %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}
			else
			if (tail->rw == SBA_DESC) {
				sprintf(print_stmt,"D %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}
			else
			if (tail->rw == SBA_WKLOAD_START) {
				sprintf(print_stmt,"S %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}
			else
			if (tail->rw == SBA_WKLOAD_END) {
				sprintf(print_stmt,"E %ld t= %s b= %d.%06d e= %d.%06d\n", tail->blocknr, 
				tail->btype, my_div(timediff1,1000000), my_mod(timediff1,1000000), 
				my_div(timediff2,1000000), my_mod(timediff2,1000000)); 
			}

			if ((pos + strlen(print_stmt)) < MAX_UBUF_SIZE) {
				copy_to_user(ubuf+pos, print_stmt, strlen(print_stmt));
				pos += strlen(print_stmt);
			}
			else {
				sba_debug(1, "Kernel log messages are greater than the user buffer size\n");
				break;
			}
		}
		

		if (((temp_count+1) - prev_count) > MAX_MSGS) {
			prev_count = temp_count;
			break;
		}	
		
		temp_count ++;
		tail = tail->prev;
	}

	if (!tail) {
		sprintf(print_stmt, "COPY COMPLETED");
		if ((pos + strlen(print_stmt)) < MAX_UBUF_SIZE) {
			copy_to_user(ubuf+pos, print_stmt, strlen(print_stmt));
			pos += strlen(print_stmt);
			ubuf[pos] = '\0';
		}
	}

	SBA_UNLOCK(&stat_lock);

	return 1;
}
