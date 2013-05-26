SRCS = read_block.c parse_dir_blk.c parse_inode_blk.c parse_indir_blk.c read_inode_block.c parse_grpdesc_blk.c zero_disk.c read_inode_block1.c parse_bitmap_blk.c
TARG = read_block parse_dir_blk parse_inode_blk parse_indir_blk read_inode_block parse_grpdesc_blk zero_disk read_inode_block1 parse_bitmap_blk
OPTS = -I../include -Wall -O6 -g
LIBS =

all: $(TARG)

$(TARG):
	$(CC) $(LIBS) $(OPTS) -o $@ $@.c

clean:
	rm -f $(TARG)
