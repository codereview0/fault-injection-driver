#ifndef __CACHE_H
#define __CACHE_H

#include "utils.h"
#include "hash2.h"
#include "limitedSizeCache.h"

typedef struct cache_tag{
  hashtable * ht;
  limSizeCache * lsc;
} cache;

//errorCode cacheCreate(cache ** c);

errorCode cacheCreate(int sizeCache, int HTnumberEntries, keyCompareFunction HTcompare, deallocKeyFunction HTdeallocK, deallocDataFunction HTdeallocD, cache ** c);

errorCode cacheInsert(cache * c, void * key, void * data);
errorCode cacheLookup(cache * c, void * key, void ** data);
errorCode cachePinpoint(cache * c, void * key);
errorCode cacheUnPinpoint(cache * c, void * key);
errorCode cacheLookupFailedInsert(cache * c, void * key, void ** data);
errorCode cacheCopy(cache * src, cache * dest);
errorCode cacheProbe(cache * c, void * key, accnt ** a);
errorCode cacheDestroy(cache ** c);
errorCode cacheTouch(cache * c, void * key);
errorCode cacheSize(cache *c);
errorCode cacheAllPinpointed(cache * c);
errorCode cacheExtract(cache * c, void * key);
#endif
