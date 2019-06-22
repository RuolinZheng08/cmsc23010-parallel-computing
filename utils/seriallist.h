#ifndef SERIALLIST_H_
#define SERIALLIST_H_

#include <stdbool.h>
#include "packetsource.h"

typedef struct Item {
	int key;
	 volatile Packet_t * value;
	struct Item * next;
}Item_t;

typedef struct {
	int size;
	Item_t * head;
}SerialList_t;

SerialList_t * createSerialList();

SerialList_t *  createSerialListWithItem(int key,  volatile Packet_t * value);

Item_t * getItem_list(SerialList_t * list, int key);

bool contains_list(SerialList_t * list, int key);

bool remove_list(SerialList_t * list, int key);

void add_list(SerialList_t * list, int key, volatile Packet_t * value);

void addNoCheck_list(SerialList_t * list, int key, volatile Packet_t * value);

void print_list(SerialList_t * list);

#endif /* SERIALLIST_H_ */
