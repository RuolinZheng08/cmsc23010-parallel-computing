#include "packetsource.h"

PacketSource_t * createPacketSource(long mean, int numSources, short seed) {

	PacketSource_t * packetSource = (PacketSource_t *)malloc(sizeof(PacketSource_t));

    // record the mean
    packetSource->constantGen = mean;

	packetSource->uniformGen = (UniformGenerator_t *)malloc(sizeof(UniformGenerator_t) * numSources);
	packetSource->uniformSeed = (UniformGenerator_t *)malloc(sizeof(UniformGenerator_t) * numSources);
	packetSource->uniformCounts = (long *)malloc(sizeof(long) * numSources);

	packetSource->exponentialGen = (ExponentialGenerator_t *)malloc(sizeof(ExponentialGenerator_t) * numSources);
	packetSource->exponentialSeed = (UniformGenerator_t *)malloc(sizeof(UniformGenerator_t) * numSources);
	packetSource->exponentialCounts = (long *)malloc(sizeof(long) * numSources);

	long exponentialMeans[numSources];

	ExponentialGenerator_t meanGen;
	meanGen.mean = mean;
	initGenerator(&(meanGen.randGen));

	for(short i = 0; i < seed; i++){
		genExponentialRand(&meanGen);
	}
	long tmpMean = 0;

	for(int i = 0; i < numSources; i++){
	 	exponentialMeans[i] = genExponentialRand(&meanGen);
	 	tmpMean += exponentialMeans[i];
	}
	tmpMean -= (mean*numSources);
    tmpMean /= numSources;
    const int BIG_NUM = 100000000;
    for( int i = 0; i < numSources; i++) {
      initGenerator(&(packetSource->uniformGen[i].randGen));
      packetSource->uniformGen[i].maxValue=  2*mean + 1;
      packetSource->uniformGen[i].randGen.seed = ((1+i) & 0xFF);

      initGenerator(&(packetSource->uniformSeed[i].randGen));
      packetSource->uniformSeed[i].maxValue= (BIG_NUM + 1);
      packetSource->uniformSeed[i].randGen.seed = ((1+i) & 0xFF);

      initGenerator(&(packetSource->exponentialGen[i].randGen));
      packetSource->exponentialGen[i].mean = exponentialMeans[i]-tmpMean;
      packetSource->exponentialGen[i].randGen.seed = ((1+i) & 0xFF);

      initGenerator(&(packetSource->exponentialSeed[i].randGen));
      packetSource->exponentialSeed[i].maxValue= BIG_NUM + 1;
      packetSource->exponentialSeed[i].randGen.seed = ((1+i) & 0xFF);

      packetSource->uniformCounts[i] = 0;
      packetSource->exponentialCounts[i] = 0;

      for( short j = 0; j < seed; j++ ) {
    	getUniformRand(&(packetSource->uniformSeed[i]));
    	getUniformRand(&(packetSource->exponentialSeed[i]));
      }
    }
    return packetSource;
}
void deletePacketSource(PacketSource_t * packetSource)
{
	free(packetSource->uniformGen);
	free(packetSource->uniformSeed);
	free(packetSource->uniformCounts);

	free(packetSource->exponentialGen);
	free(packetSource->exponentialCounts);

    free(packetSource);
}
volatile Packet_t * getUniformPacket(PacketSource_t * packetSource, int sourceNum){
	volatile Packet_t * tmp = (volatile Packet_t *)malloc(sizeof(volatile Packet_t));
	tmp->iterations =  getUniformRand(&(packetSource->uniformGen[sourceNum]));
	tmp->seed =  getUniformRand(&(packetSource->uniformSeed[sourceNum]));
	packetSource->uniformCounts[sourceNum] += tmp->iterations;
	return tmp;
}
volatile Packet_t * getExponentialPacket(PacketSource_t * packetSource, int sourceNum)
{
	volatile Packet_t * tmp = (volatile Packet_t *)malloc(sizeof(volatile Packet_t));
	tmp->iterations = genExponentialRand(&(packetSource->exponentialGen[sourceNum]));
	tmp->seed = getUniformRand(&(packetSource->exponentialSeed[sourceNum]));
	packetSource->exponentialCounts[sourceNum] += tmp->iterations;
	return tmp;
}
volatile Packet_t * getConstantPacket(PacketSource_t * packetSource, int sourceNum) {
    volatile Packet_t * tmp = (volatile Packet_t *)malloc(sizeof(volatile Packet_t));
    tmp->iterations = packetSource->constantGen;
    tmp->seed = getUniformRand(&(packetSource->uniformSeed[sourceNum]));
    return tmp;
}
long getUniformCount(PacketSource_t * packetSource,int sourceNum) {
	return packetSource->uniformCounts[sourceNum];
}
long  getExponentialCount(PacketSource_t * packetSource,int sourceNum) {
	return packetSource->exponentialCounts[sourceNum];
}
