/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#include "../include/List.h"
 

typedef struct tagIS
{
	struct tagIS *next;
	void *data;
} ItemSlot;

struct _tag_array
{
	struct tagIS *head;
	struct tagIS *tail;
	unsigned int entryCount;
	int foundEntryNumber;
	struct tagIS *foundEntry;
};

Suntech_List * suntech_list_new()
{
	Suntech_List *nlist = (Suntech_List *) malloc(sizeof(Suntech_List));

	if (! nlist)
		return 0;

	nlist->head = nlist->foundEntry = 0;
	nlist->tail = 0;
	nlist->foundEntryNumber = -1;
	nlist->entryCount = 0;

	return nlist;
}

void suntech_list_del(Suntech_List *ptr)
{
	if (!ptr)
		return;

	while (ptr->entryCount) suntech_list_rem(ptr, 0);

	free(ptr);
}

unsigned int suntech_list_count(Suntech_List *ptr)
{
	if (! ptr)
		return 0;

	return ptr->entryCount;
}

SUNTECH_Err suntech_list_add(Suntech_List *ptr, void* item)
{
	ItemSlot *entry;
    if (! ptr)
    	return SUNTECH_BAD_PARAM;

	entry = (ItemSlot *) malloc(sizeof(ItemSlot));
	if (!entry)
		return SUNTECH_OUT_OF_MEM;

	entry->data = item;
	entry->next = 0;

	if (! ptr->head)
	{
		ptr->head = entry;
		ptr->entryCount = 1;
	}
	else
	{
		ptr->entryCount += 1;
		ptr->tail->next = entry;
	}
	ptr->tail = entry;
	ptr->foundEntryNumber = ptr->entryCount - 1;
	ptr->foundEntry = entry;

	return SUNTECH_OK;
}

SUNTECH_Err suntech_list_rem(Suntech_List *ptr, unsigned int itemNumber)
{
	ItemSlot *tmp, *tmp2;
	unsigned int i;

	//if head is null (empty list)
	if ( (! ptr) || (! ptr->head) || (ptr->head && !ptr->entryCount) || (itemNumber >= ptr->entryCount) )
		return SUNTECH_BAD_PARAM;

	//we delete the head
	if (! itemNumber)
	{
		tmp = ptr->head;
		ptr->head = ptr->head->next;
		ptr->entryCount --;
		ptr->foundEntry = ptr->head;
		ptr->foundEntryNumber = 0;
		free(tmp);
		//that was the last entry, reset the tail
		if (!ptr->entryCount)
		{
			ptr->tail = ptr->head = ptr->foundEntry = 0;
			ptr->foundEntryNumber = -1;
		}
		return SUNTECH_OK;
	}

	tmp = ptr->head;
	i = 0;
	while (i < itemNumber - 1)
	{
		tmp = tmp->next;
		i++;
	}
	tmp2 = tmp->next;
	tmp->next = tmp2->next;
	//if we deleted the last entry, update the tail
	if (! tmp->next || (ptr->tail == tmp2) )
	{
		ptr->tail = tmp;
		tmp->next = 0;
	}

	free(tmp2);
	ptr->entryCount --;
	ptr->foundEntry = ptr->head;
	ptr->foundEntryNumber = 0;

	return SUNTECH_OK;
}

void *suntech_list_get(Suntech_List *ptr, unsigned int itemNumber)
{
	ItemSlot *entry;
	unsigned int i;

	if ((! ptr) || (itemNumber >= ptr->entryCount) )
		return 0;

	if ( itemNumber < (unsigned int) ptr->foundEntryNumber )
	{
		ptr->foundEntryNumber = 0;
		ptr->foundEntry = ptr->head;
	}
	entry = ptr->foundEntry;
	for (i = ptr->foundEntryNumber; i < itemNumber; i++ )
	{
		entry = entry->next;
	}
	ptr->foundEntryNumber = itemNumber;
	ptr->foundEntry = entry;

	return (void *) entry->data;
}

int suntech_list_del_item(Suntech_List *ptr, void *item)
{
	int i = suntech_list_find(ptr, item);
	if (i>=0) suntech_list_rem(ptr, (unsigned int) i);

	return i;
}

int suntech_list_find(Suntech_List *ptr, void *item)
{
	unsigned int i, count;
	count = suntech_list_count(ptr);
	for (i=0; i<count; i++)
	{
		if (suntech_list_get(ptr, i) == item)
			return (int) i;
	}

	return -1;
}

