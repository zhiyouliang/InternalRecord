/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#ifndef LIST_H_
#define LIST_H_



#ifdef __cplusplus extern "C" {
#endif
 
#include "../include/def.h"


typedef struct _tag_array Suntech_List;

//list constructor
Suntech_List *suntech_list_new();

//list destructor
void suntech_list_del(Suntech_List *ptr);

//get count
unsigned int suntech_list_count(Suntech_List *ptr);

//add item
SUNTECH_Err suntech_list_add(Suntech_List *ptr, void* item);

//removes item
SUNTECH_Err suntech_list_rem(Suntech_List *ptr, unsigned int position);

//gets item
void *suntech_list_get(Suntech_List *ptr, unsigned int position);

//finds item
int suntech_list_find(Suntech_List *ptr, void *item);

//deletes item
int suntech_list_del_item(Suntech_List *ptr, void *item);
 
#ifdef __cplusplus }
#endif


#endif /* LIST_H_ */
