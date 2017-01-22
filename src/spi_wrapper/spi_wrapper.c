/*
 * spi_wrapper.c
 *
 *  Created on: Jan 22, 2017
 *      Author: VirtWin
 */

#include "spi_wrapper.h"
#include "../util.h"
#include "soc_C6748.h"
#include "hw_psc_C6748.h"
#include "lcdkC6748.h"
#include "spi.h"
#include "psc.h"
#include "interrupt.h"
#include <stdbool.h>

/******************************************************************************
**                      INTERNAL MACRO DEFINITIONS
*******************************************************************************/
/* value to configure SMIO,SOMI,CLK and CS pin as functional pin */
#define SIMO_SOMI_CLK_CS        (0x00000E04)
#define CHAR_LENGTH             (0x8)
#define CS_VALUE                (0x20)

/****************************************************************************/
/*                      GLOBAL VARIABLES                                    */
/****************************************************************************/
static bool init_done = 0;
static volatile unsigned int flag = 1;
static unsigned int tx_len;
static unsigned int rx_len;
static unsigned char * p_tx;
static unsigned char volatile * p_rx;

/******************************************************************************
**                      INTERNAL FUNCTION DEFINITIONS
*******************************************************************************/

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

#ifdef _TMS320C6X
    IntEventClear(SYS_INT_SPI0_INT);
#else
    IntSystemStatusClear(56);
#endif

    intCode = SPIInterruptVectorGet(SOC_SPI_0_REGS);

    while (intCode)
    {
        if(intCode == SPI_TX_BUF_EMPTY)
        {
            tx_len--;

            SPITransmitData1(SOC_SPI_0_REGS, *p_tx);

            p_tx++;
            if (!tx_len)
            {
                SPIIntDisable(SOC_SPI_0_REGS, SPI_TRANSMIT_INT);
            }
        }

        if(intCode == SPI_RECV_FULL)
        {
            rx_len--;

            *p_rx = (char)SPIDataReceive(SOC_SPI_0_REGS);

            p_rx++;
            if (!rx_len)
            {
                flag = 0;
                SPIIntDisable(SOC_SPI_0_REGS, SPI_RECV_INT);
            }
        }

        intCode = SPIInterruptVectorGet(SOC_SPI_0_REGS);
    }
}

/*
** Configures Data Format register of SPI
**
*/
static
void
spi_config_data_format_reg (
    unsigned int dataFormat
    )
{
    /* Configures the polarity and phase of SPI clock */
    /*
	SPIConfigClkFormat(SOC_SPI_1_REGS,
                       (SPI_CLK_POL_HIGH | SPI_CLK_INPHASE),
                       dataFormat);
    */
    SPIConfigClkFormat(SOC_SPI_0_REGS,
                       (SPI_CLK_POL_LOW | SPI_CLK_INPHASE),
                       dataFormat);

    /* Configures SPI to transmit MSB bit First during data transfer */
    SPIShiftMsbFirst(SOC_SPI_0_REGS, dataFormat);

    /* Sets the Charcter length */
    SPICharLengthSet(SOC_SPI_0_REGS, CHAR_LENGTH, dataFormat);
}

/*
** Configures SPI Controller
**
*/
static
void
spi_setup (
    void
    )
{
    unsigned char dcs = 0x20;
    unsigned int  val = SIMO_SOMI_CLK_CS;

    // Register the ISR in the vector table
    IntRegister(C674X_MASK_INT4, spi_isr);

    // Map system interrupt to the DSP maskable interrupt
    IntEventMap(C674X_MASK_INT4, SYS_INT_SPI0_INT);

    // Enable the DSP maskable interrupt
    IntEnable(C674X_MASK_INT4);

    SPIReset(SOC_SPI_0_REGS);

    SPIOutOfReset(SOC_SPI_0_REGS);

    SPIModeConfigure(SOC_SPI_0_REGS, SPI_MASTER_MODE);

    SPIPinControl(SOC_SPI_0_REGS, 0, 0, &val);

    /* Configures SPI Data Format Register */
    spi_config_data_format_reg(SPI_DATA_FORMAT0);

    //SPIClkConfigure(SOC_SPI_1_REGS, 150000000, 20000000, SPI_DATA_FORMAT0);
    SPIClkConfigure(SOC_SPI_0_REGS, 150000000, 1000000, SPI_DATA_FORMAT0);

    SPIDefaultCSSet(SOC_SPI_0_REGS, dcs);

     /* Selects the SPI Data format register to used and Sets CSHOLD
      * to assert CS pin(line)
      */
    //SPIDat1Config(SOC_SPI_0_REGS, (SPI_CSHOLD | SPI_DATA_FORMAT0), 0);

     /* map interrupts to interrupt line INT1 */
    SPIIntLevelSet(SOC_SPI_0_REGS, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);

    /* Enable SPI communication */
    SPIEnable(SOC_SPI_0_REGS);
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

    /*
     * Waking up the SPI1 instance.
     */
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_SPI0, PSC_POWERDOMAIN_ALWAYS_ON,
                     PSC_MDCTL_NEXT_ENABLE);

    /*
     * Performing the Pin Multiplexing for SPI0.
     */
    SPIPinMuxSetup(0);

    /*
     * Using the Chip Select(CS) 5 pin of SPI0.
     */
    SPI1CSPinMuxSetup(5);

    /*
     * Initialize SPI w/ Interrupts.
     */

    spi_setup();

    init_done = 1;

    DEBUG_PRINT("Done initializing spi!\n");

    return SPI_OK;
}

/*
** Enables SPI Transmit and Receive interrupt.
** Deasserts Chip Select line.
*/
int
spi_send (
    unsigned char * data,
    unsigned int len
    )
{
    p_tx = data;
    p_rx = data;

    tx_len = len;
    rx_len = len;

    /* Asserts the CS pin(line) */
    SPIDat1Config(SOC_SPI_0_REGS, SPI_DATA_FORMAT0, CS_VALUE);

    SPIIntEnable(SOC_SPI_0_REGS, (SPI_RECV_INT | SPI_TRANSMIT_INT));
    while(flag);
    flag = 1;

    /* Deasserts the CS pin(line) */
    SPIDat1Config(SOC_SPI_0_REGS, SPI_DATA_FORMAT0, CS_VALUE);

    return SPI_OK;
}
