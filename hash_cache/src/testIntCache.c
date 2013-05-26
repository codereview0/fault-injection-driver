/* 

   Test file for history-related operations (test the hash table)
   Author: Florentina Popovici 
   Filename: testHash.c
   

*/
#include <stdlib.h>
#include "cache.h"
#include <string.h>

int main(void){  
  cache * c = NULL;
  int data;
  int * lookupData;
  int result, i;

  printf("%d\n", (int)c);
  fflush(stdout);

  result = cacheCreate(3, 3, compareBlockNumberNoPointer, NULL, NULL, &c);
  FAIL(result, "Error allocating cache");
  
  /* alloc entries for cache */
  for (i = 0 ; i < 3; i++){    
    data = i;

    result = cacheInsert(c, i * 100, &data);
    FAIL(result,"Error inserting");
  }

  result = cacheLookup(c, 200, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %d\n", *lookupData);
  result = cacheLookup(c, 200, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %d\n", *lookupData);
  result = cacheLookup(c, 0, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %d\n", *lookupData);
  result = cacheLookup(c, 0, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %d\n", *lookupData);

  printInfoLSCache(stdout, c -> lsc);

  data = 111;
  result = cacheInsert(c, 300, &data);
  printInfoLSCache(stdout, c -> lsc);

  fflush(stdout);

  return 0;
}



