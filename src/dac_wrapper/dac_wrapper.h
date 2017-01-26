/*
 * dac_wrapper.h
 *
 *  Created on: Jan 25, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef DAC_WRAPPER_H_
#define DAC_WRAPPER_H_

#include <stdint.h>

enum DAC_ERROR_CODES
{
    DAC_OK,
    DAC_NOT_INITIALIZED,
    DAC_ALREADY_INITIALIZED,
    DAC_UPDATE_FAILED
};

int
dac_init (
    unsigned int cs
    );

int
dac_update (
    uint8_t address,
    uint16_t data
    );

#endif /* DAC_WRAPPER_H_ */
