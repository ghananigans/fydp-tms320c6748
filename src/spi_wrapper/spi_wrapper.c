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
#include "edma.h"
#include "edma_event.h"
#include "hw_types.h"
#include <string.h>
#include "dspcache.h"

/******************************************************************************/
/*                      INTERNAL MACRO DEFINITIONS                            */
/******************************************************************************/
/* value to configure SMIO,SOMI,CLK and CS pin as functional pin */
#define SIMO_SOMI_CLK_CS        (0x00000E30)
#define CHAR_LENGTH             (0x8)
#define	DATA_FORMAT             SPI_DATA_FORMAT0
#define SPI_REG                 SOC_SPI_0_REGS
#define SPI_CS0                 (0x10)
#define SPI_CS1                 (0x20)
#define SPI_SCLK_HZ             (5000000)

/****************************************************************************/
/*                      INTERNAL GLOBAL VARIABLES                           */
/****************************************************************************/
static bool init_done = 0;
static void (*callback_functions[EDMA3_NUM_TCC]) (
    unsigned int tcc,
    unsigned int status
    );
static volatile bool tx_flag = 0;
static volatile bool rx_flag = 0;
static unsigned int event_queue = 0;

/******************************************************************************/
/*                      INTERNAL FUNCTION DEFINITIONS                         */
/******************************************************************************/
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
** EDMA3 completion Interrupt Service Routine(ISR).
*/

static
void
emda3_completion_handler_isr (
    void
    )
{
    volatile unsigned int pending_irqs;
    volatile unsigned int is_ipr = 0;

    volatile unsigned int indexl;
    volatile unsigned int cnt = 0;
    indexl = 1;
#ifdef _TMS320C6X
    IntEventClear(SYS_INT_EDMA3_0_CC0_INT1);
#else
    IntSystemStatusClear(SYS_INT_CCINT0);
#endif
    is_ipr = EDMA3GetIntrStatus(SOC_EDMA30CC_0_REGS);

    if (is_ipr)
    {
        while ((cnt < EDMA3CC_COMPL_HANDLER_RETRY_COUNT) && (indexl != 0))
        {
            indexl = 0;
            pending_irqs = EDMA3GetIntrStatus(SOC_EDMA30CC_0_REGS);

            while (pending_irqs)
            {
                if ((pending_irqs & 1) == TRUE)
                {
                    /**
                     * If the user has not given any callback function
                     * while requesting the TCC, its TCC specific bit
                     * in the IPR register will NOT be cleared.
                     */
                    /* Here write to ICR to clear the corresponding IPR bits. */
                    EDMA3ClrIntr(SOC_EDMA30CC_0_REGS, indexl);
                    (*callback_functions[indexl])(indexl, EDMA3_XFER_COMPLETE);
                }

                ++indexl;
                pending_irqs >>= 1;
            }

            cnt++;
        }
    }
}

/*
** EDMA3 Error Interrupt Service Routine(ISR).
*/
static
void
edma3_ccerr_handler_isr (
    void
    )
{
    volatile unsigned int pending_irqs = 0;
    unsigned int cnt = 0;
    unsigned int index = 1;
    unsigned int region_num = 0;
    unsigned int event_queue_num = 0;

#ifdef _TMS320C6X
    IntEventClear(SYS_INT_EDMA3_0_CC0_ERRINT);
#else
    IntSystemStatusClear(SYS_INT_CCERRINT);
#endif

    if ((HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_EMR) != 0 )
        || (HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_QEMR) != 0)
        || (HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERR) != 0))
    {
        /* Loop for EDMA3CC_ERR_HANDLER_RETRY_COUNT number of time, breaks
           when no pending interrupt is found. */
        while ((cnt < EDMA3CC_ERR_HANDLER_RETRY_COUNT) && (index != 0))
        {
            index = 0;
            pending_irqs = HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_EMR);

            while (pending_irqs)
            {
                /*Process all the pending interrupts.*/
                if ((pending_irqs & 1)==TRUE)
                {
                    /* Write to EMCR to clear the corresponding EMR bits.*/
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_EMCR) = (1<<index);
                    /*Clear any SER*/
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_S_SECR(region_num)) = (1<<index);
                }

                ++index;
                pending_irqs >>= 1;
            }

            index = 0;
            pending_irqs = HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_QEMR);

            while (pending_irqs)
            {
                /*Process all the pending interrupts.*/
                if ((pending_irqs & 1)==TRUE)
                {
                    /* Here write to QEMCR to clear the corresponding QEMR bits.*/
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_QEMCR) = (1<<index);
                    /*Clear any QSER*/
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_S_QSECR(0)) = (1<<index);
                }

                ++index;
                pending_irqs >>= 1;
            }

            index = 0;
            pending_irqs = HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERR);

            if (pending_irqs != 0)
            {
                /* Process all the pending CC error interrupts. */
                /* Queue threshold error for different event queues.*/
                for (event_queue_num = 0; event_queue_num < EDMA3_0_NUM_EVTQUE; event_queue_num++)
                {
                    if ((pending_irqs & (1 << event_queue_num)) != 0)
                    {
                        /* Clear the error interrupt. */
                        HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERRCLR) = (1 << event_queue_num);
                    }
                }

                /* Transfer completion code error. */
                if ((pending_irqs & (1 << EDMA3CC_CCERR_TCCERR_SHIFT)) != 0)
                {
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERRCLR) = \
                         (0x01 << EDMA3CC_CCERR_TCCERR_SHIFT);
                }

                ++index;
            }

            cnt++;
        }
    }
}

static
void
edma3_init (
    void
    )
{
    /*
     * Enabling the PSC for EDMA3CC_0).
     */
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_CC0, PSC_POWERDOMAIN_ALWAYS_ON,
                     PSC_MDCTL_NEXT_ENABLE);

    /* Enabling the PSC for EDMA3TC_0.*/
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_TC0, PSC_POWERDOMAIN_ALWAYS_ON,
                     PSC_MDCTL_NEXT_ENABLE);

    /*
     * Initialization of EDMA3
     */
    EDMA3Init(SOC_EDMA30CC_0_REGS, event_queue);

    /*
     * Register EDMA3 Interrupts
     */
#ifdef _TMS320C6X
    IntRegister(C674X_MASK_INT4, emda3_completion_handler_isr);
    IntRegister(C674X_MASK_INT5, edma3_ccerr_handler_isr);

    IntEventMap(C674X_MASK_INT4, SYS_INT_EDMA3_0_CC0_INT1);
    IntEventMap(C674X_MASK_INT5, SYS_INT_EDMA3_0_CC0_ERRINT);

    IntEnable(C674X_MASK_INT4);
    IntEnable(C674X_MASK_INT5);
#else
    IntRegister(SYS_INT_CCINT0, emda3_completion_handler_isr);

    IntChannelSet(SYS_INT_CCINT0, 2);

    IntSystemEnable(SYS_INT_CCINT0);

    IntRegister(SYS_INT_CCERRINT, edma3_ccerr_handler_isr);

    IntChannelSet(SYS_INT_CCERRINT, 2);

    IntSystemEnable(SYS_INT_CCERRINT);
#endif

}

static
void
request_edma3_channels (
    void
    )
{
    /* Request DMA Channel and TCC for SPI Transmit*/
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, \
                        EDMA3_CHA_SPI0_TX, EDMA3_CHA_SPI0_TX, event_queue);

    /* Request DMA Channel and TCC for SPI Receive*/
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, \
                        EDMA3_CHA_SPI0_RX, EDMA3_CHA_SPI0_RX, event_queue);
}

/*
** This function is used to set the PaRAM entries of EDMA3 for the Transmit
** Channel of SPI0. The corresponding EDMA3 channel is also enabled for
** transmission.
*/
static
void
edma3_param_set_spi_tx (
    unsigned int tcc_num,
    unsigned int channel_num,
    char volatile * buffer,
    unsigned short length
    )
{
    EDMA3CCPaRAMEntry paramSet;

    /* Clean-up the contents of structure variable. */
    memset(&paramSet, 0, sizeof(EDMA3CCPaRAMEntry));

    /* Fill the PaRAM Set with transfer specific information. */

    /* srcAddr holds address of memory location buffer. */
    paramSet.srcAddr = (unsigned int) buffer;

    /* destAddr holds address of SPIDAT1 register. */
    paramSet.destAddr = (unsigned int) (SPI_REG + SPI_SPIDAT1);

    /* aCnt holds the number of bytes in an array. */
    paramSet.aCnt = 1;

    /* bCnt holds the number of such arrays to be transferred. */
    paramSet.bCnt = length;

    /* cCnt holds the number of frames of aCnt*bBcnt bytes to be transferred. */
    paramSet.cCnt = 1;

    /*
    ** The srcBidx should be incremented by aCnt number of bytes since the
    ** source used here is  memory.
    */
    paramSet.srcBIdx = 1;

    /* A sync Transfer Mode is set in OPT.*/
    /* srCIdx and destCIdx set to zero since ASYNC Mode is used. */
    paramSet.srcCIdx =  0;

    /* Linking transfers in EDMA3 are not used. */
    paramSet.linkAddr = 0xFFFF;
    paramSet.bCntReload = 0;

    paramSet.opt = 0x00000000;

    /* SAM field in OPT is set to zero since source is memory and memory
       pointer needs to be incremented. DAM field in OPT is set to zero
       since destination is not a FIFO. */

    /* Set TCC field in OPT with the tccNum. */
    paramSet.opt |= ((tcc_num << EDMA3CC_OPT_TCC_SHIFT) & EDMA3CC_OPT_TCC);

    /* EDMA3 Interrupt is enabled and Intermediate Interrupt Disabled.*/
    paramSet.opt |= (1 << EDMA3CC_OPT_TCINTEN_SHIFT);

    /* Now write the PaRam Set to EDMA3.*/
    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, channel_num, &paramSet);

    /* EDMA3 Transfer is Enabled. */
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, channel_num, EDMA3_TRIG_MODE_EVENT);
}

/*
** This function is used to set the PaRAM entries of EDMA3 for the Receive
** Channel of SPI0. The corresponding EDMA3 channel is also enabled for
** reception.
*/

static
void
edma3_param_set_spi_rx (
    unsigned int tcc_num,
    unsigned int channel_num,
    char volatile * buffer,
    unsigned short length,
    unsigned int dest_bidx_flag
    )
{
    EDMA3CCPaRAMEntry paramSet;

    /* Clean-up the contents of structure variable. */
    memset(&paramSet, 0, sizeof(EDMA3CCPaRAMEntry));

    /* Fill the PaRAM Set with Receive specific information. */

    /* srcAddr holds address of SPI Rx FIFO. */
    paramSet.srcAddr = (unsigned int) (SPI_REG + SPI_SPIBUF);

    /* destAddr is address of memory location named buffer. */
    paramSet.destAddr = (unsigned int) buffer;

    /* aCnt holds the number of bytes in an array. */
    paramSet.aCnt = 1;

    /* bCnt holds the number of such arrays to be transferred. */
    paramSet.bCnt = length;

    /* cCnt holds the number of frames of aCnt*bBcnt bytes to be transferred. */
    paramSet.cCnt = 1;

    /* The srcBidx should not be incremented since it is a h/w register. */
    paramSet.srcBIdx = 0;

    if (TRUE == dest_bidx_flag)
    {
        /* The destBidx should be incremented for every byte. */
        paramSet.destBIdx = 1;
    }
    else
    {
        /* The destBidx should not be incremented. */
        paramSet.destBIdx = 0;
    }

    /* A sync Transfer Mode. */
    /* srCIdx and destCIdx set to zero since ASYNC Mode is used. */
    paramSet.srcCIdx = 0;
    paramSet.destCIdx = 0;

    /* Linking transfers in EDMA3 are not used. */
    paramSet.linkAddr = 0xFFFF;
    paramSet.bCntReload = 0;

    paramSet.opt = 0x00000000;

    /* Set TCC field in OPT with the tccNum. */
    paramSet.opt |= ((tcc_num << EDMA3CC_OPT_TCC_SHIFT) & EDMA3CC_OPT_TCC);

    /* EDMA3 Interrupt is enabled and Intermediate Interrupt Disabled.*/
    paramSet.opt |= (1 << EDMA3CC_OPT_TCINTEN_SHIFT);

    /* Now write the PaRam Set to EDMA3.*/
    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, channel_num, &paramSet);

    /* EDMA3 Transfer is Enabled. */
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, channel_num, EDMA3_TRIG_MODE_EVENT);
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
     * Initialize the EDMA3 instance.
     */
    edma3_init();

    /*
     * Initialize SPI w/ Interrupts.
     */
    spi_setup();

    /*
     * Request EDMA3CC for Tx an Rx channels for SPI0.
     */
    request_edma3_channels();

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
    char volatile * data,
    unsigned int len,
    unsigned int cs
    )
{
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

    /*
     * Writeback whatever is in cache to main memory.
     */
    CacheWB((unsigned int) data, len);

    DEBUG_PRINT("Sending and receiving spi data with cs: %d\n", cs);
    DEBUG_PRINT("Transaction starting...\n");

    /* Configure the PaRAM registers in EDMA for Transmission.*/
    edma3_param_set_spi_tx(EDMA3_CHA_SPI0_TX, EDMA3_CHA_SPI0_TX, data, len);

    /* Registering Callback Function for Transmission. */
    callback_functions[EDMA3_CHA_SPI0_TX] = &callback;

    /* Configure the PaRAM registers in EDMA for Reception.*/
    edma3_param_set_spi_rx(EDMA3_CHA_SPI0_RX, EDMA3_CHA_SPI0_RX, data, len, 1);

    /* Registering Callback Function for Reception. */
    callback_functions[EDMA3_CHA_SPI0_RX] = &callback;

    /*
     * Assert the CS pin.
     */
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL
                | SPI_SPIDAT1_CSHOLD | DATA_FORMAT), cs);

    /*
     * Enable the interrupts.
     */
    SPIIntEnable(SPI_REG, SPI_DMA_REQUEST_ENA_INT);


    /* Wait until both the flags are set to 1 in the callback function. */
	while ((0 == tx_flag) || (0 == rx_flag));
	tx_flag = 0;
	rx_flag = 0;

    /*
     * Deasserts the CS pin. (Notice no CSHOLD flag)
     */
    SPIDat1Config(SPI_REG, (SPI_SPIDAT1_WDEL
                | DATA_FORMAT), cs);

    DEBUG_PRINT("Transaction finished!\n");

    return SPI_OK;
}
