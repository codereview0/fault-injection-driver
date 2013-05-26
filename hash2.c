#include "hash2.h"
extern int diff_time(struct timeval, struct timeval);

//#define DF_HASH_TIME 1
#undef DF_HASH_TIME

int df_list_lookup(hashtableEntry *head, int key, int *val)
{
	hashtableEntry *tmp = head;
	for(tmp = head; tmp; tmp = tmp->next) {
		if (tmp->key == key) {
			*val = tmp->data;
			return 1;
   		}
   	}
	return -1;
}

int df_list_insert(hashtable *ht, hashtableEntry **head, int key, int val)
{
	hashtableEntry *tmp = kmalloc(sizeof(hashtableEntry), GFP_ATOMIC);

	//hashtableEntry *tmp = df_get_hash_entry(ht);
	if (!tmp)
		panic("df_list_insert: kmalloc failed");
	tmp->key = key;
	tmp->data = val;
	tmp->next = *head;
	*head = tmp;
	return 1;
}

int df_list_delete(hashtableEntry **head, int key, int *val)
{
	hashtableEntry *tmp = *head, *prev = *head;
	if (!tmp)
		return -1;

  	if (tmp->key == key) {
		*head = tmp->next;
		*val = tmp->data;
		//df_put_hash_entry(tmp);
		kfree(tmp);
		return 1;
  	}
  	for( ; tmp ; tmp = tmp->next) {
		if (tmp ->key == key) {
			*val = tmp->data;
			prev->next = tmp->next;
			//df_put_hash_entry(tmp);
			kfree(tmp);
			return 1;
   		}
		prev = tmp;
   	}
	return -1;
}


int df_list_free(hashtableEntry *head)
{
	hashtableEntry *tmp = head, *tmp1 = head;

	while(tmp) {
		tmp1 = tmp->next;
		kfree(tmp);
		//df_put_hash_entry(tmp);
		tmp = tmp1;
  	}
	return 1;
}

void df_list_print(hashtableEntry *head)
{
	hashtableEntry *tmp = head;
	for(; tmp ; tmp = tmp->next)
		printk("%d ", tmp->key);
}


/* res will be 1 if the hashtable is empty */
int HTempty(hashtable * ht, int * res)
{
	if (ht == NULL)
    	return STATUS_ERR;
	else {
    	*res = (ht -> size == 0);
    	return STATUS_OK;
  	}
}

int HTsize(hashtable *ht)
{
	return ht->size;
}

errorCode createHTscan(hashtable * ht)
{
	ht->cur_entry = ht->table[0];
	ht->cur_row = 0;
	ht->scan_active = 1;
	return 1;
}

int destroyHTscan(hashtable *ht)
{
	ht->scan_active = 0;
	ht->cur_entry = NULL;
	return 1;
}


/* needs lock aquired here also */
int HTadvanceScan(hashtable * ht, void ** key)
{
	if (ht->busy)
		panic("\nRACE CONDITION IN HASH");
  	ht->busy = 1;

	if (!ht->cur_entry) {
		ht->cur_row ++;
		while ((ht->cur_row < ht->nr_rows) && !ht->table[ht->cur_row])
			ht->cur_row ++;

   		if (ht->cur_row >= ht->nr_rows) {
			ht->busy = 0;
			return -1;
   		}
		*key = (void*)(ht->table[ht->cur_row] ->key);
		ht->cur_entry = ht->table[ht->cur_row]->next;
		ht->busy = 0;
		return 1;
  	}

	*key = (void*)(ht->cur_entry ->key);
	ht->cur_entry = ht->cur_entry ->next;
	ht->busy = 0;
	return 1;
}


/* How to distinguish between a null pointer and block number 0? */
errorCode compareBlockNumberNoPointer(void * k1, void * k2, int * result){
  unsigned int key1 = (int)k1;
  unsigned int key2 = (int)k2;

  if (key1 == key2)
    *result = 0;
  else
    *result = 1;
  return STATUS_OK;
}


errorCode createHashtable(int maxsize, keyCompareFunction compare, deallocKeyFunction deallocK, deallocDataFunction deallocD, hashtable ** tmp_ht)
{
	int i = 0;
	hashtable *ht;

	if (!(ht = kmalloc(sizeof(hashtable), GFP_ATOMIC)))
		return STATUS_ERR;

	ht ->size = 0;
	ht ->scan_active = 0;
	ht ->cur_entry = NULL;
	ht ->maxsize = maxsize;
	ht ->next_free = 0;

	ht ->nr_rows = maxsize;
	ht ->busy = 0;

/*	if (!(ht->free_pool = kmalloc(sizeof(hashtableEntry)* maxsize, GFP_ATOMIC)))
		printk("FATAL: kmalloc failed in hashcreate!!\n");
*/

	if (!(ht ->table = kmalloc(sizeof(hashtableEntry *) * ht->nr_rows, GFP_ATOMIC)))
		printk("FATAL: kmalloc failed !!!!!!!!!!!\n");

	for(i = 0; i < ht->nr_rows; i++)
		ht ->table[i] = NULL;

/*	for(i = 0; i < ht->maxsize; i++)
		ht ->free_pool[i].busy = 0;
*/

	*tmp_ht = ht;
	return STATUS_OK;
}


/* deallocator is a function that knows how to deallocate data and keys */
errorCode deleteHashtable(hashtable ** tmp_ht)
{
	hashtable *ht = *tmp_ht;
	int i;

	if (!ht || !ht->table) {
  		printk("FATAL!! Hash table CORRUPT\n");
		return -1;
	}

	for(i = 0; i< ht->nr_rows; i++) {
		if (ht->table[i])
			df_list_free(ht->table[i]);
   	}


	//kfree(ht->free_pool);
	kfree(ht->table);
	kfree(ht);
	*tmp_ht = NULL;
  	return 1;
}

int compute_hash(hashtable * ht, int key)
{
	int hash = ((unsigned int)key) % (ht -> nr_rows);
	return hash;
}

int HTinsert(hashtable * ht, void * key, void * data)
{
	int val;
	int hash = compute_hash(ht, (int)key);

#ifdef DF_HASH_TIME
	struct timeval s_tv, e_tv;
	do_gettimeofday(&s_tv);
#endif

	if (ht->busy)
		panic("\nRACE CONDITION IN HASH");
  	ht->busy = 1;

	if (hash < 0)
		panic("\nIncorrect hash");

	//printk("HTinsert: key = %d\n", (int)key);
	if (df_list_lookup(ht->table[hash], (int)key, &val) >= 0) {
		ht->busy = 0;
		return STATUS_DUPL_ENTRY;
  	}

  	df_list_insert(ht, &ht->table[hash], (int)key, (int)data);
	ht->size ++;
	ht->busy = 0;
#ifdef DF_HASH_TIME
	do_gettimeofday(&e_tv);
	printk("HTinsert: time %d\n", diff_time(s_tv, e_tv));
#endif
	return 1;
}


errorCode HTextract(hashtable * ht, void * key, void ** data)
{
	int hash = compute_hash(ht, (int)key);
	int ret = -1;

#ifdef DF_HASH_TIME
	struct timeval s_tv, e_tv;
	do_gettimeofday(&s_tv);
#endif

	if (hash < 0)
		panic("\nIncorrect hash");

	//printk("HTextract: key = %d\n", (int)key);
	if (ht->scan_active && ht->cur_entry && (ht-> cur_entry->key == (int)key)) {
		if (ht->cur_row != hash)
			panic("\nduplicate entries detected in HTextract");
		ht->cur_entry = ht->cur_entry ->next;
  	}

	if (df_list_delete(&ht->table[hash], (int)key, (int*)data) > 0) {
		ht->size --;
		ht->busy = 0;
		ret = 1;
  	}
#ifdef DF_HASH_TIME
	do_gettimeofday(&e_tv);
	printk("HTextract: time %d\n", diff_time(s_tv, e_tv));
#endif

	return ret;
}


errorCode HTlookup(hashtable * ht, void * key, void ** data)
{
	int hash = compute_hash(ht, (int)key);
	int ret = -1;

#ifdef DF_HASH_TIME
	struct timeval s_tv, e_tv;
	do_gettimeofday(&s_tv);
#endif


	if (hash < 0)
		panic("\nIncorrect hash");

	//printk("HTlookup: hash = %d, key = %d\n", hash, (int)key);
	if (df_list_lookup(ht->table[hash], (int)key, (int*)data) > 0)
		ret = 1;

	//do_gettimeofday(&e_tv);
	//printk("Time - HTlookup = %ld usec\n", diff_time(s_tv, e_tv));
#ifdef DF_HASH_TIME
	do_gettimeofday(&e_tv);
	printk("HTlookup: time %d\n", diff_time(s_tv, e_tv));
#endif

	return ret;
}

int HTupdate(hashtable * ht, void * key, void * data, void ** old_data)
{
	int val;
	int hash = compute_hash(ht, (int)key);

#ifdef DF_HASH_TIME
	struct timeval s_tv, e_tv;
	do_gettimeofday(&s_tv);
#endif


	if (ht->busy)
		panic("\nRACE CONDITION IN HASH");
  	ht->busy = 1;

	if (hash < 0)
		panic("\nIncorrect hash");

	if (df_list_lookup(ht->table[hash], (int)key, &val) > 0) {
		*old_data = (void*)val;
		ht->busy = 0;
		return STATUS_DUPL_ENTRY;
  	}

	df_list_insert(ht, &ht->table[hash], (int)key, (int)data);
	ht->size ++;
	ht->busy = 0;
#ifdef DF_HASH_TIME
	do_gettimeofday(&e_tv);
	printk("HTupdate: time %d\n", diff_time(s_tv, e_tv));
#endif

	return 1;
}

void printHashtableContent(hashtable * ht, char * str)
{
	int i;
	for(i = 0; i< ht->nr_rows; i++)
		df_list_print(ht->table[i]);
  	printk("\n");
}
