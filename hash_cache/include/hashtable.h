#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include "utils.h"


typedef errorCode (*keyCompareFunction)(void *, void *, int *);
typedef errorCode (*deallocKeyFunction)(void *);
typedef errorCode (*deallocDataFunction)(void *);
//typedef errorCode (*deallocValuesFunction)(void *);

typedef struct hashtableEntry_tag {
  void * key;
  void * data;
  struct hashtableEntry_tag * next;
} hashtableEntry;

typedef struct hashtable_tag{
  /* number of elements stored */
  int size;
  /* number of entries in the hash table */
  int numberEntries;
  hashtableEntry * table;
  keyCompareFunction compare;
  deallocKeyFunction deallocKey;
  deallocDataFunction deallocData;

  /* scan related entries; every hashtable has a scan attached to it */
  int scanActive;
  int crtEntryNr;
  hashtableEntry * crtEntry;
  /* says if we need to advance the pointer of the scan or not. Some opetations, like remove, will implicitly advance the pointer */
  int advance;

} hashtable;



#if 0
/* hashtable data for the DG and analyzer*/
typedef struct addressEntry_tag{
  unsigned int address;
} addressEntry;

typedef struct blockNumberKey_tag{
  unsigned int blockNumber;
} blockNumberKey;

#else
typedef unsigned int blockNumber;
typedef unsigned int addressEntry;
#endif

/* these can be supplied by the application also, present here only for convenience */
//errorCode allocDataEntry(unsigned int address, dataEntry **);
//errorCode compareAddress(void * key1, void * key2, int * result);

/* application specific */
errorCode compareBlockNumber(void * k1, void * k2, int * result);
errorCode compareBlockNumberNoPointer(void * k1, void * k2, int * result);
/* should be specified by the application and be part of the hashtable function */
errorCode hashFunction(hashtable * ht, void * key, int * entry);

/* interface to the hashtable str */

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
int 	  HTsize(hashtable *ht);

#ifndef __KERNEL__

void printAddressEntry(FILE * f, addressEntry * );
void printAddressEntries(FILE * f, hashtable * ht, int entry);
void printHashtableContent(FILE * f, hashtable * ht, char *);

void printBlockNumberAndAddressEntry(FILE * f, hashtableEntry * hte);
void printBlockNumberKey(FILE * f, blockNumber * key);

#else

void printAddressEntry(addressEntry * );
void printAddressEntries(hashtable * ht, int entry);
void printHashtableContent(hashtable * ht, char *);

#endif


// local

errorCode listLookup(hashtableEntry * start, void * key,  keyCompareFunction 
		  compare, hashtableEntry ** predecessor);
errorCode allocHashtableEntry(void * key, void * data, hashtableEntry ** hte);
errorCode lookup(hashtable * ht, void * key, hashtableEntry ** hte);
/* this is a function tailored for each type of key and data stored, should be changed when the key and data types change */

#endif
