/*
 * adc_wrapper.h
 *
 *  Created on: Mar 10, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef SRC_ADC_WRAPPER_ADC_WRAPPER_H_
#define SRC_ADC_WRAPPER_ADC_WRAPPER_H_

#include <stdint.h>

enum ADC_ERROR_CODES
{
    ADC_OK,
    ADC_NOT_INITIALIZED,
    ADC_ALREADY_INITIALIZED,
    ADC_COMMAND_SEND_FAILED,
    ADC_INTERNAL_FAILURE_COMMAND_SIZE
};

int
adc_init (
    unsigned int cs
    );

int
adc_read (
    uint16_t * data
    );

#endif /* SRC_ADC_WRAPPER_ADC_WRAPPER_H_ */
