#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include "utils.h"


typedef errorCode (*keyCompareFunction)(void *, void *, int *);
typedef errorCode (*deallocKeyFunction)(void *);
typedef errorCode (*deallocDataFunction)(void *);
//typedef errorCode (*deallocValuesFunction)(void *);

typedef struct hashtableEntry_tag {
  unsigned int key;
  unsigned int data;
  int busy;
  struct hashtableEntry_tag *next;
} hashtableEntry;

typedef struct hashtable_tag{
  /* number of elements stored */
  int size;
  int nr_rows;
  int busy;

  int maxsize;
  hashtableEntry *cur_entry;
  int scan_active;
  int cur_row;
  hashtableEntry *free_pool;
  int next_free;

  hashtableEntry **table;
} hashtable;


/* application specific */
errorCode compareBlockNumber(void * k1, void * k2, int * result);
errorCode compareBlockNumberNoPointer(void * k1, void * k2, int * result);
/* should be specified by the application and be part of the hashtable function */

errorCode createHashtable(int numberEntries, keyCompareFunction compare, deallocKeyFunction deallocK, deallocDataFunction deallocD, hashtable ** ht /* return value */);
errorCode deleteHashtable(hashtable ** ht);
errorCode deleteHashtableAndData(hashtable ** ht);

errorCode HTinsert(hashtable * ht, void * key, void * data);
errorCode HTextract(hashtable * ht, void * key, void ** data);
errorCode HTlookup(hashtable * ht, void * key, void ** data);
errorCode HTupdate(hashtable * ht, void * key, void * data, void ** old_data);

errorCode createHTscan(hashtable * ht);
errorCode HTadvanceScan(hashtable * ht, void ** key);
errorCode destroyHTscan(hashtable * ht);

errorCode HTempty(hashtable * ht, int * res);
int HTsize(hashtable *);

void printHashtableContent(hashtable * ht, char *);

#endif
