  

#ifndef AVUTIL_MEM_H
#define AVUTIL_MEM_H

 
void *av_malloc(unsigned int size);

 
void *av_realloc(void *ptr, unsigned int size);

 
void av_free(void *ptr);

 
void *av_mallocz(unsigned int size);

 
char *av_strdup(const char *s);

 
void av_freep(void *ptr);

#endif /* AVUTIL_MEM_H */
