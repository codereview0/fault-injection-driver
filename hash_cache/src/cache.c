
#include "cache.h"


/* create a cache, need init parameters for the hash and limited size cache */
errorCode cacheCreate(int sizeCache, int HTnumberEntries, keyCompareFunction HTcompare, deallocKeyFunction HTdeallocK, deallocDataFunction HTdeallocD, cache ** c){
  FCT_ERROR(memAlloc(sizeof(cache), (void **)c));
  
  FCT_ERROR(createHashtable(HTnumberEntries, HTcompare, HTdeallocK, HTdeallocD, (&((*c) -> ht))));
  FCT_ERROR(createLimCache(sizeCache, (&((*c) -> lsc))));

  return STATUS_OK;

}


/* Pinpoints a value in the cache */
errorCode cachePinpoint(cache * c, void * key){
  int slot;

  FCT_ERROR(HTlookup(c -> ht, (void*)key, (void **)&slot));
  FCT_ERROR(pinpoint(c -> lsc, slot));

  return STATUS_OK;
}


/* 
   This function is called after we saw that the key is not in the cache, we will insert the key and return in data the pointer to the address where the data should be stored
*/
errorCode cacheLookupFailedInsert(cache * c, void * key, void ** data){
  int res;

    /* No data available, we will try to insert the key, and return an address where the data will be copied */

  int slot;
  void * extractedData;

  if (findEmptySlot(c -> lsc, &slot) == STATUS_OK){
    /* we have an empty slot
       - copy data
       - insert in the hash
       - update counter, key information
    */

    if ((res = HTinsert(c -> ht, (void *)key, (void *)slot)) != STATUS_OK)
      return res;
    FCT_ERROR(insertNewElementNoDataCopy(c -> lsc, slot, key, data));
  }
  else {
    /* no free slots
       - find a victim 
       - evict from the hash
       - insert new element
     */
    findEvictionCandidate(c -> lsc, &slot);

    FCT_ERROR(HTextract(c -> ht, ACCNT_KEY(c -> lsc, slot), &extractedData));
    //FCT_ERROR(HTinsert(c -> ht, key, slot));
    if ((res = HTinsert(c -> ht, (void*)key, (void*)slot)) != STATUS_OK)
      return res;
    FCT_ERROR(insertNewElementNoDataCopy(c -> lsc, slot, key, data));
  }


  return STATUS_OK;


}

/* 
   This function is called after we saw that the key is not in the cache, we will insert the key and return in data the pointer to the address where the data should be stored
   returns the slot where the data was inserted
*/
errorCode cacheLookupFailedInsertSlot(cache * c, void * key, void ** data, int * resSlot){
  int res;

    /* No data available, we will try to insert the key, and return an address where the data will be copied */

  int slot;
  void * extractedData;

  if (findEmptySlot(c -> lsc, &slot) == STATUS_OK){
    /* we have an empty slot
       - copy data
       - insert in the hash
       - update counter, key information
    */

    if ((res = HTinsert(c -> ht, (void*)key, (void*)slot)) != STATUS_OK)
      return res;
    FCT_ERROR(insertNewElementNoDataCopy(c -> lsc, slot, key, data));
  }
  else {
    /* no free slots
       - find a victim 
       - evict from the hash
       - insert new element
     */
    findEvictionCandidate(c -> lsc, &slot);

    FCT_ERROR(HTextract(c -> ht, (void*)(ACCNT_KEY(c -> lsc, slot)), (void**)&extractedData));
    //FCT_ERROR(HTinsert(c -> ht, key, slot));
    if ((res = HTinsert(c -> ht, (void*)key, (void*)slot)) != STATUS_OK)
      return res;
    FCT_ERROR(insertNewElementNoDataCopy(c -> lsc, slot, key, data));
  }

  *resSlot = slot;

  return STATUS_OK;


}


errorCode cacheAllPinpointed(cache * c){
  return allPinpointed(c -> lsc);
}


/* ? */
errorCode cacheUnPinpoint(cache * c, void * key){
  int slot;

  FCT_ERROR(HTlookup(c -> ht, (void*)key, (void**)&slot));
  FCT_ERROR(unPinpoint(c -> lsc, slot));

  return STATUS_OK;
}


errorCode cacheInsert(cache * c, void * key, void * data){
  int slot, old_slot;
  void * extractedData;
  int res = STATUS_ERR;

  if (findEmptySlot(c -> lsc, &slot) == STATUS_OK){
    /* we have an empty slot
       - copy data
       - insert in the hash
       - update counter, key information
    */
    res = HTupdate(c -> ht, (void*)key, (void*)slot, (void**)&old_slot);

    if (res == STATUS_ERR)
      return res;

    if (res == STATUS_OK){
      FCT_ERROR(insertNewElement(c -> lsc, slot, key, data));
    }
    else{
      FCT_ERROR(insertNewElement(c -> lsc, old_slot, key, data));
    }
  }
  else {
    /* no free slots
       - find a victim 
       - evict from the hash
       - insert new element
     */
    findEvictionCandidate(c -> lsc, &slot);

    res = HTupdate(c -> ht, (void*)key, (void*)slot, (void**)&old_slot);
    if (res == STATUS_ERR)
      return res;
      
    if (res == STATUS_OK){
      FCT_ERROR(HTextract(c -> ht, (void*)(ACCNT_KEY(c -> lsc, slot)), (void**)&extractedData));
      FCT_ERROR(insertNewElement(c -> lsc, slot, key, data));
    }

    if (res == STATUS_DUPL_ENTRY){
      FCT_ERROR(insertNewElement(c -> lsc, old_slot, key, data));
    }
    

  }


  return STATUS_OK;
}


/* returns a pointer to the data that is stored in the limited size cache 
   updates the access counter also !
*/
errorCode cacheLookup(cache * c, void * key, void ** data){
  int slot;

  FCT_ERROR(HTlookup(c -> ht, (void*)key, (void**)&slot));
  updateAccessTime(c -> lsc, slot);
  // Used for LFU updateCnt(c -> lsc, slot);
  *data = ACCNT_DATA((c -> lsc), slot);
  
  return STATUS_OK;
  
}

errorCode cacheTouch(cache * c, void * key){
  int slot;

  FCT_ERROR(HTlookup(c -> ht, (void*)key, (void**)&slot));
  updateAccessTime(c -> lsc, slot); 
  
  return STATUS_OK;
}

/* extracts an element from the cache */
errorCode cacheExtract(cache * c, void * key){
  int slot;

  FCT_ERROR(HTextract(c -> ht, (void*)key, (void**)&slot));
  cleanAccntInfo(c -> lsc, slot);

  return STATUS_OK;
}


/* returns a pointer to the data that is stored in the limited size cache 
   updates the access counter also !
*/
errorCode cacheProbe(cache * c, void * key, accnt ** a){
  int slot;

  FCT_ERROR(HTlookup(c -> ht, (void*)key, (void**)&slot));

  FCT_ERROR(getAccntInfo(c -> lsc, slot, a));
  
  return STATUS_OK;
  
}

/* both caches exist already when this operation is invoked */
/* we will go throught the elements of the source and insert them in the destination 
   - the elements in src will be removed if the dest already has the elements in the cache
   - if elements are pinpoined in the src, they should preserve their state in the dest (what about the access time?) Ordering?

*/
errorCode cacheCopy(cache * src, cache * dest){
  int key;
  void * data;
  int res;
  int slotSrc, slotDest;

  if (!src || !dest){
    PRINT("Source or destination cache are null\n");
    return STATUS_ERR;
  }
  
  createHTscan(src -> ht);

  while (HTadvanceScan(src -> ht, (void *)&key) != STATUS_ERR){
    FCT_FAIL(HTextract(src -> ht, (void*)key, (void **)&data), "Can not extract key in cacheCopy");
    slotSrc = (int)data;
    res = HTlookup(dest -> ht, (void*)key, (void **)&data);
    slotDest = (int)data;
    if (res == STATUS_OK){
      /* same key duplicated in src and dest 
	 - will move data from src to dest, updating the address pointers
	 - keeping the pinpoint information 
	 - access time will be the current access time
       */
      
      pageFree(ACCNT_DATA(dest -> lsc, slotDest));
      ACCNT_DATA(dest -> lsc, slotDest) = ACCNT_DATA(src -> lsc, slotSrc);
      ACCNT_DATA(src -> lsc, slotSrc) = NULL;

      /* update the usage slots, pinpoint access time */
      FCT_FAIL(moveAttrib(src -> lsc, dest -> lsc, (void*)key, slotSrc, slotDest), "Error transferring attributes");
    }
    else{
      FCT_FAIL(cacheLookupFailedInsertSlot(dest, (void*)key, (void **)&data, &slotDest), "Error update in cacheCopy\n");
      //ACCNT_DATA(src -> lsc, slotSrc) = NULL;
      ACCNT_DATA(dest -> lsc, slotDest) = ACCNT_DATA(src -> lsc, slotSrc);
      ACCNT_DATA(src -> lsc, slotSrc) = NULL;
      /* update the usage slots, pinpoint access time */
      FCT_FAIL(moveAttrib(src -> lsc, dest -> lsc, (void*)key, slotSrc, slotDest), "Error transferring attributes");
      //ACCNT_USED(src, slotSrc) = -1; /* unused */      
    }

  }

  destroyHTscan(src -> ht);

  return STATUS_OK;
  
}


errorCode cacheDestroy(cache ** c){
  destroyCache(& ((*c) -> lsc));
  deleteHashtable(& ((*c) -> ht ));

  memFree(*c);
  
  *c = NULL;
  
  return STATUS_OK;

}

