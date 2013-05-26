#include "ht_at_wrappers.h"

#define HT_AT_lock(a)		spin_lock((a))
#define HT_AT_unlock(a)		spin_unlock((a))
#define HT_AT_lock_init(a)	spin_lock_init((a))

#define HASH_TABLE_ENTRIES	1237

/*Hash table wrappers */
int normal_compare(void *key1, void *key2, int *eq)
{
	*eq = ((int)key1 == (int)key2) ? 0:1 ;
	return *eq;
}
 
int ht_create_with_size(hash_table **ht, char *name, int size)
{
	hash_table *tmp = kmalloc(sizeof(hash_table), GFP_ATOMIC);
	HT_AT_lock_init(&tmp->lock);
	tmp->past_size = 0;
	strcpy(tmp->name, name);

	createHashtable(size, compareBlockNumberNoPointer, NULL, NULL, &tmp->table);
	*ht = tmp;
	return 1;
}

int ht_create(hash_table **ht, char *name)
{
	hash_table *tmp = kmalloc(sizeof(hash_table), GFP_ATOMIC);
	HT_AT_lock_init(&tmp->lock);
	tmp->past_size = 0;
	strcpy(tmp->name, name);

	createHashtable(HASH_TABLE_ENTRIES, compareBlockNumberNoPointer, NULL, NULL, &tmp->table);
	*ht = tmp;
	return 1;
}

int ht_get_size(hash_table *ht)
{
	return ht->table->size;
}
 
int ht_destroy(hash_table *ht)
{
	deleteHashtable(&ht->table);
	kfree(ht);
	return 1;
}
 
 
static inline void ht_display_mem_footprint(hash_table *ht)
{
	int diff_size = HTsize(ht->table) - ht->past_size;
	if ((diff_size > 25) || (diff_size < -25)) {
		struct timeval st;
		ht->past_size = HTsize(ht->table);
		do_gettimeofday(&st);
		printk("HT %s: %ld.%ld : %d\n", ht->name, st.tv_sec, st.tv_usec, ht->past_size);
	}
}

int ht_add(hash_table *ht, int key)
{
	int val = 1, st;

	HT_AT_lock(&ht->lock);
	st = HTinsert(ht->table, (void*)key, (void*)val);

	if (st == STATUS_DUPL_ENTRY) {
		printk("duplicate insertion into %s key %d\n", ht->name, key);
	}

#ifdef MEMORY_FOOTPRINT
	ht_display_mem_footprint(ht);
#endif

	//printk("ht_add: key = %d, st = %d\n", key, st);
	HT_AT_unlock(&ht->lock);
	return st;
}
 
int ht_add_val(hash_table *ht, int key, int val)
{
	int st;

	//printk("ht_add_val: key = %d, val = %d\n", key, val);

	HT_AT_lock(&ht->lock);
	st = HTinsert(ht->table, (void*)key, (void*)val);

	if (st == STATUS_DUPL_ENTRY) {
		printk("Duplicate insertion into %s\n", ht->name);
	}

#ifdef MEMORY_FOOTPRINT
	ht_display_mem_footprint(ht);
#endif

	HT_AT_unlock(&ht->lock);

	return st;
}
 
int ht_add_force(hash_table *ht, int key, int val)
{
	int st; int tmp;
	HT_AT_lock(&ht->lock);
	HTextract(ht->table, (void*)key, (void*)&tmp);
	st = HTinsert(ht->table, (void*)key, (void*)val);

#ifdef MEMORY_FOOTPRINT
	ht_display_mem_footprint(ht);
#endif

	HT_AT_unlock(&ht->lock);
	return st;
}

int ht_lookup(hash_table *ht, int key)
{
	int val, st;

	HT_AT_lock(&ht->lock);
	st = HTlookup(ht->table, (void*)key, (void**)&val);

	//printk("ht_lookup: key = %d, HTlookup returned %d\n", key, st);
	if (st < 0)
			st = 0;

	HT_AT_unlock(&ht->lock);

	return st;
}
 
int ht_lookup_val(hash_table *ht, int key, int *val)
{
	int st;
	//printk("ht_lookup_val, key = %d\n", key);

	HT_AT_lock(&ht->lock);
	st = HTlookup(ht->table, (void*)key, (void**)val);
	if (st < 0)
			st = 0;
	HT_AT_unlock(&ht->lock);
	return st;
}
 
int ht_remove(hash_table *ht, int key)
{
	int val, st;
	//printk("ht_remove: key = %d\n", key);
	HT_AT_lock(&ht->lock);
	st = HTextract(ht->table, (void*)key, (void**)&val);
#ifdef MEMORY_FOOTPRINT
	ht_display_mem_footprint(ht);
#endif

	HT_AT_unlock(&ht->lock);
	return st;
}

void ht_print(hash_table *ht)
{
	//HT_AT_lock(&ht->lock);
	printk("Contents of %s\n", ht->name);
	printHashtableContent(ht->table, "");
	//HT_AT_unlock(&ht->lock);
}
 
int ht_open_scan(hash_table *ht)
{
	int st;
	HT_AT_lock(&ht->lock);
	st = createHTscan(ht->table);
	HT_AT_unlock(&ht->lock);
	return st;
}
 
int ht_scan(hash_table *ht, int *blk)
{
	int st = -1;
	HT_AT_lock(&ht->lock);
	if (ht->table->size)
			st = HTadvanceScan(ht->table, (void*)blk);
	HT_AT_unlock(&ht->lock);
	return st;
}


/* AVL tree wrappers */
int at_create(avl_tree **t, char *name)
{
	avl_tree *tmp = kmalloc(sizeof(avl_tree), GFP_ATOMIC);
	graid_avl_init(&tmp->tree);
	spin_lock_init(&tmp->lock);

	strcpy(tmp->name, name);
	tmp->num  = 0;
	*t = tmp;
	return 1;
}
 
int at_destroy(avl_tree *t)
{
	struct tree *node;
	HT_AT_lock(&t->lock);
	while((node = graid_avl_find_next(t->tree, 0))) {
			graid_avl_remove(&t->tree, node);
			if (!t->tree)
					break;
	}

	HT_AT_unlock(&t->lock);
	kfree(t);
	return 1;
}

int at_add_val(avl_tree *t,  int key, int val)
{
	struct tree *node;

	HT_AT_lock(&t->lock);

	if (graid_avl_find(t ->tree, key)) {
			printk("Error: Duplicate insertion in add_val: key %x\n", key);
			HT_AT_unlock(&t->lock);
			return -1;
	}

	node = kmalloc(sizeof(struct tree), GFP_ATOMIC);
	if (!node)
			panic("mem alloc failure in at_add_val\n");
	node->key = key;
	node->val = (void*)val;
	node->tree_avl_left = node->tree_avl_right = NULL;

	if (graid_avl_insert(&t->tree, node) > 0)
			t->num ++;

	HT_AT_unlock(&t->lock);

	return 1;
}
 
int at_lookup(avl_tree *t, int key)
{
	int retval;
	HT_AT_lock(&t->lock);

	if (graid_avl_find(t->tree, key))
			retval = 1;
	else
			retval = 0;
	HT_AT_unlock(&t->lock);
	return retval;
}

int at_lookup_val(avl_tree *t, int key, int *val)
{
	int retval;
	struct tree *node;

	HT_AT_lock(&t->lock);

	if ((node = graid_avl_find(t->tree, key))) {
		*val = (int)node->val;
		retval = 1;
	}
	else
		retval = 0;
	HT_AT_unlock(&t->lock);

	return retval;
}
 
int at_remove(avl_tree *t, int key)
{
	struct tree *node;
	int retval = -1;
	HT_AT_lock(&t->lock);
	if ((node = graid_avl_find(t->tree, key))) {
			if (graid_avl_remove(&t->tree, node) > 0)
					t->num --;
			retval = 1;
	}

	HT_AT_unlock(&t->lock);
	return retval;
}

int at_find_closest_before(avl_tree *t, int key, int *res_key)
{
	int retval = -1;
	struct tree *node;
	HT_AT_lock(&t->lock);

	if ((node = graid_avl_find_previous(t->tree, key)) && (node->key > 0)) {
			*res_key = node->key;
			retval = 1;
	}

	HT_AT_unlock(&t->lock);
	return retval;
}
 
int at_num_elements(avl_tree *t)
{
	return t->num;
}
 
int at_open_scan(avl_tree *t)
{
	t->cur_key_val = -1;
	return 1;
}
 
int at_scan(avl_tree *t, int *key)
{
	struct tree *node;
	int retval;
	HT_AT_lock(&t->lock);

	if ((node = graid_avl_find_next(t->tree, t->cur_key_val))) {
			if (node ->key < 0)
					retval = 0;
			else {
					*key = node->key;
					t->cur_key_val = node->key;
					retval = 1;
			}
	}
	else
			retval = 0;
	HT_AT_unlock(&t->lock);
	return retval;
}
