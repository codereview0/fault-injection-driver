EXTRA_CFLAGS += -I/root/vijayan/sosp05/analysis/include -I/root/vijayan/sosp05/analysis/hash_cache/include
obj-m += SBA.o
SBA-objs += avl_tree.o hash2.o ht_at_wrappers.o sba_ext3.o sba_common.o sba.o
