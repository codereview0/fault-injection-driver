#ifndef __CACHE_H
#define __CACHE_H

#include "utils.h"
#include "hashtable.h"

typedef struct cache_entry {
	int valid;
	char *data;
	int block;
}cache_entry;

typedef struct cache_tag{
	int size; 
	cache_entry *entry;
} cache;

errorCode cacheCreate(int sizeCache, int HTnumberEntries, keyCompareFunction HTcompare, deallocKeyFunction HTdeallocK, deallocDataFunction HTdeallocD, cache ** c);

errorCode cacheInsert(cache * c, void * key, void * data);
errorCode cacheLookup(cache * c, void * key, void ** data);
errorCode cachePinpoint(cache * c, void * key);
errorCode cacheUnPinpoint(cache * c, void * key);
errorCode cacheLookupFailedInsert(cache * c, void * key, void ** data);
errorCode cacheCopy(cache * src, cache * dest);

errorCode cacheDestroy(cache ** c);

#endif
