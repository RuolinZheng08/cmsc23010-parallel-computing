
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashgenerator.h"

const float DENOMINATOR = 1048575; // 2^20 - 1

int mangleKey(HashPacket_t * hashPacket){
    const int CRC_POLY = 954680065; // 0x38E74301 - standard CRC30 from CDMA
    const int iterations = 32;
    int crc = hashPacket->key;
    for( int i = 0; i < iterations; i++ ) {
      if( ( crc & 1 ) > 0 )
        crc = (crc >> 1) ^ CRC_POLY;
      else
        crc = crc >> 1;
    }
    return crc;

}
void printPacket(HashPacket_t * hashPacket) {
    if( hashPacket->type == Add )
      printf("Add Packet: Key = %d Body=%p\n",hashPacket->key, hashPacket->body);
    else if( hashPacket->type == Remove )
        printf("Remove Packet: Key = %d Body=%p\n",hashPacket->key, hashPacket->body);
    else
        printf("Contains Packet: Key = %d Body=%p\n",hashPacket->key, hashPacket->body);
 }


HashPacketGenerator_t * createHashPacketGenerator(float fractionAdd,
	    										  float fractionRemove,
	    										  float hitRate,
	    										  long mean)
{
	HashPacketGenerator_t * gen = (HashPacketGenerator_t *)malloc(sizeof(HashPacketGenerator_t));

	gen->currentHead = 0;
	gen->currentTail = 0;
	gen->totalPackets = 0;
	gen->fractionAdd = (int)floor(fractionAdd * DENOMINATOR + 0.5f);
	gen->fractionRemove = (int)floor(fractionRemove * DENOMINATOR + 0.5f);
	gen->head = (HashKeyGenerator_t *)malloc(sizeof(HashKeyGenerator_t));
	gen->head->hitRate = hitRate;
	gen->head->drunkWalk = 1.0f;
	gen->head->key = 1;
	initGenerator(&gen->head->rand);
	gen->tail = (HashKeyGenerator_t *)malloc(sizeof(HashKeyGenerator_t));
	gen->tail->hitRate = hitRate;
	gen->tail->drunkWalk = 1.0f;
	gen->tail->key = 1;
	initGenerator(&gen->tail->rand);
	gen->mean = mean;
	initGenerator(&gen->rand);

	return gen;
}
HashPacket_t *  getRandomPacket(HashPacketGenerator_t * gen){

	 int tmp = ((int)updateRand(&gen->rand)) & (int)DENOMINATOR;

	 gen->totalPackets++;

	 if( ( tmp < gen->fractionAdd ) || ( gen->currentHead <= gen->currentTail ) )
	      return getAddPacket(gen);
	 else if( tmp < (gen->fractionAdd + gen->fractionRemove))
	      return getRemovePacket(gen);
	 else
		 return getContainsPacket(gen);
}

HashPacket_t *  getAddPacket(HashPacketGenerator_t * gen){
	HashPacket_t * pkt = (HashPacket_t*)malloc(sizeof(HashPacket_t));
	pkt->type = Add;
	gen->currentHead = getKey(gen->head);
	pkt->key = gen->currentHead;
	pkt->body = (Packet_t *)malloc(sizeof(Packet_t));
	pkt->body->iterations = updateRand(&gen->rand) % gen->mean;
	pkt->body->seed = updateRand(&gen->rand) % gen->mean;

	return pkt;
}

HashPacket_t *  getRemovePacket(HashPacketGenerator_t * gen){
	HashPacket_t * pkt = (HashPacket_t*)malloc(sizeof(HashPacket_t));
	pkt->type = Remove;
	gen->currentTail = getKey(gen->tail);
	pkt->key = gen->currentTail;
	pkt->body = (Packet_t *)malloc(sizeof(Packet_t));
	pkt->body->iterations = updateRand(&gen->rand) % gen->mean;
	pkt->body->seed = updateRand(&gen->rand) % gen->mean;

	return pkt;
}

HashPacket_t *  getContainsPacket(HashPacketGenerator_t * gen){
	HashPacket_t * pkt = (HashPacket_t*)malloc(sizeof(HashPacket_t));
    const long THIRTY_ONE_BITS = 2147483647; // 2^32 - 1
    int key = (int) ( THIRTY_ONE_BITS & updateRand(&gen->rand) );
    key = ( key % ( gen->currentHead - gen->currentTail + 1 ) ) + gen->currentTail;
	pkt->type = Contains;
	pkt->key = key;
	pkt->body = (Packet_t *)malloc(sizeof(Packet_t));
	pkt->body->iterations = updateRand(&gen->rand) % gen->mean;
	pkt->body->seed = updateRand(&gen->rand) % gen->mean;

	return pkt;

}
int getKey(HashKeyGenerator_t * gen){

    while(gen->drunkWalk < 0 ) {
      gen->key++;
      gen->drunkWalk += gen->hitRate;
    }
    gen->drunkWalk -= 1.0;
    return gen->key;
}
