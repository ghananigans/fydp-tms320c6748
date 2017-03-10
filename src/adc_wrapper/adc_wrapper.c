/*
 * adc_wrapper.c
 *
 *  Created on: Mar 10, 2017
 *      Author: Ghanan Gowripalan
 */

#include "../config.h"
#include "../util.h"
#include "adc_wrapper.h"
#include "../spi_wrapper/spi_wrapper.h"
#include <stdbool.h>

static bool init_done = 0;
static unsigned int spi_cs = 0;

#pragma pack(push, 1)
typedef union _adc_command_t
{
    struct {
        uint16_t dont_care  : 13;
        uint8_t msb_first  : 1; // if 1, msb first; else lsb first
        uint8_t odd_signn  : 1; // channel select if single eded, or polarity for differntial mode
        uint8_t sgl_diffn  : 1; // sigle ended or differentla mode (SIF/!DIFF; 1 for single; 0 for diff)
        uint8_t start      : 1;
        uint16_t zeroes     : 15;
    } request;

	struct {
        uint16_t data        : 12;
        uint8_t null_bit     : 1;
        uint32_t dont_care   : 19;
    } response;

    uint32_t as_uint32t;
} adc_command_t;
#pragma pack(pop)

static
int
adc_command_send (
    adc_command_t * payload
    )
{
    int ret_val;

    if (sizeof(adc_command_t) != 4)
    {
        ERROR_PRINT("Unrecognized command struct! Was something updated?\n");
        return ADC_INTERNAL_FAILURE_COMMAND_SIZE;
    }

    DEBUG_PRINT("Payload AS uint32_t: 0x%x\n", payload->as_uint32t);
    payload->as_uint32t = ENDIAN_SWAP_32(payload->as_uint32t);
    DEBUG_PRINT("Payload AS uint32_t after endian swap: 0x%x\n", payload->as_uint32t);

    ret_val = spi_send_and_receive((char volatile *) payload, sizeof(adc_command_t), spi_cs);

    if (ret_val != SPI_OK)
    {
        ERROR_PRINT("spi send data failed with error: %d", ret_val);
        ret_val = ADC_COMMAND_SEND_FAILED;
    }
    else
    {
        ret_val = ADC_OK;
    }

    DEBUG_PRINT("Response AS uint32_t: 0x%x\n", payload->as_uint32t);

    return ret_val;
}

int
adc_init (
    unsigned int cs
    )
{
     if (init_done)
    {
        DEBUG_PRINT("ADC already initialized!\n");
        return ADC_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing adc...\n");

    spi_cs = cs;

    init_done = 1;

    return ADC_OK;
}

int
adc_read (
    uint16_t * data
    )
{
    adc_command_t command;
    int ret_val;

    command.request.zeroes = 0;
    command.request.start = 1;
    command.request.sgl_diffn = 1;
    command.request.odd_signn = 0;
    command.request.msb_first = 1;

    DEBUG_PRINT("Sending read command to adc\n");

    ret_val = adc_command_send(&command);

    *data = command.response.data;
    return ret_val;
}
