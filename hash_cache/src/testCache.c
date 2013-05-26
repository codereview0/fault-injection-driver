/* 

   Test file for cache-hash operations
   Author: Florentina Popovici 
   Filename: testCache.c
   

*/
#include <stdlib.h>
#include "cache.h"
#include <string.h>

int main(void){  
  cache * c = NULL, * c2 = NULL;
  char data[20][CACHE_BLOCK_SIZE];
  char * lookupData;
  int result, i;
  void * address;

  result = cacheCreate(3, 3, compareBlockNumberNoPointer, NULL, NULL, &c);
  FAIL(result, "Error allocating cache");
  
  /* alloc entries for cache */
  for (i = 0 ; i < 3; i++){    
    sprintf(data[i], "%4d %4d\n", i, i);

    result = cacheInsert(c, i * 100, data[i]);
    FAIL(result,"Error inserting");
  }
  
  for (i = 0 ; i < 3; i++){    
    sprintf(data[i], "%4d %4d\n", i, i);

    result = cacheLookup(c, i * 100, &address);
    printf("%d %p\n", i * 100, address);
    FAIL(result,"Error Lookup");
  }

  printInfoLSCache(stdout, c -> lsc);

  result = cacheCreate(5, 5, compareBlockNumberNoPointer, NULL, NULL, &c2);
  FAIL(result, "Error allocating cache");
  
  /* alloc entries for cache */
  for (i = 1 ; i < 5; i++){    
    sprintf(data[i], "%4d %4d\n", i * 10, i * 10);
    
    result = cacheInsert(c2, i * 100, data[i]);
    FAIL(result,"Error inserting");
  }

  for (i = 1 ; i < 5; i++){    
    sprintf(data[i], "%4d %4d\n", i * 10, i * 10);
    
    result = cacheLookup(c2, i * 100, &address);
    printf("%d %p\n", i * 100, address);
    FAIL(result,"Error inserting");
  }


  //printInfoLSCache(stdout, c2 -> lsc);

  //cachePinpoint(c, 100);


  cacheCopy(c, c2);
  
  for (i = 0 ; i < 5; i++){    
    sprintf(data[i], "%4d %4d\n", i * 10, i * 10);
    
    result = cacheLookup(c2, i * 100, &address);
    printf("%d %p\n", i * 100, address);
    FAIL(result,"Error inserting");
  }



  printf("c\n");
  printInfoLSCache(stdout, c -> lsc);
  printf("c2\n");
  printInfoLSCache(stdout, c2 -> lsc);

  return 0;

  result = cacheLookup(c2, 200, &lookupData);

  printf("data %10s\n", lookupData);

  result = cacheLookup(c2, 400, &lookupData);

  printf("data %10s\n", lookupData);
  
  
  result = cacheLookup(c2, 100, &lookupData);

  printf("data %10s\n", lookupData);
  
  cacheDestroy(&c2);
  cacheDestroy(&c);

  return 0;

  result = cacheLookup(c, 0, &address);
  
  printf("%d\n", result);
  fflush(stdout);

  FCT_FAIL(result, "Error lookup");

  sprintf(address, "%4d %4d\n", 5, 5);

  result = cacheLookup(c, 0, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %10s\n", lookupData);
  printInfoLSCache(stdout, c -> lsc);

  printInfoLSCache(stdout, c -> lsc);
  sprintf(data[3], "%4d %4d\n", 3, 3);  
  result = cacheInsert(c, 300, data[3]);
  
  result = cacheLookup(c, 300, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %10s\n", lookupData);

  //printInfoLSCache(stdout, c -> lsc);

  sprintf(data[3], "%4d %4d\n", 10, 10);  
  result = cacheInsert(c, 300, data[3]);

  //printInfoLSCache(stdout, c -> lsc);

  result = cacheLookup(c, 300, &lookupData);
  FAIL(result,"Error lookup");
  printf("data %10s\n", lookupData);

  return 0;

  cacheUnPinpoint(c, 100);
  printInfoLSCache(stdout, c -> lsc);
  sprintf(data[3], "%4d %4d\n", 4, 4);  
  result = cacheInsert(c, 400, data[4]);
  
  printInfoLSCache(stdout, c -> lsc);
  
  fflush(stdout);

  return 0;
}



