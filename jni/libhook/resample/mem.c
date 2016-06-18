  

#include <limits.h>
#include <stdlib.h>
#include <string.h>
 

#include "mem.h"

/* here we can use OS-dependent allocation functions */
#undef free
#undef malloc
#undef realloc

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

void *av_malloc(unsigned int size)
{
    void *ptr = NULL;
 
    /* let's disallow possible ambiguous cases */
    if(size > (INT_MAX-16) )
        return NULL;
 
    ptr = malloc(size);
     return ptr;
}

void *av_realloc(void *ptr, unsigned int size)
{ 
    return realloc(ptr, size);
 
}

void av_free(void *ptr)
{
     
    if (ptr) 
        free(ptr);
 
}

void av_freep(void *arg)
{
    void **ptr= (void**)arg;
    av_free(*ptr);
    *ptr = NULL;
}

void *av_mallocz(unsigned int size)
{
    void *ptr = av_malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

char *av_strdup(const char *s)
{
    char *ptr= NULL;
    if(s){
        int len = strlen(s) + 1;
        ptr = av_malloc(len);
        if (ptr)
            memcpy(ptr, s, len);
    }
    return ptr;
}

