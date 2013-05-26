/* 

   Test file for history-related operations (test the hash table)
   Author: Florentina Popovici 
   Filename: testHash.c
   

*/
#include <stdlib.h>
#include "hashtable.h"


int main(void){
  
  /* create a hash table with values */
  hashtable * ht = NULL;
  int numberEntries = 1024;
  errorCode result;
  int values = 100;
  int i = 0;
  int data;
  int key, empty;

  /* create hashtable */
  result =  createHashtable(numberEntries, compareBlockNumberNoPointer, NULL, NULL, &ht);
  FAIL(result, "Error allocating hashtable");

  result = HTinsert(ht, 100, i * 10);
  FAIL(result,"Error inserting");

  printHashtableContent(stdout, ht, "");

  result = HTextract(ht, 100, &data);
  FAIL(result,"Error deleting");

  /* alloc entries for DG hashtable */
  for (i = 0 ; i < 5000; i++){    
    result = HTinsert(ht, i * 100, i * 10);
    FAIL(result,"Error inserting");
  }

  for (i = 0 ; i < 5000; i++){    
    result = HTextract(ht, i * 100, &data);
    FAIL(result,"Error deleting");
  }

//  printHashtableContent(stdout, ht, "");

  return 0;



  printf("Data extracted %d\n", data);

  printHashtableContent(stdout, ht, "");

  result = HTlookup(ht, 100, &data);
  
  printf("return of lookup %d\n", result);
  fflush(stdout);

  createHTscan(ht);

  while (HTadvanceScan(ht, &key) != STATUS_ERR){
    HTlookup(ht, key, &data);
    printf("Lookup %d\n", data);
    HTextract(ht, key, &data);
    printf("Extracted %d\n", data);
  }

  printHashtableContent(stdout, ht, "");

  destroyHTscan(ht);

  HTempty(ht, &empty);

  printf("creating scan again %d\n", empty);

  //deleteHashtable(&ht);
  
  createHTscan(ht);

  while (HTadvanceScan(ht, &key) != STATUS_ERR){
    result = HTlookup(ht, key, &data);
    printf("Lookup %d %d\n", data, result);
    fflush(stdout);
    result = HTextract(ht, key, &data);
    printf("Extracted %d %d\n", data, result);
    fflush(stdout);
  }
    
  destroyHTscan(ht);
  
  printf("destroying scan again\n");
  
  fflush(stdout);

  if (ht == NULL){
    printf("ht null \n");
    fflush(stdout);
  }

  deleteHashtable(&ht);


  /*
  result = HTextract(ht, 200, &data);
  FAIL(result,"Error deleting");

  printf("Data extracted %d\n", data);

  printHashtableContent(stdout, ht);

  result = HTlookup(ht, 300, &data);
  if (result == STATUS_OK){
    printf("Data found %d\n", data);
  }
  else
    printf("Data not found\n");

  result = HTextract(ht, 300, &data);
  FAIL(result,"Error deleting");

  printf("Data extracted %d\n", data);

  
    result = HTlookup(ht, 300, &data);
  if (result == STATUS_OK){
    printf("Data found %d\n", data);
  }
  else
    printf("K not found 300\n");
  

  result = HTlookup(ht, 100, &data);
  if (result == STATUS_OK){
    printf("Data found %d\n", data);
  }
  else
    printf("Key not found 100\n");

  */

  return 0;
}



