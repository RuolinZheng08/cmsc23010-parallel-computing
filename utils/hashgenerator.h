/*
 * hashpacket.h
 *
 *  Created on: May 2, 2013
 *      Author: lamont
 */

#ifndef HASHPACKET_H_
#define HASHPACKET_H_

#include "packetsource.h"
#include "generators.h"

enum PacketType {Add,Remove,Contains};

typedef struct {
	  int key;
	  volatile Packet_t * body;
	  enum PacketType type;
}HashPacket_t;

typedef struct {
	RandomGenerator_t  rand;
	float hitRate;
	float drunkWalk;
	int key;
}HashKeyGenerator_t;

typedef struct {
	HashKeyGenerator_t * head;
	int currentHead;
	HashKeyGenerator_t * tail;
	int currentTail;
	RandomGenerator_t  rand;
	int totalPackets;
	int fractionAdd;
	int fractionRemove;
	long mean;
}HashPacketGenerator_t;

HashPacket_t * createHashPacket(int key,  volatile Packet_t * body, enum PacketType type);

int mangleKey(HashPacket_t * hashPacket);
void printPacket(HashPacket_t * hashPacket);

HashPacketGenerator_t * createHashPacketGenerator(float fractionAdd,
	    										  float fractionRemove,
	    										  float hitRate,
	    										  long mean);
HashPacket_t *  getRandomPacket(HashPacketGenerator_t * gen);

HashPacket_t *  getAddPacket(HashPacketGenerator_t * gen);

HashPacket_t *  getRemovePacket(HashPacketGenerator_t * gen);

HashPacket_t *  getContainsPacket(HashPacketGenerator_t * gen);

int getKey(HashKeyGenerator_t * gen);


#endif /* HASHPACKET_H_ */
