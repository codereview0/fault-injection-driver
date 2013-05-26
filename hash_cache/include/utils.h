/*

   Author: Florentina Popovici 
   Filename: utils.h

*/


#ifndef __UTILS_H_
#define __UTILS_H_

#ifdef __KERNEL__

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/time.h> /* for time definitions and do_gettimeofday */
#include <linux/mm.h> /* for get_free_page */

#else

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h> 
#include <string.h>
#include <sys/time.h>

#endif

#ifdef __KERNEL__
#define PRINT printk
#else
#define PRINT printf
#endif

/* Error codes */
#define STATUS_OK 1
#define STATUS_ERR -1
#define STATUS_DUPL_ENTRY -2

typedef int errorCode;

#define CACHE_BLOCK_SIZE 4096
//#define CACHE_BLOCK_SIZE 1024

#ifndef INT_MAX
#define INT_MAX         ((int)(~0U>>1))
#endif

#ifdef __KERNEL__
#define GETTIMEOFDAY(t) do_gettimeofday(t)
#else
#define GETTIMEOFDAY(t) gettimeofday(t, NULL)
#endif


#ifdef DEBUG
#define PRINT_INFO(x,y,z) printInfo(x,y,z)
#else
#define PRINT_INFO(x,y,z)
#endif

#ifndef __KERNEL__

#define FCT_ERROR(errorCode)    if(errorCode != STATUS_OK){\
                                fflush(stderr);\
                               return errorCode;\
                             } 

#define FCT_FAIL(errorCode, str)    if(errorCode != STATUS_OK){\
                               fprintf(stderr, "%s\n", str);\
                               fflush(stderr);\
                             } 

#define FAIL(errorCode, str)    if(errorCode == STATUS_ERR){\
                               fprintf(stderr, "%s\n", str);\
                               fflush(stderr);\
                               exit(1);\
                             } 

#else

#define FCT_ERROR(errorCode)    if(errorCode != STATUS_OK){\
                               return errorCode;\
                             } 

#define FCT_FAIL(errorCode, str)    if(errorCode != STATUS_OK){\
                               printk("%s\n", str);\
                               return errorCode;\
                             } 

#define FAIL(errorCode, str)    if(errorCode == STATUS_ERR){\
                               printk("%s\n", str);\
                             } 

#endif

errorCode pageAlloc(int size, void ** ptr);
errorCode pageFree(void *);
errorCode memAlloc(int size, void ** ptr);
errorCode memFree(void *);

#ifndef __KERNEL__
void  printWarning(const char *format, /* args */ ...);
void  printError(const char *format, /* args */ ...);
void printInfo(char * name, int count, int * values);
#endif

#endif


