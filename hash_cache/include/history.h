#ifndef __HISTORY_H_
#define __HISTORY_H_

#include "analyzer.h"
#include "dataGatherer.h"
#include "consts.h"
#include "utils.h"


/* Structure that will be used as a key to index into the 
   the history table 
*/

typedef errorCode (*keyCompareFunction)(void *, void *, int *);
typedef errorCode (*deallocKeyFunction)(void *);
typedef errorCode (*deallocInfoFunction)(void *);
//typedef errorCode (*deallocValuesFunction)(void *);


typedef struct historyDGKey_tag {
  /*  keyCompareFunction compare;
      deallocFunction deallocFct;*/
  deviceID * device;
  dataID * dataId;
  instanceID instance;
} historyDGKey;

typedef struct historyAnalyzerKey_tag {
  /*  keyCompareFunction compare;
      deallocFunction deallocFct; */
  deviceID * device;
  attributeID * attrId;
  instanceID instance;
} historyAnalyzerKey;


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
  deallocInfoFunction deallocInfo;

} hashtable;

/* hashtable data for the DG and analyzer*/
typedef struct dataEntry_tag{
  int modType;
  /* how data values are organized */
  int historyType;
  /* the data structure for data values has to be decided, depending
     on the kind of history we keep */
  void * values;
  /* deallocFunction deallocFct; */
} dataEntry;

typedef struct values_tag{
  
} values;

/* errorCode insertHashtable(); */

errorCode allocDataEntry(int modType, int historyType, void * values, dataEntry **);
errorCode allocHistoryAnalyzerKey(deviceID * device, attributeID * attrId,
			       instanceID instance, historyAnalyzerKey ** key);
errorCode allocHistoryDGKey(deviceID * device, dataID * dataId,
			 instanceID instance, historyDGKey ** key);
errorCode allocHashtableEntry(void * key, void * data, hashtableEntry ** hte);

/* fills in the data of a key, in a previously allocated key structure */
errorCode fillInHistoryAnalyzerKey(deviceID * device, attributeID * attrId,
			       instanceID instance, historyAnalyzerKey * key);

/* fills in the data of a key, in a previously allocated key structure */
errorCode fillInHistoryDGKey(deviceID * device, attributeID * attrId,
			       instanceID instance, historyDGKey * key);


errorCode compareDG(void * key1, void * key2, int * result);
errorCode compareAnalyzer(void * key1, void * key2, int * result);

errorCode deallocDGEntry(void*);
errorCode deallocAnalyzerEntry(void* entry);

errorCode createHashtable(int numberEntries, keyCompareFunction compare, deallocEntryFunction deallocEntry, hashtable ** ht /* return value */);
/* deallocator is a function that knows how to deallocate data and keys */
errorCode deleteHashtable(hashtable ** ht);
errorCode hashFunction(hashtable * ht, void * key, int * entry);
errorCode insert(hashtable * ht, hashtableEntry * hte);
errorCode deleteKey(hashtable * ht, void * key);
errorCode listLookup(hashtableEntry * start, void * key,  keyCompareFunction 
		  compare, hashtableEntry ** predecessor);
errorCode lookup(hashtable * ht, void * key, hashtableEntry ** hte);

void printDGEntry(FILE * f, hashtableEntry * hte);
void printDataEntry(FILE * f, dataEntry * data);
void printHistoryDGKey(FILE * f, historyDGKey * key);
void printAnalyzerDGKey(FILE * f, historyAnalyzerKey * key);
void printDGEntries(FILE * f, hashtable * ht, int entry);

#endif





