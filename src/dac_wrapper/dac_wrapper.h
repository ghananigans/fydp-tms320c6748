/*
 * dac_wrapper.h
 *
 *  Created on: Jan 25, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef DAC_WRAPPER_H_
#define DAC_WRAPPER_H_

#include <stdint.h>

#define DAC_CHANNELS           (8)

#define DAC_POWER_ON_CHANNEL_0 (0x01)
#define DAC_POWER_ON_CHANNEL_1 (0x02)
#define DAC_POWER_ON_CHANNEL_2 (0x04)
#define DAC_POWER_ON_CHANNEL_3 (0x08)
#define DAC_POWER_ON_CHANNEL_4 (0x10)
#define DAC_POWER_ON_CHANNEL_5 (0x20)
#define DAC_POWER_ON_CHANNEL_6 (0x40)
#define DAC_POWER_ON_CHANNEL_7 (0x80)

#define DAC_ADDRESS_CHANNEL_0  (0x0)
#define DAC_ADDRESS_CHANNEL_1  (0x1)
#define DAC_ADDRESS_CHANNEL_2  (0x2)
#define DAC_ADDRESS_CHANNEL_3  (0x3)
#define DAC_ADDRESS_CHANNEL_4  (0x4)
#define DAC_ADDRESS_CHANNEL_5  (0x5)
#define DAC_ADDRESS_CHANNEL_6  (0x6)
#define DAC_ADDRESS_CHANNEL_7  (0x7)

enum DAC_ERROR_CODES
{
    DAC_OK,
    DAC_NOT_INITIALIZED,
    DAC_ALREADY_INITIALIZED,
    DAC_COMMAND_SEND_FAILED,
    DAC_INVALID_DAC_CHANNEL_ADDRESS,
    DAC_INTERNAL_FAILURE_COMMAND_SIZE,
    DAC_INTERNAL_FAILURE_ZERO_BIT_NOT_ZERO
};

int
dac_init (
    unsigned int cs
    );

int
dac_reset (
    void
    );

int
dac_power_up (
    uint8_t dac_flags
    );

int
dac_power_down (
    uint8_t dac_flags
    );

int
dac_update (
    uint8_t address,
    uint16_t data
    );

#endif /* DAC_WRAPPER_H_ */
