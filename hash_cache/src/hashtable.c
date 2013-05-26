/* 

   Hashtable implementation
   Author: Florentina Popovici 
   Filename: hashtable.c
   

*/


#include <hashtable.h>
#include <utils.h>


/* allocs a hash table entry, should not be used by the upper applications */
errorCode allocHashtableEntry(void * key, void * data, hashtableEntry ** hte){
  * hte = NULL;
  
  FCT_ERROR(memAlloc(sizeof(hashtableEntry), (void **)hte));
  (*hte) -> key = key;
  (*hte) -> data = data;
  (*hte) -> next = NULL;
  /* hte -> deallocFct = deallocFct; */
  return STATUS_OK;
}


/* not visible from outside */
errorCode deallocHashtableEntry(void * entry){
  if (entry == NULL)
    return STATUS_OK;
  
  memFree(entry);
  return STATUS_OK;
}


/* not visible from outside */
errorCode deallocHashtableEntryFreeKeyAndData(hashtable * ht, void * entry){
  hashtableEntry * hte = (hashtableEntry *) entry;

  if (entry == NULL)
    return STATUS_OK;
  
  if (hte -> key != NULL && ht -> deallocKey)
    (ht -> deallocKey)(hte -> key);

  if (hte -> key != NULL && ht -> deallocData)
    (ht -> deallocData)(hte -> data);
  
  memFree(entry);
  return STATUS_OK;
}


/* res will be 1 if the hashtable is empty */
errorCode HTempty(hashtable * ht, int * res){

  if (ht == NULL)
    return STATUS_ERR;
  else {
    *res = (ht -> size == 0);
    return STATUS_OK;
  }
}

int HTsize(hashtable * ht)
{
  if (ht == NULL)
    return STATUS_ERR;
  else 
    return ht->size;
}

/* search the hashtable for a non-null entry 
   as a side effect modifies the scan to point to the new position in the hashtable
   this is a local function
*/
errorCode scanForNonNullEntry(hashtable * ht){
  int notFound = 1;

  if (ht -> advance || 
      (!(ht -> advance) && 
       (ht -> crtEntry == NULL))){    
    while (notFound){
      if (ht -> crtEntry == NULL){
	/* need to go to the next entry in the table */
	ht -> crtEntryNr ++;
	if (ht -> crtEntryNr >= ht -> numberEntries){
	  /* we can not advance any more, return error */
	  return STATUS_ERR;
	}
	ht -> crtEntry = ht -> table[ht-> crtEntryNr].next;
      }
      else
	ht -> crtEntry = ht -> crtEntry -> next;
      
      notFound = (ht -> crtEntry == NULL);
      
    }
    ht -> advance = 1;
    return STATUS_OK;
  }
  else{
    ht -> advance = 1;

    if(ht -> crtEntry == NULL)
      return STATUS_ERR;
    else
      return STATUS_OK;
  }

}


errorCode createHTscan(hashtable * ht){
  (ht) -> scanActive = 1; 
  (ht) -> crtEntryNr = -1;
  (ht) -> crtEntry = NULL;
  (ht) -> advance = 1;

  return STATUS_OK;
  /* do not care for error code now, just use this to set it to the initial position */
  //scanForNonNullEntry(scan);
}


/* needs lock aquired here also */
errorCode HTadvanceScan(hashtable * ht, void ** key){
  int res = STATUS_ERR;
  
  res = scanForNonNullEntry(ht);  
  if (res == STATUS_OK)
    *key = ht -> crtEntry -> key;
  else
    *key = NULL;
    
  return res;
}


errorCode destroyHTscan(hashtable * ht){
  (ht) -> scanActive = 0;
  (ht) -> crtEntryNr = -1;
  (ht) -> crtEntry = NULL;
  (ht) -> advance = 1;
  
  return STATUS_OK;
}


/* returns 0 if the values are equal, 
   non-zero otherwise 

   Should be provided by the upper level, it is specific to the application

*/
#if 0
errorCode compareBlockNumber(void * k1, void * k2, int * result){
  blockNumber * key1 = (blockNumber*)k1, * key2 = (blockNumber*)k2;
  if (!key1 || !key2)
    return STATUS_ERR;
  if ((*(key1 -> blockNumber) == *(key2 -> blockNumber)))
    *result = 0;
  else
    *result = 1;
  return STATUS_OK;
}
#endif

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


errorCode createHashtable(int numberEntries, keyCompareFunction compare, deallocKeyFunction deallocK, deallocDataFunction deallocD, hashtable ** ht /* return value */){
  /* INA: what alloc function to use? */
  int i = 0;

  FCT_ERROR(memAlloc(sizeof(hashtable), (void**)ht));

  
  (*ht) -> scanActive = 0;
  (*ht) -> crtEntry = NULL;
  (*ht) -> crtEntryNr = -1;
  (*ht) -> advance = 1;
  (*ht) -> size = 0;
  (*ht) -> numberEntries = numberEntries;
  FCT_ERROR(memAlloc(numberEntries * sizeof(hashtableEntry), (void **)&((*ht) -> table)));
  /* initialize all next fields with NULL */
  for(i = 0; i < numberEntries; i++){
    (((*ht) -> table)[i]).next = NULL;
  }
  (*ht) -> compare = compare;
  (*ht) -> deallocKey = deallocK;
  (*ht) -> deallocData = deallocD;
  
  return STATUS_OK;
}


/* deallocator is a function that knows how to deallocate data and keys */
errorCode deleteHashtable(hashtable ** ht){
  int i;
  hashtableEntry * temp, *successor;;

  PRINT("%d\n", (*ht) -> numberEntries);
  //fflush(stdout);
  for(i = 0; i < (*ht) -> numberEntries; i++){
    PRINT("Deallocating %d\n", i);
    //fflush(stdout);
    temp = (((*ht) -> table)[i]).next;
    successor = NULL;
    while(temp != NULL){
      successor = temp -> next;
      deallocHashtableEntry(temp);
      temp = successor;
    }
  }
  memFree((*ht) -> table);
  memFree(*ht);
  *ht = NULL;

  return STATUS_OK;
}


errorCode deleteHashtableAndData(hashtable ** ht){
  int i;

  for(i = 0; i < (*ht) -> numberEntries; i++){
    hashtableEntry * temp = (((*ht) -> table)[i]).next, * successor = NULL;
    while(temp != NULL){
      successor = temp -> next;
      deallocHashtableEntryFreeKeyAndData(*ht, temp);
      temp = successor;
    }
  }
  memFree((*ht) -> table);
  memFree(*ht);
  *ht = NULL;

  return STATUS_OK;
}


/* need a good hash function here 
   This might have to change 
   
   We can have a different hash function for each type of key. 
   In this case the hash function will be a record in the hashtable struct

*/
errorCode hashFunction(hashtable * ht, void * key, int * entry){
  *entry = (((int)key) % (ht -> numberEntries));	
	return STATUS_OK;
}



/* tries  to insert and if key already present returns the old value of the data*/
errorCode HTupdate(hashtable * ht, void * key, void * data, void ** old_data){
  errorCode result = STATUS_ERR;
  hashtableEntry * predecessor = NULL;
  hashtableEntry * hte;
  int position;
  

  allocHashtableEntry(key, data, &hte);

  result = hashFunction(ht, hte -> key, &position); 
  FCT_ERROR(result);
  
  result = listLookup((ht -> table) + position, hte -> key, ht -> compare, &predecessor);	
  if (result == STATUS_OK){
    * old_data = predecessor -> next -> data;
    deallocHashtableEntry(hte);
    return STATUS_DUPL_ENTRY;
  }
  hte -> next = (ht -> table)[position].next; 
  (ht -> table)[position].next = hte;
  (ht -> size) ++;

  return STATUS_OK; 

}


/* insert the pair (key,data) in the hashtable. compare is the function used
   to compare two entries in the hash 
   don't insert if there is another entry with the same key
*/
errorCode HTinsert(hashtable * ht, void * key, void * data){
  errorCode result = STATUS_ERR;
  hashtableEntry * predecessor = NULL;
  hashtableEntry * hte;
  int position;
  

  allocHashtableEntry(key, data, &hte);

  result = hashFunction(ht, hte -> key, &position); 
  FCT_ERROR(result);
  
  result = listLookup((ht -> table) + position, hte -> key, ht -> compare, &predecessor);	
  if (result == STATUS_OK){
    deallocHashtableEntry(hte);
    return STATUS_DUPL_ENTRY;
  }
  hte -> next = (ht -> table)[position].next; 
  (ht -> table)[position].next = hte;
  (ht -> size) ++;

  return STATUS_OK;
}


/* 
   Delete the key and return the entry. 
   Do the data deallocation outside of this function 
*/
errorCode HTextract(hashtable * ht, void * key, void ** data){
  int entry = -1;
  errorCode result = STATUS_ERR;
  hashtableEntry * predecessor = NULL, * deletedEntry;
  errorCode resultComp = STATUS_ERR;


  result = hashFunction(ht, key, &entry);
  FCT_ERROR(result);
	/* lookup the key */
  
  result = listLookup((ht -> table) + entry, key, ht -> compare, &predecessor);
  if (result == STATUS_OK){
    /* the key exists */
    
    if (ht -> scanActive && ht -> crtEntry){
      FCT_ERROR(ht -> compare(ht -> crtEntry -> key, key, &resultComp));
      /* if we remove the element that is current for the scan, then we need to update the scan */
      if (!resultComp){
	/* advance the scan */
	ht -> advance = 0;
	ht -> crtEntry = predecessor -> next -> next;
      }
    }

    deletedEntry = predecessor -> next;
    * data = deletedEntry -> data;
    predecessor -> next = deletedEntry -> next;
    (ht -> size) --;
    deallocHashtableEntry(deletedEntry);    
  }
  return result;
}


errorCode HTlookup(hashtable * ht, void * key, void ** data){
  int entry = -1;
  errorCode result = STATUS_ERR;
  hashtableEntry * predecessor = NULL, * lookupEntry;
  

  *data = NULL;
  result = hashFunction(ht, key, &entry);
  FCT_FAIL(result, "Error hash");
	/* lookup the key */
  
  result = listLookup((ht -> table) + entry, key, ht -> compare, &predecessor);
  if (result == STATUS_OK){
    /* the key exists */
    lookupEntry = predecessor -> next;
    * data = lookupEntry -> data;
    //errorCode = deallocHashtableEntry(l);    
  }
  return result;
}


/* Helper function, looks for key in a hashtable entry, using the specified compare functiona and returneing the predecessor */
errorCode listLookup(hashtableEntry * start, void * key,  keyCompareFunction 
		  compare, hashtableEntry ** predecessor){
  errorCode result = STATUS_ERR;
  int resultCompare;
  *predecessor = start;
  

  start = start -> next;
  while(start != NULL){
    FCT_ERROR(compare(key, start -> key, &resultCompare));
    if (resultCompare == 0){
      result = STATUS_OK;
      return result;
    }
    *predecessor = start;
    start = start -> next;
  }
  return STATUS_ERR; 
}


/* Looks for key in a hashtable entry */
errorCode lookup(hashtable * ht, void * key, hashtableEntry ** hte){
  int entry = -1;
  errorCode result = STATUS_ERR;
  hashtableEntry * predecessor = NULL;
  *hte = NULL;

  result = hashFunction(ht, key, &entry);
  FCT_ERROR(result);
  
  result = listLookup((ht -> table) + entry, key, ht -> compare, &predecessor); 
  if (result == STATUS_OK)
    *hte = predecessor -> next;
  return result;
 
}


#ifndef __KERNEL__



void printHashtableContent(FILE * f, hashtable * ht, char * str){
  int i = 0;

  fprintf(f, "%s\n", str);
  
  for (i = 0; i < ht -> numberEntries; i++){
    fprintf(f, "Entry %d\n", i);
    printAddressEntries(f, ht, i);
    fprintf(f, "\n");
  }
}

/* prints all the structs that are linked from the entry */
void printAddressEntries(FILE * f, hashtable * ht, int entry){
  hashtableEntry * hte;

  if (entry >= ht -> numberEntries)
    return;
  hte = (ht -> table + entry) -> next;

  fprintf(f, "Link of structures from entry %d\n", entry);
  while(hte != NULL){
    printBlockNumberAndAddressEntry(f, hte);
    hte = hte -> next;
  }
  
}

void printBlockNumberAndAddressEntry(FILE * f, hashtableEntry * hte){
  fprintf(f, "Hashtable entry: \n");
  if (hte == NULL)
    return;
  printBlockNumberKey(f, (blockNumber *)(hte -> key));
  printAddressEntry(f, (addressEntry *)(hte -> data));
  fprintf(f, "\n");
}


void printAddressEntry(FILE * f, addressEntry * data){
  fprintf(f, "\tAddress int data: \n");
  /*
    We need to print integer values of 0 also
  if (data == NULL)
    return;
  */
  fprintf(f, "\t\t%d", ((int)data));
  fprintf(f, "\t\t?DATA?\n");
  fflush(f);
  
}

		
void printBlockNumberKey(FILE * f, blockNumber * key){
  fprintf(f, "\tBlock Number int Key: \n");
  /*
    want to account for int keys that can be 0
  if (key == NULL)
    return;
  */
  fprintf(f, "\t\t%d", (int)(key));
  fflush(f);
}


#else

void printHashtableContent(hashtable * ht, char * str){
  int i = 0;

  printk("%s\n", str);
  for (i = 0; i < ht -> numberEntries; i++){
    //printk("Entry %d\n", i);
    printAddressEntries(ht, i);
  }
  printk("\n");
}



void printAddressEntry(addressEntry * data){
  printk( "\tAddress int data: \n");
  /*
    We need to print integer values of 0 also
  if (data == NULL)
    return;
  */
  printk( "\t\t%d", ((int)data));
  printk( "\t\t?DATA?\n");
  
}

		
void printBlockNumberKey(blockNumber * key){
  //printk( "\tBlock Number int Key: \n");
  /*
    want to account for int keys that can be 0
  if (key == NULL)
    return;
  */
  printk( "%d ", (int)(key));
}


void printBlockNumberAndAddressEntry(hashtableEntry * hte){
  //printk( "Hashtable entry: \n");
  if (hte == NULL)
    return;
  printBlockNumberKey((blockNumber *)(hte -> key));
  //printAddressEntry(f, (addressEntry *)(hte -> data));
}



/* prints all the structs that are linked from the entry */
void printAddressEntries(hashtable * ht, int entry){
  hashtableEntry * hte;

  if (entry >= ht -> numberEntries)
    return;
  hte = (ht -> table + entry) -> next;

  //printk( "Link of structures from entry %d\n", entry);
  while(hte != NULL){
    printBlockNumberAndAddressEntry(hte);
    hte = hte -> next;
  }
  
  //printk("\n");
}


#endif
