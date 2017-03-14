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


/******************************************************************************/
/*                      INTERNAL MACRO DEFINITIONS                            */
/******************************************************************************/
/*
 * Dac Command Control (DCC) flags.
 *
 * DCC_WRITE_TO_INPUT_REGISTER = 0x0
 *   - Write to the input register of the dac specified in address
 *     field but do not update the dac register itself. (input register
 *     needs to be moved to the dac register).
 *
 * DCC_UPDATE_DAC_REGISTER = 0x1
 *   - Move contents of input register into the register of the
 *     channel specified in the address field.
 *
 * DCC_WRITE_TO_INPUT_REGISTER_AND_UPDATE_ALL_DAC_REGISTERS = 0x2
 *   - Write to the input register of the channel specified in the
 *     address field and update the registers of all channels on the
 *     DAC.
 *
 * DCC_WRITE_TO_INPUT_REGISTER_AND_UPDATE_DAC_REGISTER = 0x3
 *   - Write to the input register AND update the register of the
 *     channel specified in the address field.
 *
 * DCC_POWER = 0x4
 *   - Update power settings of the DAC.
 *
 * DCC_SOFTWARE_RESET = 0x7
 *   - Reset the DAC.
 */
#define DCC_WRITE_TO_INPUT_REGISTER  (0x0)
#define DCC_UPDATE_DAC_REGISTER      (0x1)
#define DCC_WRITE_TO_INPUT_REGISTER_AND_UPDATE_ALL_DAC_REGISTERS (0x02)
#define DCC_WRITE_TO_INPUT_REGISTER_AND_UPDATE_DAC_REGISTER (0x3);
#define DCC_POWER                    (0x4)
#define DCC_SOFTWARE_RESET           (0x7)
#define DCC_INTERNAL_REFERENCE_POWER (0x8)
/******************************************************************************/
/*                      INTERNAL DATA STRUCTURE DEFINITIONS                   */
/******************************************************************************/
#pragma pack(push, 1)
typedef union _dac_command_t
{
    struct
    {
        uint8_t feature      : 4; // dont cares
        uint16_t data        : 16;
        uint8_t address      : 4;
        uint8_t control      : 4;
        uint8_t dont_care    : 3;
        uint8_t zero         : 1; // SET TO 0
    } update;

    struct
    {
        uint32_t dont_care_l : 24;
        uint8_t control      : 4; // set this to 0x7
        uint8_t dont_care_m  : 3;
        uint8_t zero         : 1; // SET TO 0
    } reset;

    struct
    {
        uint8_t dacs         : 8; // LSB = DACA; MSB = DAC8
        uint8_t config       : 2; // 0x0: power up; 0x1: power down (1kOhm resistor to GND); 0x2: power down (100kOHM resistor to GND); 0x3: power down (HIGH-Z to GND)
        uint16_t dont_care_m : 14;
        uint8_t control      : 4; // SET TO 0x4
        uint8_t dont_care_l  : 3;
        uint8_t zero         : 1; // SET TO 0
    } power;

    struct {
        uint8_t flag         : 1; // SET TO 1 for POWER ON AND 0 for POWER OFF
        uint32_t dont_care_m : 23;
        uint8_t control      : 4; // SET TO 0x8
        uint8_t dont_care_l  : 3;
        uint8_t zero         : 1; // SET TO 0
    } internal_reference_power;

    uint32_t as_uint32t;
} dac_command_t;
#pragma pack(pop)

/******************************************************************************/
/*                      INTERNAL GLOBAL VARIABLES                             */
/******************************************************************************/
static bool init_done = 0;
static unsigned int spi_cs;


/******************************************************************************/
/*                      INTERNAL FUNCTION DEFINITIONS                         */
/******************************************************************************/
static
int
dac_command_send (
    dac_command_t * payload,
    bool blocking
    )
{
    int ret_val;

    if (sizeof(dac_command_t) != 4)
    {
        ERROR_PRINT("Unrecognized command struct! Was something updated?\n");
        return DAC_INTERNAL_FAILURE_COMMAND_SIZE;
    }

    if (payload->update.zero)
    {
        ERROR_PRINT("Command format specifies that the MSB bit must be 0!");
        return DAC_INTERNAL_FAILURE_ZERO_BIT_NOT_ZERO;
    }

    DEBUG_PRINT("Payload AS uint32_t: 0x%x\n", payload->as_uint32t);
    payload->as_uint32t = ENDIAN_SWAP_32(payload->as_uint32t);
    DEBUG_PRINT("Payload AS uint32_t after endian swap: 0x%x\n", payload->as_uint32t);

    if (blocking == 0)
    {
        ret_val = spi_send_and_receive_non_blocking((char volatile *) payload, sizeof(dac_command_t), spi_cs);
    }
    else
    {
        ret_val = spi_send_and_receive_blocking((char volatile *) payload, sizeof(dac_command_t), spi_cs);
    }


    if (ret_val != SPI_OK)
    {
        ERROR_PRINT("spi send data failed with error: %d", ret_val);
        ret_val = DAC_COMMAND_SEND_FAILED;
    }
    else
    {
        ret_val = DAC_OK;
    }

    DEBUG_PRINT("Response AS uint32_t: 0x%x\n", payload->as_uint32t);

    return ret_val;
}

static
int
dac_power (
    uint8_t dac_flags,
    bool power_up,
    bool blocking
    )
{
    dac_command_t command;

    if (dac_flags == 0)
    {
        DEBUG_PRINT("No dacs were specified so simply return and do nothing... This is not a failure, but why do you do this?\n");
        return DAC_OK;
    }

    command.power.dacs = dac_flags;
    command.power.config = (power_up ? 0x0 : 0x2);
    command.power.control = DCC_POWER;
    command.power.zero = 0;

    DEBUG_PRINT("Sending power related command to dac with dac flags [0x%02x] and config [0x%x]\n", dac_flags, command.power.config);

    return dac_command_send(&command, blocking);
}

static
int
dac_internal_reference_power (
    bool power_up,
    bool blocking
    )
{
    dac_command_t command;

    command.internal_reference_power.flag = !!power_up;
    command.internal_reference_power.control = DCC_INTERNAL_REFERENCE_POWER;
    command.internal_reference_power.zero = 0;

    DEBUG_PRINT("Sending internal power related command to dac with flag [%d]\n", command.internal_reference_power.flag);

    return dac_command_send(&command, blocking);
}


/******************************************************************************/
/*                          FUNCTION DEFINITIONS                              */
/******************************************************************************/
int
dac_init (
    unsigned int cs
    )
{
    int ret_val;

    if (init_done)
    {
        DEBUG_PRINT("DAC already initialized!\n");
        return DAC_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing dac...\n");

    spi_cs = cs;

    init_done = 1;

    ret_val = dac_reset(1);

    if (ret_val != DAC_OK)
    {
        ERROR_PRINT("Post Init reset failed with error: %d!\n", ret_val);
        init_done = 0;
    }
    else
    {
       DEBUG_PRINT("Done initializing the dac!\n");
    }

    return ret_val;
}

int
dac_reset (
    bool blocking
    )
{
    dac_command_t command;

    if (init_done == 0)
    {
        ERROR_PRINT("DAC not intialized!\n");
        return DAC_NOT_INITIALIZED;
    }

    DEBUG_PRINT("Resetting DAC...\n");

    command.reset.control = DCC_SOFTWARE_RESET;
    command.reset.zero = 0;

    return dac_command_send(&command, blocking);
}

int
dac_power_up (
    uint8_t dac_flags,
    bool blocking
    )
{
    if (init_done == 0)
    {
        ERROR_PRINT("DAC not intialized!\n");
        return DAC_NOT_INITIALIZED;
    }

    if (dac_flags == 0)
    {
        DEBUG_PRINT("No dacs were specified so simply return and do nothing... This is not a failure, but why do you do this?\n");
        return DAC_OK;
    }

    return dac_power(dac_flags, 1, blocking);
}

int
dac_power_down (
    uint8_t dac_flags,
    bool blocking
    )
{
    if (init_done == 0)
    {
        ERROR_PRINT("DAC not intialized!\n");
        return DAC_NOT_INITIALIZED;
    }

    if (dac_flags == 0)
    {
        DEBUG_PRINT("No dacs were specified so simply return and do nothing... This is not a failure, but why do you do this?\n");
        return DAC_OK;
    }

    return dac_power(dac_flags, 0, blocking);
}

int
dac_update (
    uint8_t address,
    uint16_t data,
    bool blocking
    )
{
    dac_command_t command;

    if (init_done == 0)
    {
        ERROR_PRINT("DAC not intialized!\n");
        return DAC_NOT_INITIALIZED;
    }

    if (address >= DAC_CHANNELS)
    {
        ERROR_PRINT("Invalid address specified: %d. Dac only has 8 channels (0 indexed).\n", address);
        return DAC_INVALID_DAC_CHANNEL_ADDRESS;
    }

    DEBUG_PRINT("Sending update command to DAC channel [%d] with data [0x%05x]\n", address, data);

    command.update.address = address;
    command.update.data = data;
    command.update.control = DCC_WRITE_TO_INPUT_REGISTER_AND_UPDATE_DAC_REGISTER;
    command.update.zero = 0;

    return dac_command_send(&command, blocking);
}

int
dac_internal_reference_power_down (
    bool blocking
    )
{
    return dac_internal_reference_power(0, blocking);
}

int
dac_internal_reference_power_up (
    bool blocking
    )
{
    return dac_internal_reference_power(1, blocking);
}
