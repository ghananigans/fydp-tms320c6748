/*
 * calibration.h
 *
 *  Created on: Mar 14, 2017
 *      Author: vinaypadma
 */

#ifndef SRC_CALIBRATION_CALIBRATION_H_
#define SRC_CALIBRATION_CALIBRATION_H_

#define MIN_FREQ 100
#define MAX_FREQ 1000
#define INTERVAL_SIZE_HZ 5

//array representing the phase offsets required at each frequency in the range
extern float phase_array[(MAX_FREQ - MIN_FREQ)/INTERVAL_SIZE_HZ + 1];

void initPhaseArray(void);
int roundToClosestInterval(unsigned int freqHz);
int getIndexFromFrequency(unsigned int freqHz);
int getFrequencyFromIndex(unsigned int index);
float getCalibrationForFreq(unsigned int freqHz);
void updateCalibrationForFreq(unsigned int freqHz, float newTheta);
float getEquivalentPhase(float theta);


#endif /* SRC_CALIBRATION_CALIBRATION_H_ */
