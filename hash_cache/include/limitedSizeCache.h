#ifndef __LIMITED_SIZE_CACHE_H
#define __LIMITED_SIZE_CACHE_H

#include "utils.h"

#define DATA(t, i) ((t -> dataSpace) + i * CACHE_BLOCK_SIZE)
#define ACCNT_COUNTER(t, i) (((t -> accntArray) + i) -> counter )
#define ACCNT_USED(t, i) (((t -> accntArray) + i) -> used )
#define ACCNT_KEY(t, i) (((t -> accntArray) + i) -> key )
#define ACCNT_TIME(t, i) (((t -> accntArray) + i) -> tv )
#define ACCNT_DATA(t, i) (((t -> accntArray) + i) -> data )
#define ACCNT_BH(t, i) (((t -> accntArray) + i) -> bh )
#define ACCNT_EVICT(t, i) (((t -> accntArray) + i) -> evict )
#define ACCNT(t, i) ((((t) -> accntArray) + i))
 
typedef struct accnt_tag{
  int used; /* -1 = unused, 0 = pinpointed, 1 = used */
  int counter; /* used in LFU */
  struct timeval tv; /* used for the LRU cache policy */
  void * key;
  void * data;
  int evict; /* = 1, need to evict from cache when the update information is available */
  void * bh;
}accnt;

typedef struct limSizeCache_tag{
  
  /* size of the cache */
  int size;

  /* number of elements pinpointed */
  int pinpointed; 

  /* how many slots are occupied */
  int usage;

  /* will keep the actual data that needs to be stored */
  void * dataSpace;
  
  /* keeps accounting data, reference counters and the key necessary to remove a record from the hash*/
  accnt * accntArray;
  
} limSizeCache;

/* creates the data structures associated with the limited size cache: 
   - space where the data will be stored
   - accounting structure that keeps track of the accesses and used blocks
*/
errorCode createLimCache(int size, limSizeCache ** lsc);

errorCode findEmptySlot(limSizeCache * lsc, unsigned int * slot);

errorCode copyData(limSizeCache * lsc, unsigned int slot, void * data);

/* returns the slot that was freed
*/
errorCode findEvictionCandidate(limSizeCache * lsc, unsigned int * slot);
errorCode insertNewElement(limSizeCache * lsc, unsigned int slot, void * key, void * data);
errorCode insertNewElementNoDataCopy(limSizeCache * lsc, unsigned int slot, void * key, void ** address);
/* 
   updates the usage counter for slot
*/
errorCode updateCnt(limSizeCache * lsc, unsigned int slot);
errorCode setCnt(limSizeCache * lsc, unsigned int slot, int value);
errorCode getCnt(limSizeCache * lsc, unsigned int slot, int * value);
errorCode updateAccessTime(limSizeCache * lsc, unsigned int slot);
errorCode moveAttrib(limSizeCache * src, limSizeCache * dest, void * key, int slotSrc, int slotDest);

errorCode pinpoint(limSizeCache * lsc, unsigned int slot);
errorCode unPinpoint(limSizeCache * lsc, unsigned int slot);
errorCode allPinpointed(limSizeCache * lsc);

errorCode invalidateEntries(limSizeCache *lsc);
errorCode destroyCache(limSizeCache ** lsc);

errorCode getAccntInfo(limSizeCache * lsc, unsigned int slot, accnt ** a);
errorCode cleanAccntInfo(limSizeCache * lsc, unsigned int slot);
#ifndef __KERNEL__

errorCode printInfoLSCache(FILE * f, limSizeCache * lsc);

#else

errorCode printInfoLSCache(limSizeCache * lsc);

#endif

#endif
