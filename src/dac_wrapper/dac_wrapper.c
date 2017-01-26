/*
 * dac_wrapper.c
 *
 *  Created on: Jan 25, 2017
 *      Author: Ghanan Gowripalan
 */

#include "dac_wrapper.h"
#include <stdbool.h>
#include "../util.h"
#include "../spi_wrapper/spi_wrapper.h"

#pragma pack(push, 1)
typedef union _dac_input_shift_register_union
{
    struct {
        uint8_t feature : 4;
        uint16_t data   : 16;
        uint8_t address : 4;
        uint8_t control : 4;
        uint8_t prefix  : 4; // SET TO 0
    } s;
    uint32_t as_uint32t;
} dac_input_shift_register_t;
#pragma pack(pop)

static bool init_done = 0;
static unsigned int spi_cs;

int
dac_init (
    unsigned int cs
    )
{
    if (init_done)
    {
        DEBUG_PRINT("DAC already initialized!\n");
        return DAC_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing dac...\n");

    spi_cs = cs;

    init_done = 1;

    return DAC_OK;
}

int
dac_update (
    uint8_t address,
    uint16_t data
    )
{
    unsigned char payload[4];
    int ret_val;

    dac_input_shift_register_t *input_shr;
    input_shr = (dac_input_shift_register_t *) &payload;

    input_shr->s.feature = 1;
    input_shr->s.address = address;
    input_shr->s.data = NIBBLE_SWAP_16(data);
    input_shr->s.control = 7;
    input_shr->s.prefix = 8;

    DEBUG_PRINT("Payload AS uint32_t: 0x%x\n", input_shr->as_uint32t);
    input_shr->as_uint32t = ENDIAN_SWAP_32(input_shr->as_uint32t);
    DEBUG_PRINT("Payload AS uint32_t after endian swap: 0x%x\n", input_shr->as_uint32t);

    ret_val = spi_send_and_receive(&payload, 4, spi_cs);

    if (ret_val != SPI_OK)
    {
        ERROR_PRINT("spi send data failed with error: %d", ret_val);
        ret_val = DAC_UPDATE_FAILED;
    }
    else
    {
    	ret_val = DAC_OK;
    }

    DEBUG_PRINT("Response AS uint32_t: 0x%x\n", input_shr->as_uint32t);

    return ret_val;
}
