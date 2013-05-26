#ifndef __INCLUDE_SBA_H__
#define __INCLUDE_SBA_H__

#include "sba_common.h"

#define DEVICE_NAME 	"sba"

/* 
 * This structure holds device specific information.
 */
 
typedef struct _sba_dev {
	struct block_device *f_dev;
	struct block_device *j_dev;
	unsigned long size;
	spinlock_t lock;
	struct gendisk *gd;
	int usage;
}	sba_dev;

/* fwd declarations */

long long diff_time(struct timeval st, struct timeval et);
int sba_open(struct inode *inode, struct file *filp);
int sba_release(struct inode *inode, struct file *filp);
int sba_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
int sba_check_change(struct gendisk *gd);
int sba_revalidate(struct gendisk *gd);
int sba_get_f_device_number(void);
int sba_get_device_number(void);
char *read_block(int block);

#endif /* __INCLUDE_SBA_H__ */

