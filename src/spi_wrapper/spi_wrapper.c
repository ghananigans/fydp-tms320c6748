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
#define SIMO_SOMI_CLK_CS        (0x00000E30)
#define CHAR_LENGTH             (0x8)
#define	DATA_FORMAT             SPI_DATA_FORMAT0
#define SPI_REG                 SOC_SPI_0_REGS
#define SPI_CS0                 (0x10)
#define SPI_CS1                 (0x20)

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
                flag = 0;
                SPIIntDisable(SPI_REG, SPI_RECV_INT);
            }
        }

        intCode = SPIInterruptVectorGet(SPI_REG);
    }
}

/*
 * Configures SPI Controller.
 */
static
void
spi_setup (
    void
    )
{
    unsigned int  val = SIMO_SOMI_CLK_CS;

    /*
     * Register the ISR in the vector table
     */
    IntRegister(C674X_MASK_INT4, spi_isr);

    /*
     * Map system interrupt to the DSP maskable interrupt
     */
    IntEventMap(C674X_MASK_INT4, SYS_INT_SPI0_INT);

    // Enable the DSP maskable interrupt
    IntEnable(C674X_MASK_INT4);

    SPIReset(SPI_REG);

    SPIOutOfReset(SPI_REG);

    SPIModeConfigure(SPI_REG, SPI_MASTER_MODE);

    SPIPinControl(SPI_REG, 0, 0, &val);

    SPIClkConfigure(SPI_REG, 150000000, 1000000, DATA_FORMAT);

    /*
     * Configures SPI Data Format Register
     */

    /*
     * Configures the polarity and phase of SPI clock
     */
    SPIConfigClkFormat(SPI_REG,
                        (SPI_CLK_POL_LOW | SPI_CLK_INPHASE),
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
    //SPIDat1Config(SPI_REG, (SPI_SPIDAT1_CSHOLD | DATA_FORMAT), 0);
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL | DATA_FORMAT), 0);

    /*
     * Maximum delay from CS edges to start and end of transmission
     * of data.
     */
    SPIDelayConfigure(SPI_REG, 0, 0, 0xFF, 0xFF);

    /*
     * Delay between transactions (max ammount of delay).
     */
    //SPIWdelaySet(SPI_REG, 0x3F, DATA_FORMAT);

     /*
      * map interrupts to interrupt line INT1
      */
    SPIIntLevelSet(SPI_REG, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);

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

    /*
     * Waking up the SPI1 instance.
     */
    PSCModuleControl(SPI_REG, HW_PSC_SPI0, PSC_POWERDOMAIN_ALWAYS_ON,
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

    /*
     * Initialize SPI w/ Interrupts.
     */

    spi_setup();

    init_done = 1;

    DEBUG_PRINT("Done initializing spi!\n");

    return SPI_OK;
}

/*
 * Enables SPI Transmit and Receive interrupt.
 * Deasserts Chip Select line.
 *
 * CS should be 0 or 1 only.
 */
int
spi_send_and_receive (
    unsigned char * data,
    unsigned int len,
    unsigned int cs
    )
{
    if (cs > 1)
    {
        ERROR_PRINT("CS is not 0 or 1; Only supporting 2 CS (Got: %d)", cs);
        return SPI_INVALID_CS;
    }

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

    p_tx = data;
    p_rx = data;

    tx_len = len;
    rx_len = len;

    /*
     * Assert the CS pin.
     */
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL
                | SPI_SPIDAT1_CSHOLD | DATA_FORMAT), cs);

    /*
     * Enable the interrupts.
     */
    SPIIntEnable(SPI_REG, (SPI_RECV_INT | SPI_TRANSMIT_INT));

    /*
     * Wait for flag to change (transaction to finish).
     */
    while (flag);
    flag = 1;

    /*
     * Deasserts the CS pin. (Notice no CSHOLD flag)
     */
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL
                | DATA_FORMAT), cs);

    return SPI_OK;
}
