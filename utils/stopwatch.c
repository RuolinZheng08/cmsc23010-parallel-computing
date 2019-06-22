#include "stopwatch.h"

void startTimer(StopWatch_t * watch){
	gettimeofday(&watch->startTime, NULL);
}

void stopTimer(StopWatch_t * watch) {
	gettimeofday(&watch->stopTime, NULL);
}

double getElapsedTime(StopWatch_t * watch) {
	return ((watch->stopTime.tv_sec-watch->startTime.tv_sec)*1000000LL + watch->stopTime.tv_usec-watch->startTime.tv_usec)/1000.0f;
}

