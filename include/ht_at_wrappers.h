#ifndef __INCLUDE_HT_AT_WRAPPERS__
#define __INCLUDE_HT_AT_WRAPPERS__ 

#include <hash2.h>
#include <cache.h>
#include "avl_tree.h"

typedef struct avl_tree {
	struct tree *tree;
	spinlock_t lock;
	int num;
	char name[32];
	int cur_key_val;
}avl_tree;
 
typedef struct hash_table {
	char name[30];
	hashtable *table;
	spinlock_t lock;
	int past_size;
}hash_table;

int ht_add(hash_table *ht, int key);
int ht_add_val(hash_table *ht, int key, int val);
int ht_add_force(hash_table *ht, int key, int val);
int ht_lookup(hash_table *ht, int key);
int ht_lookup_val(hash_table *ht, int key, int *val);
int ht_remove(hash_table *ht, int key);
int ht_open_scan(hash_table *ht);
int ht_scan(hash_table *ht, int *blk);
int ht_create_with_size(hash_table **ht, char *name, int size);
int ht_create(hash_table **ht, char *name);
int ht_destroy(hash_table *ht);
void ht_print(hash_table *ht);
int ht_get_size(hash_table *ht);

int at_create(avl_tree **t, char *name);
int at_find_closest_before(avl_tree *t, int key, int *res_key);
int at_destroy(avl_tree *t);
int at_num_elements(avl_tree *t);
int at_add_val(avl_tree *t,  int key, int val);
int at_remove(avl_tree *t, int key);
int at_lookup(avl_tree *t, int key);
int at_lookup_val(avl_tree *t, int key, int *val);
int at_open_scan(avl_tree *t);
int at_scan(avl_tree *t, int *key);

#endif
