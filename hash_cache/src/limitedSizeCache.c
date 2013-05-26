#include "utils.h"
#include "limitedSizeCache.h"



/* creates the data structures associated with the limited size cache: 
   - space where the data will be stored
   - accounting structure that keeps track of the accesses and used blocks
*/
errorCode createLimCache(int size, limSizeCache ** lsc){
  int i = 0;

  FCT_ERROR(memAlloc(sizeof(limSizeCache), (void **)lsc));
  FCT_ERROR(memAlloc(size * sizeof(accnt), (void **)&((*lsc) -> accntArray)));
  //FCT_ERROR(memAlloc(size * CACHE_BLOCK_SIZE, (void **)&((*lsc) -> dataSpace)));
  
  for (i = 0; i < size; i++){
    //((*lsc) -> accntArray)[i].counter = 0; /* keep track of LFU*/
    ((*lsc) -> accntArray)[i].used = -1; /* unused */
    ((*lsc) -> accntArray)[i].key = NULL;
    ((*lsc) -> accntArray)[i].tv.tv_sec = 0;
    ((*lsc) -> accntArray)[i].tv.tv_usec = 0;
    ((*lsc) -> accntArray)[i].data = NULL;
    ((*lsc) -> accntArray)[i].bh = NULL;
    ((*lsc) -> accntArray)[i].evict = 0;
  }

  (*lsc) -> size = size;
  (*lsc) -> usage = 0;

  return STATUS_OK;
}


errorCode findEmptySlot(limSizeCache * lsc, unsigned int * slot){
  int i = 0;

  for (i = 0; i < lsc -> size; i++){
    if (ACCNT_USED(lsc, i) == -1){
      *slot = i;
      return STATUS_OK;
    }
  }

  return STATUS_ERR;

}


errorCode cleanAccntInfo(limSizeCache * lsc, unsigned int slot){
  ACCNT_USED(lsc, slot) = -1;
  ACCNT_KEY(lsc, slot) = NULL;
  ACCNT_EVICT(lsc, slot) = 0;
  ACCNT_BH(lsc, slot) = NULL;
  /* do not free memory here for the data, seems there are issues in the kernel?*/
  
  return STATUS_OK;

}


errorCode getAccntInfo(limSizeCache * lsc, unsigned int slot, accnt ** a){
  if (slot > lsc -> size || slot < 0)
    return STATUS_ERR;
  *a = ACCNT(lsc, slot);
 
  return STATUS_OK;
}

/* Function that will be used by the cacheLookupFailedInsert() 
   It inserts an element, but does no data copying
   The access counter is updated
 */
errorCode insertNewElementNoDataCopy(limSizeCache * lsc, unsigned int slot, void * key, void ** address){
  updateAccessTime(lsc, slot);
  ACCNT_USED(lsc, slot) = 1;

  ACCNT_KEY(lsc, slot) = key;
  ACCNT_EVICT(lsc, slot) = 0;
  ACCNT_BH(lsc, slot) = NULL;

  if (ACCNT_DATA(lsc, slot) == NULL){
    /* we need to allocate data space */
   FCT_FAIL(pageAlloc(CACHE_BLOCK_SIZE, &(ACCNT_DATA(lsc, slot))), "insertNewElementNoDataCopy: CAn not allocate memory for data");
    
  }
  
  *address = ACCNT_DATA(lsc, slot);
  
  return STATUS_OK;
}



errorCode insertNewElement(limSizeCache * lsc, unsigned int slot, void * key, void * data){
  copyData(lsc, slot, data);
  //setCnt(lsc, slot, 1);
  updateAccessTime(lsc, slot);
  ACCNT_USED(lsc, slot) = 1;
  ACCNT_KEY(lsc, slot) = key;
  ACCNT_BH(lsc, slot) = NULL;
  ACCNT_EVICT(lsc, slot) = 0;

  return STATUS_OK;
}


/* copies the data to the assigned slot */
errorCode copyData(limSizeCache * lsc, unsigned int slot, void * data){
  if (!ACCNT_DATA(lsc, slot)){
    /* we need to allocate data space */
    FCT_FAIL(pageAlloc(CACHE_BLOCK_SIZE, &(ACCNT_DATA(lsc, slot))), "insertNewElementNoDataCopy: CAn not allocate memory for data");  
  }

  memcpy(ACCNT_DATA(lsc, slot), data, CACHE_BLOCK_SIZE);
  
  return STATUS_OK;

}


/* returns the slot that has to be freed
   for now it goes through all entries looking for the minimum usage counter
 */
/*
  This works for LFU
errorCode findEvictionCandidate(limSizeCache * lsc, unsigned int * slot){
  int minCounter = INT_MAX;
  int tempSlot = -1;
  int i;

  for (i = 0; i < lsc -> size; i++){
    if (ACCNT_COUNTER(lsc, i) < minCounter){
      tempSlot = i;
      minCounter = ACCNT_COUNTER(lsc, i);
    }
  }
  
  if (tempSlot != -1){
    *slot = tempSlot;
    return STATUS_OK;
  }
  else {
    *slot = -1;
    return STATUS_ERR;
  }

}
*/


/* returns 1 if the first argument represents a moment further in time than the second argument */
int furtherInTime(struct timeval tv1, struct timeval tv2){
  float t1, t2;

  t1 = tv1.tv_sec + (float)(tv1.tv_usec) / (float)1000000;
  t2 = tv2.tv_sec + (float)(tv2.tv_usec) / (float)1000000;
  
  if (t1 < t2)
    return 1;
  else 
    return 0;

}


/* need to look for the slot that was accessed the furthest back in time */
errorCode findEvictionCandidate(limSizeCache * lsc, unsigned int * slot){
  int tempSlot = -1;
  int i;
  struct timeval crtFurther;

  GETTIMEOFDAY(&crtFurther);

  for (i = 0; i < lsc -> size; i++){
    if (ACCNT_USED(lsc, i) == 0)
      continue;
    if (furtherInTime(ACCNT_TIME(lsc, i), crtFurther)){
      tempSlot = i;
      crtFurther = ACCNT_TIME(lsc, i);
    }
  }
  
  if (tempSlot != -1){
    *slot = tempSlot;
    return STATUS_OK;
  }
  else {
    *slot = -1;
    return STATUS_ERR;
  }
}
  

/* update the usage slots, pinpoint access time */
errorCode moveAttrib(limSizeCache * src, limSizeCache * dest, void * key, int slotSrc, int slotDest){
  ACCNT_TIME(dest, slotDest) = ACCNT_TIME(src, slotSrc);
  ACCNT_USED(dest, slotDest) = ACCNT_USED(src, slotSrc);
  /* key deallocation needed here if the key is a pointer, not a scalar */
  ACCNT_KEY(dest, slotDest) = ACCNT_KEY(src, slotSrc);

  ACCNT_USED(src, slotSrc) = -1;
  ACCNT_KEY(src, slotSrc) = NULL;

  ACCNT_BH(dest, slotDest) = ACCNT_BH(src, slotSrc);
  ACCNT_EVICT(dest, slotDest) = ACCNT_EVICT(src, slotSrc);

  return STATUS_OK;

}



errorCode updateAccessTime(limSizeCache * lsc, unsigned int slot){
  GETTIMEOFDAY(&(ACCNT_TIME(lsc, slot)));
  
  return STATUS_OK;
}


errorCode pinpoint(limSizeCache * lsc, unsigned int slot){
  if (ACCNT_USED(lsc, slot) == -1)
    return STATUS_ERR;

  lsc -> pinpointed ++;
  ACCNT_USED(lsc, slot) = 0;
  return STATUS_OK;

}

errorCode unPinpoint(limSizeCache * lsc, unsigned int slot){
  if (ACCNT_USED(lsc, slot) != 0)
    return STATUS_ERR;

  lsc -> pinpointed --;
  ACCNT_USED(lsc, slot) = 1;
  return STATUS_OK;

}

errorCode allPinpointed(limSizeCache * lsc){
  if (lsc -> size == lsc -> pinpointed)
    return STATUS_OK;
  else
    return STATUS_ERR;
}

/* 
   updates the usage counter for slot
   - increase the value by 1 for LFU
   - update the access time for LRU
*/
errorCode updateCnt(limSizeCache * lsc, unsigned int slot){
  ACCNT_COUNTER(lsc, slot) ++;

  return STATUS_OK;
}


errorCode setCnt(limSizeCache * lsc, unsigned int slot, int value){
  ACCNT_COUNTER(lsc, slot) = value;
  
  return STATUS_OK;
}


errorCode getCnt(limSizeCache * lsc, unsigned int slot, int * value){
  * value = ACCNT_COUNTER(lsc, slot);
  
  return STATUS_OK;
}


/* pointer to keys will not be freed here */
errorCode destroyCache(limSizeCache ** lsc){
  int i = 0;
  
  for(i = 0; i < (*lsc) -> size; i++){
    if ((*lsc) -> accntArray[i].data)
      pageFree((*lsc) -> accntArray[i].data);
  }
 
  memFree((*lsc) -> accntArray);

  memFree(*lsc);
 
  *lsc = NULL;

  return STATUS_OK;
}

#ifndef __KERNEL__

errorCode printInfoLSCache(FILE * f, limSizeCache * lsc){
  int i; 

  for (i = 0; i < lsc -> size; i++){
    fprintf(f, "Slot %d\n", i);
    fprintf(f, "\t used %d\n", ACCNT_USED(lsc, i));
    fprintf(f, "\t counter %d\n", ACCNT_COUNTER(lsc, i));
    fprintf(f, "\t int key %d\n", (int)ACCNT_KEY(lsc, i));
    fprintf(f, "\t data pointer %p \n\n", ACCNT_DATA(lsc, i));
  }
  
  fflush(f);

}

#else

errorCode printInfoLSCache(limSizeCache * lsc){
  int i; 

  for (i = 0; i < lsc -> size; i++){
    printk("Slot %d\n", i);
    printk("\t used %d\n", ACCNT_USED(lsc, i));
    printk("\t counter %d\n", ACCNT_COUNTER(lsc, i));
    printk("\t int key %d\n", (int)(ACCNT_KEY(lsc, i)));
    printk("\t data pointer %p \n\n", ACCNT_DATA(lsc, i));
  }
  

}

#endif
