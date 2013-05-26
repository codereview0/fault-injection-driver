#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include "sba.h"
 
static int sba_major;       /* 0 - let the system assign the major no */

/*This holds the sba device related info*/
extern sba_dev sba_device;

/*This struct contains some of the statistics*/
extern sba_stat ss;

/* Our request queue */
static struct request_queue *sba_queue;

/*Type of file system*/
extern int filesystem;

/* To avoid the mke2fs traffic */
int start_sba;

/* To only check selected traffic */
int test_system = 0;

/* To crash the file system */
int squash_writes = 0;

/*Is journal in a separate device ?*/
extern int jour_dev;

/* When the driver starts*/
struct timeval start_time;

/*flag to indicate whether fault has been injected*/
extern int fault_injected;

/*to emulate system crash*/
extern int crash_system;

/*to crash a system after commit and before checkpointing to initiate recovery*/
extern int crash_after_commit;

/*array of physical device number*/
//static int f_dev[] = {MKDEV(8, 33)}; 
static int f_dev[] = {MKDEV(8, 35)}; 
static int j_dev[] = {MKDEV(8, 51)}; 

/*separate journal device can also be specified while loading*/
module_param(jour_dev, int, 0);

/*end io function*/
static bio_end_io_t sba_end_io;

/*------------------------------------------------------------------*/

/* Open the device. Increment the usage count */
int sba_open(struct inode *inode, struct file *filp)
{
	int num = iminor(inode);
	if (num >= SBA_DEVS) {
		return -ENODEV;
	}

	sba_device.usage ++;

	return 0;
}

/*Close the device. Decrement the usage count.*/
int sba_release(struct inode *inode, struct file *filp)
{
	struct block_device *bd = I_BDEV(inode);
	sba_device.usage --;

	if (!sba_device.usage) { 
		fsync_bdev(bd);
	}

	return 0;
}

int sba_ioctl(struct inode *inode, struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	switch (cmd) {

	case START_SBA:
		start_sba = 1;

		/*find the journal entries only when the file system is ext3
		  and there is no separate journal device*/
		switch(filesystem) {
			#ifdef INC_EXT3
			case EXT3:
				if (!jour_dev) {
					sba_ext3_start();
				}
			break;
			#endif

			#ifdef INC_REISERFS
			case REISERFS:
				if (!jour_dev) {
					sba_reiserfs_start();
				}
			break;
			#endif

			#ifdef INC_JFS
			case JFS:
				if (!jour_dev) {
					sba_jfs_start();
				}
			break;
			#endif
		}
		
		sba_common_print_model();
	break;

	case STOP_SBA:
		start_sba = 0;
		break;

	case INJECT_FAULT:
		add_fault((fault *)arg);
		break;

	case REINIT_FAULT:
		reinit_fault((fault *)arg);
		break;

	case REMOVE_FAULT:
		remove_fault(REM_FORCE);
		break;

	case PRINT_JOURNAL:
		sba_common_print_journal();
		break;

	case PRINT_STAT:
		sba_common_print_stat(&ss);
		sba_common_zero_stat(&ss);
		break;

	case ZERO_STAT:
		sba_common_zero_stat(&ss);
		break;

	case CLEAN_STAT:
		sba_common_clean_stats();
		break;

	case CLEAN_ALL_STAT:
		sba_common_clean_all_stats();
		break;

	case PRINT_FAULT:
		sba_common_print_fault();
		break;

	case FAULT_INJECTED:
		{
			int *response = (int *)arg;
			copy_to_user(response, &fault_injected, sizeof(fault_injected));
		}
		break;

	case PROCESS_FAULT:
		sba_common_process_fault();
		break;

	case INIT_DIR_BLKS:
		{
			unsigned long inodenr;
			unsigned long *temp_inodenr = (unsigned long *)arg;

			inodenr = *(temp_inodenr);
			sba_debug(1, "<<<--------inode nr = %ld----------------------->>>\n", inodenr);

			sba_common_init_dir_blocks(inodenr);
		}
		break;

	case INIT_INDIR_BLKS:
		{
			unsigned long inodenr;
			unsigned long *temp_inodenr = (unsigned long *)arg;

			inodenr = *(temp_inodenr);
			sba_debug(1, "<<<--------inode nr = %ld----------------------->>>\n", inodenr);

			sba_common_init_indir_blocks(inodenr);
		}
		break;

	case MOVE_2_START:
		sba_common_move_to_start();
		break;

	case TEST_SYSTEM:
		test_system = 1;
		break;

	case DONT_TEST:
		test_system = 0;
		break;

	case SQUASH_WRITES:
		squash_writes = 1;
		break;

	case ALLOW_WRITES:
		squash_writes = 0;
		break;

	case PRINT_JBLOCKS:
		sba_common_print_journaled_blocks();
		break;

	case CRASH_COMMIT:
		crash_after_commit = 1;
		break;

	case DONT_CRASH_COMMIT:
		crash_after_commit = 0;
		crash_system = 0;
		break;

	case CRASH_SYSTEM:
		crash_system = 1;
		break;

	case DONT_CRASH:
		crash_system = 0;
		break;

	case WORKLOAD_START:
		sba_common_add_workload_start();
		break;

	case WORKLOAD_END:
		sba_common_add_workload_end();
		break;

	case EXTRACT_STATS:
		{
			char *ubuf = (char *)arg;

			if (access_ok(VERIFY_WRITE, ubuf, MAX_UBUF_SIZE)) {
				sba_common_extract_stats(ubuf, start_time);
			}
			else {
				sba_debug(1, "Invalid user buffer\n");
			}
		}
		break;

	}

	return -ENOTTY;
}

int sba_media_changed(struct gendisk *gd)
{
	return 1;
}

int sba_revalidate_disk(struct gendisk *gd)
{
	return 0;
}


struct block_device_operations sba_bdops = {
	.owner = THIS_MODULE,
	.open = sba_open,
	.release = sba_release,
	.ioctl = sba_ioctl,
	.media_changed = sba_media_changed,
	.revalidate_disk = sba_revalidate_disk
};

int sba_get_j_device_number(void)
{
	return j_dev[0];
}

int sba_get_f_device_number(void)
{
	return f_dev[0];
}

/*Returns the physical device number*/
int sba_get_device_number(void)
{
	return sba_get_f_device_number();
}

/* this end io routine is to handle mkfs traffic - it doesn't do any 
 * fancy things like collecting statistics */
int sba_mkfs_end_io(struct bio *sba_bio, unsigned int bytes, int error)
{
	struct bio *org_bio = (struct bio *)sba_bio->bi_private;
	int uptodate = test_bit(BIO_UPTODATE, &sba_bio->bi_flags);

	if (sba_bio->bi_size) {
		return 1;
	}

	sba_debug(0, "bio for sector %ld over (uptodate %d)\n", sba_bio->bi_sector, uptodate);

	if (uptodate) {
		bio_endio(org_bio, org_bio->bi_size, 0);
	}
	else {
		bio_endio(org_bio, org_bio->bi_size, -EIO);
	}

	bio_put(sba_bio);

	return 0;
}

int sba_end_io(struct bio *sba_bio, unsigned int bytes, int error)
{
	int uptodate;

	uptodate = test_bit(BIO_UPTODATE, &sba_bio->bi_flags);

	if (sba_bio->bi_size) {
		return 1;
	}

	sba_debug(0, "bio for blk %ld over (uptodate %d)\n", SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector), uptodate);

	if (sba_bio->bi_private) {
		stat_info **record;
		struct bio *sba_bio_org;
		sba_request *sba_req = NULL;
		
		sba_req = (sba_request *)sba_bio->bi_private;
		sba_bio_org = sba_req->sba_bio;
		record = sba_req->record;

		sba_common_get_end_timestamp(sba_req);

		if ((bio_data_dir(sba_bio) == READ) || (bio_data_dir(sba_bio) == READA) || (bio_data_dir(sba_bio) == READ_SYNC)) {
			if (test_system) {
				sba_common_inject_fault(sba_bio_org, sba_req, &uptodate);
			}
		}

		if (uptodate) {
			bio_endio(sba_bio_org, sba_bio_org->bi_size, 0);
		}
		else {
			bio_endio(sba_bio_org, sba_bio_org->bi_size, -EIO);
		}

		/*free the record pointers*/
		kfree(sba_req->record);
		kfree(sba_req);

		bio_put(sba_bio);
		return 0;
	}
	else {
		bio_put(sba_bio);
		return 1;
	}
}

int sba_print_bio(struct bio *sba_bio)
{
	int i;
	struct bio_vec *bvl;

	if ((bio_data_dir(sba_bio) == READ) || (bio_data_dir(sba_bio) == READA) || (bio_data_dir(sba_bio) == READ_SYNC)) {
		sba_debug(1, "READ block %ld size = %d sectors\n", SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector), bio_sectors(sba_bio));

		//access each page of data
		bio_for_each_segment(bvl, sba_bio, i) {
			sba_debug(0, "READ: Page vir addrs %0x\n", (int)(bio_iovec_idx(sba_bio, i)->bv_page));
		}
	}
	else {
		sba_debug(1, "WRITE block %ld size = %d sectors\n", SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector), bio_sectors(sba_bio));

		if (bio_sectors(sba_bio) > 8) {
			//access each page of data
			bio_for_each_segment(bvl, sba_bio, i) {
				sba_debug(0, "WRITE: Page vir addrs %0x\n", (int)(bio_iovec_idx(sba_bio, i)->bv_page));
			}
		}
	}

	return 1;
}

sba_request *allocate_trace(struct bio *sba_bio_clone, struct bio *sba_bio_org)
{
	int i;
	sba_request *sba_req;

	sba_req = kmalloc(sizeof(sba_request), GFP_KERNEL);
	if (!sba_req) {
		sba_debug(1, "Error: unable to allocate memory to sba request\n");
		return NULL;
	}
	
	if (bio_sectors(sba_bio_org)/8 > 1) {
		sba_debug(1, "More than one block (%d) in sba_bio request\n", bio_sectors(sba_bio_org)/8);
	}

	sba_req->sba_bio = sba_bio_org;

	if (bio_sectors(sba_bio_org) < 8) {
		sba_debug(1, "Note: The number of sectors in this req = %d\n", bio_sectors(sba_bio_org));
		sba_req->count = 1;
	}
	else {
		sba_req->count = bio_sectors(sba_bio_org)/8;
	}

	/*allocate mem for pointers*/
	sba_req->record = kmalloc(sizeof(stat_info *)*sba_req->count, GFP_KERNEL);
	if (!sba_req->record) {
		sba_debug(1, "Error: unable to allocate memory to records\n");
		kfree(sba_req);
		return NULL;
	}

	/*allocate mem for records*/
	for (i = 0; i < sba_req->count; i ++) {
		sba_req->record[i] = kmalloc(sizeof(stat_info), GFP_KERNEL);
		if (!sba_req->record[i]) {
			sba_debug(1, "Error: unable to allocate memory to records\n");
			kfree(sba_req);
			return NULL;
		}
		memset(sba_req->record[i], 0, sizeof(stat_info));
	}

	sba_req->rw = bio_data_dir(sba_bio_org);

	return sba_req;
}

int sba_crash_system(struct bio *sba_bio)
{
	bio_endio(sba_bio, sba_bio->bi_size, -EIO);
	return 1;
}

/*
 * in this function, we check if the request matches the model
 * we've build for the system. if it matches, then we pass the
 * request to the fault injector which may inject the fault. we
 * then study the file system's response to the fault. as far as
 * my current plans go, we are currently going to concentrate 
 * only on writes from the journaling file system. reads will be
 * passed as usual to/from the disk.
 */

/*
 * rough algo:
 * 1. check if a request matches the failure criterion
 * 2. if so, fail it
 */

int sba_new_request(request_queue_t *queue, struct bio *sba_bio)
{
	int uptodate;
	struct bio *sba_bio_clone;

	sba_bio_clone = bio_clone(sba_bio, GFP_NOIO);

	sba_bio_clone->bi_bdev = sba_device.f_dev;
	sba_bio_clone->bi_sector = sba_bio->bi_sector;

	if (!start_sba) {
		/* these are mkfs traffic - allow them to go as usual */
		sba_bio_clone->bi_end_io = sba_mkfs_end_io;
		sba_bio_clone->bi_private = sba_bio;
		sba_bio_clone->bi_rw = bio_data_dir(sba_bio);

		if ((bio_data_dir(sba_bio) == READ) || (bio_data_dir(sba_bio) == READA) || (bio_data_dir(sba_bio) == READ_SYNC)) {
			switch(filesystem) {
				case EXT3:
				case REISERFS:
				case JFS:
					sba_debug(1, "Block %ld is read during mkfs\n", SBA_SECTOR_TO_BLOCK(sba_bio->bi_sector));
				break;
			}
		}
		else {
			switch(filesystem) {
				case EXT3:
				case REISERFS:
				case JFS:
					sba_common_handle_mkfs_write(sba_bio->bi_sector);
				break;
			}
		}
	}
	else {
		sba_request *sba_req;

		sba_print_bio(sba_bio);

		if (crash_system) {
			/* this is to emulate crashing of the system *
			 * we fail all the reads and writes          */
			sba_crash_system(sba_bio);
			bio_put(sba_bio_clone);
			return 0;
		}
		
		sba_req = allocate_trace(sba_bio_clone, sba_bio);
		sba_common_get_start_timestamp(sba_req);

		sba_bio_clone->bi_end_io = sba_end_io;
		sba_bio_clone->bi_private = sba_req;
		sba_bio_clone->bi_rw = bio_data_dir(sba_bio);

		if ((bio_data_dir(sba_bio) == READ) || (bio_data_dir(sba_bio) == READA) || (bio_data_dir(sba_bio) == READ_SYNC)) {
			#ifdef COLLECT_STAT
				ss.total_reads += bio_sectors(sba_bio)/8;
			#endif
		}
		else {
			#ifdef COLLECT_STAT
				ss.total_writes += bio_sectors(sba_bio)/8;
			#endif

			if (test_system) {
				int proceed = 1;

				/*we can administer the fault now, if any*/
				proceed = sba_common_inject_fault(sba_bio, sba_req, &uptodate);

				if (!proceed) {
					if (uptodate) {
						sba_bio->bi_size = 0;
						bio_endio(sba_bio, sba_bio->bi_size, 0);
					}
					else {
						bio_endio(sba_bio, sba_bio->bi_size, -EIO);
					}
					
					bio_put(sba_bio_clone);
					return 0;
				}
			}
		}
	}
	
	/*make the actual request*/
	generic_make_request(sba_bio_clone);

	return 0;
}

/* This method will initialize the device specific structures */
int __init sba_init(void)
{
	int nsectors;

	do_gettimeofday(&start_time);

	SBA_LOCK_INIT(&(sba_device.lock));

	if (jour_dev) {
		dev_t f_dev_no = sba_get_f_device_number();
		dev_t j_dev_no = sba_get_j_device_number();

		sba_device.size = SBA_SIZE + SBA_JOURNAL_SIZE;

		sba_device.f_dev = open_by_devnum(f_dev_no, FMODE_READ|FMODE_WRITE);
		if (IS_ERR(sba_device.f_dev)) {
			return PTR_ERR(sba_device.f_dev);
		}

		sba_device.j_dev = open_by_devnum(j_dev_no, FMODE_READ|FMODE_WRITE);
		if (IS_ERR(sba_device.j_dev)) {
			return PTR_ERR(sba_device.j_dev);
		}
	}
	else {
		dev_t f_dev_no = sba_get_f_device_number();

		sba_device.size = SBA_SIZE;

		sba_device.f_dev = open_by_devnum(f_dev_no, FMODE_READ|FMODE_WRITE);
		if (IS_ERR(sba_device.f_dev)) {
			goto out;
		}
	}

	nsectors = sba_device.size * 2;

	/* Get a request queue */
	//sba_queue = blk_init_queue(sba_request, &sba_device.lock);
	sba_queue = blk_alloc_queue(GFP_KERNEL);
	if (!sba_queue) {
		goto out;
	}
	blk_queue_hardsect_size(sba_queue, SBA_HARDSECT);
	blk_queue_make_request(sba_queue, sba_new_request);

	/* Register */
	sba_major = register_blkdev(sba_major, DEVICE_NAME);
	if (sba_major <= 0) {
		sba_debug(1, "sba: can't get major %d\n", sba_major);
		goto out;
    }

	/* Add the gendisk structure */
	sba_device.gd = alloc_disk(SBA_DEVS);
	if (!sba_device.gd) {
		goto out_unregister;
	}

	sba_device.gd->major = sba_major;
	sba_device.gd->first_minor = 0;
	sba_device.gd->fops = &sba_bdops;
	sba_device.gd->private_data = &sba_device;
	sba_device.gd->queue = sba_queue;
	strcpy(sba_device.gd->disk_name, DEVICE_NAME);
	set_capacity(sba_device.gd, nsectors);

	//add the journal disk partition
	//add_partition(sba_device->gd, 1, SBA_SIZE*2, SBA_JOURNAL_SIZE);

	add_disk(sba_device.gd);

	/*initialize other data structures*/
	sba_common_init();

	sba_debug(1, "SBA init over ... successfully added the driver (total sec %d)\n", nsectors);

	return 0;

	out_unregister:
    	unregister_blkdev(sba_major, DEVICE_NAME);
  	out:
		sba_debug(1, "Unable to load the device\n");
    	return -ENOMEM;
}

/*
 * Free up the allocated memory and cleanup.
 */

static __exit void sba_cleanup(void)
{
	del_gendisk(sba_device.gd);
	put_disk(sba_device.gd);
	unregister_blkdev(sba_major, DEVICE_NAME);
	blk_cleanup_queue(sba_queue);

	sba_common_cleanup();

	sba_debug(1, "SBA cleanup over ... exiting\n");
}

module_init(sba_init);
module_exit(sba_cleanup);

MODULE_LICENSE("GPL");
