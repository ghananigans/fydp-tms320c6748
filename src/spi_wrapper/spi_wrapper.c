/*
 * spi_wrapper.c
 *
 *  Created on: Jan 22, 2017
 *      Author: VirtWin
 */

#include "../config.h"
#include "../util.h"
#include "soc_C6748.h"
#include "hw_psc_C6748.h"
#include "lcdkC6748.h"
#include "spi.h"
#include "psc.h"
#include "interrupt.h"
#include <stdbool.h>
#include "hw_types.h"
#include <string.h>
#include "spi_wrapper.h"

#ifdef SPI_EDMA
#include "dspcache.h"
#include "../edma3_wrapper/edma3_wrapper.h"
#include "edma.h"
#include "edma_event.h"
#endif //#ifdef SPI_EDMA

/******************************************************************************/
/*                      INTERNAL MACRO DEFINITIONS                            */
/******************************************************************************/
/* value to configure SMIO,SOMI,CLK and CS pin as functional pin */
#define SIMO_SOMI_CLK_CS        (0x00000E30)
#define CHAR_LENGTH             (0x8)
#define DATA_FORMAT             SPI_DATA_FORMAT0
#define SPI_REG                 SOC_SPI_0_REGS
#define SPI_CS0                 (0x10)
#define SPI_CS1                 (0x20)
#define SPI_SCLK_HZ             (5000000)

/****************************************************************************/
/*                      INTERNAL GLOBAL VARIABLES                           */
/****************************************************************************/
static bool init_done = 0;

#ifdef SPI_EDMA
static volatile bool tx_flag = 0;
static volatile bool rx_flag = 0;
#else
static unsigned int volatile flag = 1;
static unsigned int tx_len = 0;
static unsigned int rx_len = 0;
static char volatile * p_tx = 0;
static char volatile * p_rx = 0;
static unsigned int latest_cs = 0;
#endif //#ifdef SPI_EDMA

/******************************************************************************/
/*                      INTERNAL FUNCTION DEFINITIONS                         */
/******************************************************************************/
#ifdef SPI_EDMA
/*
** This function is used as a callback from EDMA3 Completion Handler.
** The DMA Mode operation of SPI0 is disabled over here.
*/
static
void
callback (
    unsigned int tcc_num,
    unsigned int status
    )
{
    switch (tcc_num)
    {
        case EDMA3_CHA_SPI0_RX:
            rx_flag = 1;
            /* Disable SPI-EDMA Communication. */
            SPIIntDisable(SPI_REG, SPI_DMA_REQUEST_ENA_INT);
            break;

        case EDMA3_CHA_SPI0_TX:
            tx_flag = 1;
            /* Disable SPI-EDMA Communication. */
            SPIIntDisable(SPI_REG, SPI_DMA_REQUEST_ENA_INT);
            break;

        default:
            break;
    }
}
/*
** This function is used to set the PaRAM entries of EDMA3 for the Transmit
** Channel of SPI0. The corresponding EDMA3 channel is also enabled for
** transmission.
*/
static
int
edma3_param_set_spi_tx (
    unsigned int tcc_num,
    unsigned int channel_num,
    char volatile * buffer,
    unsigned short length
    )
{
    int ret_val;

    ret_val = edma3_param_set(tcc_num, channel_num, buffer, (char volatile *)(SPI_REG + SPI_SPIDAT1), 1, length, 1, 1, 0);

    if (ret_val == EDMA3_OK)
    {
        return SPI_OK;
    }
    else
    {
        ERROR_PRINT("edma3_param_set returned as not OK with error code: %d", ret_val);
        return SPI_INTERNAL_FAILURE_EDMA3_PARAM_SET_FAILED;
    }
}

/*
** This function is used to set the PaRAM entries of EDMA3 for the Receive
** Channel of SPI0. The corresponding EDMA3 channel is also enabled for
** reception.
*/

static
int
edma3_param_set_spi_rx (
    unsigned int tcc_num,
    unsigned int channel_num,
    char volatile * buffer,
    unsigned short length,
    unsigned int dest_bidx_flag
    )
{
    int ret_val;

    ret_val = edma3_param_set(tcc_num, channel_num, (char volatile *) (SPI_REG + SPI_SPIBUF), buffer, 1, length, 1, 0, 1);

    if (ret_val == EDMA3_OK)
    {
        return SPI_OK;
    }
    else
    {
        ERROR_PRINT("edma3_param_set returned as not OK with error code: %d", ret_val);
        return SPI_INTERNAL_FAILURE_EDMA3_PARAM_SET_FAILED;
    }
}
#else // #ifdef SPI_EDMA
/*
** Data transmission and receiption SPIIsr
**
*/
static
void
spi_isr (
    void
    )
{
    unsigned int intCode = 0;

    IntEventClear(SYS_INT_SPI0_INT);

    intCode = SPIInterruptVectorGet(SPI_REG);

    while (intCode)
    {
        if(intCode == SPI_TX_BUF_EMPTY)
        {
            tx_len--;

            SPITransmitData1(SPI_REG, *p_tx);

            p_tx++;
            if (!tx_len)
            {
                SPIIntDisable(SPI_REG, SPI_TRANSMIT_INT);
            }
        }

        if(intCode == SPI_RECV_FULL)
        {
            rx_len--;

            *p_rx = (char)SPIDataReceive(SPI_REG);

            p_rx++;
            if (!rx_len)
            {
                flag = 1;
                SPIIntDisable(SPI_REG, SPI_RECV_INT);

                /*
                 * Deasserts the CS pin. (Notice no CSHOLD flag)
                 */
                SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL
                            | DATA_FORMAT), latest_cs);
            }
        }

        intCode = SPIInterruptVectorGet(SPI_REG);
    }
}
#endif // #ifdef SPI_EDMA

/*
 * Configures SPI Controller.
 */
static
void
spi_setup (
    void
    )
{
    unsigned int val = SIMO_SOMI_CLK_CS;

    /*
     * Waking up the SPI0 instance.
     */
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_SPI0, PSC_POWERDOMAIN_ALWAYS_ON,
                     PSC_MDCTL_NEXT_ENABLE);

    /*
     * Performing the Pin Multiplexing for SPI0.
     */
    SPIPinMuxSetup(0);

    /*
     * Using the Chip Select(CS) 5 and 4 pin of SPI0.
     */
    SPI0CSPinMuxSetup(5);
    SPI0CSPinMuxSetup(4);

    SPIReset(SPI_REG);

    SPIOutOfReset(SPI_REG);

    SPIModeConfigure(SPI_REG, SPI_MASTER_MODE);

    SPIPinControl(SPI_REG, 0, 0, &val);

    SPIClkConfigure(SPI_REG, 150000000, SPI_SCLK_HZ, DATA_FORMAT);

    /*
     * Configures SPI Data Format Register
     */

    /*
     * Configures the polarity and phase of SPI clock
     */
    SPIConfigClkFormat(SPI_REG,
                        (SPI_CLK_POL_HIGH | SPI_CLK_OUTOFPHASE),
                        DATA_FORMAT);

    /*
     * Configures SPI to transmit MSB bit First during data transfer
     */
    SPIShiftMsbFirst(SPI_REG, DATA_FORMAT);

    /*
     * Sets the Charcter length
     */
    SPICharLengthSet(SPI_REG, CHAR_LENGTH, DATA_FORMAT);

    //SPIDefaultCSSet(SPI_REG, dcs);

     /*
      * Selects the SPI Data format register to use and add max delay
      * between transactions.
      */
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL | DATA_FORMAT), 0);

    /*
     * Maximum delay from CS edges to start and end of transmission
     * of data.
     */
    SPIDelayConfigure(SPI_REG, 0, 0, 0x0, 0x0);

    /*
     * Delay between transactions (max ammount of delay).
     */
    //SPIWdelaySet(SPI_REG, 0x3F, DATA_FORMAT);

#ifndef SPI_EDMA
    /*
    * map interrupts to interrupt line INT1
    */
    SPIIntLevelSet(SPI_REG, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);
#endif // #ifndef SPI_EDMA
    /*
     * Enable SPI communication
     */
    SPIEnable(SPI_REG);
}

/******************************************************************************/
/*                          FUNCTION DEFINITIONS                              */
/******************************************************************************/
int
spi_init (
    void
    )
{
    if (init_done)
    {
        DEBUG_PRINT("SPI is already initialized!\n");
        return SPI_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing spi...\n");

#ifdef SPI_EDMA
    /*
     * Request EDMA3CC for Tx an Rx channels for SPI0.
     */
    edma3_request_channel(EDMA3_CHA_SPI0_TX, EDMA3_CHA_SPI0_TX);
    edma3_request_channel(EDMA3_CHA_SPI0_RX, EDMA3_CHA_SPI0_RX);

    /* Registering Callback Function for Tx and Rx. */
    edma3_set_callback(EDMA3_CHA_SPI0_TX, &callback);
    edma3_set_callback(EDMA3_CHA_SPI0_RX, &callback);
#else // #ifdef SPI_EDMA
    /*
     * Register the ISR in the vector table
     */
    IntRegister(C674X_MASK_INT6, spi_isr);

    /*
     * Map system interrupt to the DSP maskable interrupt
     */
    IntEventMap(C674X_MASK_INT6, SYS_INT_SPI0_INT);

    // Enable the DSP maskable interrupt
    IntEnable(C674X_MASK_INT6);
#endif // #ifdef SPI_EDMA

    /*
     * Initialize SPI w/ Interrupts.
     */
    spi_setup();

    init_done = 1;

    DEBUG_PRINT("Done initializing spi!\n");

    return SPI_OK;
}

void
spi_wait_until_not_busy (
    void
    )
{
    DEBUG_PRINT("Waiting for spi to not be busy (finish transaction if it is doing one)\n");

#ifdef SPI_EDMA
#else  // #ifdef SPI_EDMA
    /*
     * Wait for flag to change (transaction to finish).
     */
    while (flag == 0);
#endif // #ifdef SPI_EDMA
}

/*
 * CS should be 0 or 1 only.
 */
int
spi_send_and_receive_non_blocking (
    char volatile * data,
    unsigned int len,
    unsigned int cs
    )
{
#ifdef SPI_EDMA
    int ret_val;
#endif // #ifdef SPI_EDMA

    if (init_done == 0)
    {
        ERROR_PRINT("SPI is not initialized!\n");
        return SPI_NOT_INTIAILZED;
    }

    if (cs > 1)
    {
        ERROR_PRINT("CS is not 0 or 1; Only supporting 2 CS (Got: %d)", cs);
        return SPI_INVALID_CS;
    }

    //
    // Make sure any task currently running is completed first.
    //
    spi_wait_until_not_busy();

    /*
     * Get actual cs pin mask.
     */
    switch (cs)
    {
        case 0:
            cs = SPI_CS0;
            break;
        case 1:
            cs = SPI_CS1;
            break;
        default:
            // Wont ever get here
            ERROR_PRINT("SHOULD NEVER BE HERE!?!?!?!?!?!?!??! cs = %d\n", cs);
            break;
    }

    latest_cs = cs;

    DEBUG_PRINT("Sending and receiving spi data with cs: %d\n", cs);
    DEBUG_PRINT("Transaction starting...\n");

#ifdef SPI_EDMA
    /*
     * Writeback whatever is in cache to main memory.
     */
    CacheWB((unsigned int) data, len);

    /* Configure the PaRAM registers in EDMA for Transmission.*/
    ret_val = edma3_param_set_spi_tx(EDMA3_CHA_SPI0_TX, EDMA3_CHA_SPI0_TX, data, len);
    if (ret_val != SPI_OK)
    {
        return ret_val;
    }

    /* Configure the PaRAM registers in EDMA for Reception.*/
    ret_val = edma3_param_set_spi_rx(EDMA3_CHA_SPI0_RX, EDMA3_CHA_SPI0_RX, data, len, 1);
    if (ret_val != SPI_OK)
    {
        return ret_val;
    }
#else // #ifdef SPI_EDMA
    p_tx = data;
    p_rx = data;

    tx_len = len;
    rx_len = len;
#endif // #ifdef SPI_EDMA

    /*
     * Assert the CS pin.
     */
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL
                | SPI_SPIDAT1_CSHOLD | DATA_FORMAT), cs);

#ifdef SPI_EDMA
    /*
     * Enable the interrupts.
     */
    SPIIntEnable(SPI_REG, SPI_DMA_REQUEST_ENA_INT);

    /* Wait until both the flags are set to 1 in the callback function. */
    while ((tx_flag == 0) || (rx_flag == 0));
    tx_flag = 0;
    rx_flag = 0;
#else // #ifdef SPI_EDMA
    flag = 0;

    /*
     * Enable the interrupts.
     */
    SPIIntEnable(SPI_REG, (SPI_RECV_INT | SPI_TRANSMIT_INT));
#endif // #ifdef SPI_EDMA

    return SPI_OK;
}

int
spi_send_and_receive_blocking (
    char volatile * data,
    unsigned int len,
    unsigned int cs
    )
{
    int ret_val;

    ret_val = spi_send_and_receive_non_blocking(data, len, cs);

    if (ret_val != SPI_OK)
    {
        return ret_val;
    }

    //
    // Wait for SPI to finish the transaction
    //
    spi_wait_until_not_busy();

    DEBUG_PRINT("Transaction finished!\n");

    return SPI_OK;
}
