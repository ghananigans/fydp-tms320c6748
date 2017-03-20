/*
 * calibration.c
 *
 *  Created on: Mar 14, 2017
 *      Author: vinaypadma
 */
#include "calibration.h"
#include "../util.h"

#ifndef PI
#define PI (3.14159265358979323846)
#endif

float phase_array[(MAX_FREQ - MIN_FREQ)/INTERVAL_SIZE_HZ + 1];

//set all initial values to -1f
void initPhaseArray(void) {
	unsigned int phaseArraySize = (MAX_FREQ - MIN_FREQ)/INTERVAL_SIZE_HZ + 1;
	int i;
	for (i = 0; i < phaseArraySize; ++i) {
		phase_array[i] = -1.0;
	}
	return;
}

int roundToClosestInterval(unsigned int freqHz) {
	if (freqHz > MAX_FREQ + INTERVAL_SIZE_HZ/2)
		return -1;
	if (freqHz < MIN_FREQ - INTERVAL_SIZE_HZ/2)
		return -1;

	unsigned int retFreq = freqHz - MIN_FREQ;
	unsigned int remainder = retFreq % INTERVAL_SIZE_HZ;
	if (remainder > INTERVAL_SIZE_HZ/2){
		return retFreq + MIN_FREQ + (INTERVAL_SIZE_HZ - remainder);
	} else {
		return retFreq + MIN_FREQ - remainder;
	}
}

int getIndexFromFrequency(unsigned int freqHz) {
	int roundedFreq = roundToClosestInterval(freqHz);
	if (roundedFreq < 0){
		NORMAL_PRINT("INVALID FREQUENCY RANGE\n");
		return -1;
	}
	return (roundedFreq - MIN_FREQ)/INTERVAL_SIZE_HZ;
}

int getFrequencyFromIndex(unsigned int index) {
	unsigned int phaseArraySize = (MAX_FREQ - MIN_FREQ)/INTERVAL_SIZE_HZ + 1;
	if (index >= phaseArraySize){
		NORMAL_PRINT("INVALID INDEX REQUESTED");
		return -1;
	}
	return index * INTERVAL_SIZE_HZ + MIN_FREQ;
}

float getCalibrationForFreq(unsigned int freqHz) {
	int index = roundToClosestInterval(freqHz);
	if (index < 0){
		NORMAL_PRINT("INVALID FREQUENCY RANGE\n");
		return -1.0;
	}
	return phase_array[index];
}

//theta is multiples of PI, i.e. newTheta = 90deg, we represent as newTheta = 0.5
float getEquivalentPhase(float theta) {
	while (theta - 2 > 0){
		theta -= 2;
	}
	return theta;
}

void updateCalibrationForFreq(unsigned int freqHz, float newTheta) {
	int index = roundToClosestInterval(freqHz);
	if (index < 0){
		NORMAL_PRINT("INVALID FREQUENCY RANGE\n");
		return;
	}

	phase_array[index] = getEquivalentPhase(newTheta);
	return;
}
