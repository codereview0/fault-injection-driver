/* 

   Several handy functions: alloc/dealloc memory, print error messages
   Author: Florentina Popovici 
   Filename: utils.c
   

*/


#include "utils.h"


#ifndef __KERNEL__

void printInfo(char * name, int count, int * values){
  int i;

  fprintf(stdout, "Values for %s\n", name);

  for(i = 0; i < count; i++){
    fprintf(stdout, "  %s[%d] = %d\n", name, i, values[i]);
  }
  
  fprintf(stdout,"\n");
  fflush(stdout);

}

errorCode pageAlloc(int size, void ** ptr){
  return memAlloc(size, ptr);
}


errorCode pageFree(void * p){
  return memFree(p);
}


errorCode memAlloc(int size, void ** ptr){
  * ptr = NULL;

  * ptr = malloc(size);
 
  if (*ptr == NULL){
    //printError("%s\n", "not enough memory to allocate");
    return STATUS_ERR;
    /* INA: for now exit. In general we need another way to recover from this*/
    // exit(1);
  }
  return STATUS_OK;
}

errorCode memFree(void * ptr){
  if (ptr == NULL){
    printError("%s\n", "trying to deallocate a NULL pointer");
    return STATUS_ERR;
    /* INA: for now exit. In general we need another way to recover from this*/
    // exit(1);
  }
  free(ptr);
  return STATUS_OK;
}

/* 
   INA:
   fatal error, print an error message and exit
*/
void  printError(const char *format, /* args */ ...){
  int rc;
  va_list     args;

  /*
  if (verbose == 0)
    return 0;
  */

  va_start(args, format);
  /* normal printf */
  rc = fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, args);
  fflush(stderr);
  va_end(args);
  
  //exit(1);
}

void printWarning(const char *format, /* args */ ...){
  int         rc;
  va_list     args;

  /*
  if (verbose == 0)
    return 0;
  */

  va_start(args, format);
  /* normal printf */
  fprintf(stderr, "WARNING: ");
  rc = vfprintf(stderr, format, args);
  fflush(stderr);
  va_end(args);
  
}


#else


errorCode pageAlloc(int size, void ** ptr){
  * ptr = NULL;

  *ptr = get_free_page(GFP_KERNEL);
 
  if (*ptr == NULL){
    printk("%s\n", "not enough memory to allocate");
    return STATUS_ERR;
    /* INA: for now exit. In general we need another way to recover from this*/
    // exit(1);
  }
  return STATUS_OK;
}


errorCode pageFree(void * ptr){
  if (ptr == NULL){
    printk("%s\n", "trying to deallocate a NULL pointer");
    return STATUS_ERR;
    /* INA: for now exit. In general we need another way to recover from this*/
    // exit(1);
  }
  //kfree(ptr);
  free_page(ptr);
  return STATUS_OK;
}



errorCode memAlloc(int size, void ** ptr){
  * ptr = NULL;

  * ptr = kmalloc(size, GFP_KERNEL);
 
  if (*ptr == NULL){
    printk("%s\n", "not enough memory to allocate");
    return STATUS_ERR;
    /* INA: for now exit. In general we need another way to recover from this*/
    // exit(1);
  }
  return STATUS_OK;
}

errorCode memFree(void* ptr){
  if (ptr == NULL){
    printk("%s\n", "trying to deallocate a NULL pointer");
    return STATUS_ERR;
    /* INA: for now exit. In general we need another way to recover from this*/
    // exit(1);
  }
  kfree(ptr);
  //free_page(ptr);
  return STATUS_OK;
}


#endif
