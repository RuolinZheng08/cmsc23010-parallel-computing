
#include<stdlib.h>
#include <stdio.h>
#include "seriallist.h"

SerialList_t *  createSerialList()
{
	SerialList_t * list = (SerialList_t *)malloc(sizeof(SerialList_t));
	list->size = 0;
	list->head = NULL;

	return list;
}

SerialList_t *  createSerialListWithItem(int key, volatile Packet_t * value)
{
	SerialList_t * list = (SerialList_t *)malloc(sizeof(SerialList_t));
	list->size = 1;
	Item_t * newItem = (Item_t *)malloc(sizeof(Item_t));
	newItem->key = key;
	newItem->value = value;
	newItem->next = NULL;
	list->head = newItem;

	return list;
}

Item_t * getItem_list(SerialList_t * list, int key){

	Item_t * curr = list->head;

	while(curr != NULL){
		if(curr->key == key)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

bool contains_list(SerialList_t * list, int key){
	return getItem_list(list,key) != NULL;
}

bool remove_list(SerialList_t * list, int key){
	if(!contains_list(list,key))
		return false;

	Item_t * curr = list->head;

	if(curr == NULL)
		return false;
	else if (curr->key ==key){
		Item_t * temp = curr;
		list->head = list->head->next;
		free(temp);
		list->size--;
		return true;
	}else{
		while(curr->next != NULL) {
			if(curr->next->key == key){
				Item_t * temp = curr->next;
				curr->next = curr->next->next;
				free(temp);
				list->size--;
				return true;
			}
			else
				curr = curr->next;
		}
	}
	return false;
}
void add_list(SerialList_t * list, int key, volatile Packet_t * value){
	Item_t * tmpItem = getItem_list(list,key);

	if(tmpItem != NULL){
		tmpItem->value = value;
	}else{
		Item_t * newItem = (Item_t *)malloc(sizeof(Item_t));
		newItem->key = key;
		newItem->value = value;
		newItem->next = list->head;
		list->head = newItem;
		list->size++;
	}
}
void addNoCheck_list(SerialList_t * list, int key, volatile Packet_t * value){

	Item_t * newItem = (Item_t *)malloc(sizeof(Item_t));
	newItem->key = key;
	newItem->value = value;
	newItem->next = list->head;
	list->head = newItem;
	list->size++;
}

void print_list(SerialList_t * list){
	Item_t * curr = list->head;

	while(curr != NULL){
		printf("addr:%p\tkey:%d\tvalue:%p\n",curr,curr->key,curr->value);
		curr = curr->next;
	}
}
