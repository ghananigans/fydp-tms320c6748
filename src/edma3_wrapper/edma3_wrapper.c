/*
 * edma3_wrapper.c
 *
 *  Created on: Feb 8, 2017
 *      Author: Ghanan Gowripalan
 */

#include "edma3_wrapper.h"
#include "../util.h"
#include "soc_C6748.h"
#include "hw_psc_C6748.h"
#include "lcdkC6748.h"
#include "psc.h"
#include "edma.h"
#include "edma_event.h"
#include "interrupt.h"
#include "hw_types.h"
#include <string.h>

/****************************************************************************/
/*                      INTERNAL GLOBAL VARIABLES                           */
/****************************************************************************/
static bool init_done = 0;
static void (*callback_functions[EDMA3_NUM_TCC]) (
    unsigned int,
    unsigned int
    );
static unsigned int event_queue = 0;

/******************************************************************************/
/*                      INTERNAL FUNCTION DEFINITIONS                         */
/******************************************************************************/

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

    IntEventClear(SYS_INT_EDMA3_0_CC0_INT1);

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
                    if (callback_functions[indexl])
                    {
                        (*callback_functions[indexl])(indexl, EDMA3_XFER_COMPLETE);
                    }
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

    IntEventClear(SYS_INT_EDMA3_0_CC0_ERRINT);

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

/******************************************************************************/
/*                          FUNCTION DEFINITIONS                              */
/******************************************************************************/
int
edma3_init (
    void
    )
{
    if (init_done)
    {
        DEBUG_PRINT("EDMA3 is already initialized!\n");
        return EDMA3_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing EDMA3...\n");

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
    IntRegister(C674X_MASK_INT4, emda3_completion_handler_isr);
    IntRegister(C674X_MASK_INT5, edma3_ccerr_handler_isr);

    IntEventMap(C674X_MASK_INT4, SYS_INT_EDMA3_0_CC0_INT1);
    IntEventMap(C674X_MASK_INT5, SYS_INT_EDMA3_0_CC0_ERRINT);

    IntEnable(C674X_MASK_INT4);
    IntEnable(C674X_MASK_INT5);

    init_done = 1;

    DEBUG_PRINT("Done initializing EDMA3!\n");

    return EDMA3_OK;
}

int
edma3_set_callback (
    unsigned int tcc_num,
    void (*func) (
        unsigned int,
        unsigned int
    )
    )
{
    callback_functions[tcc_num] = func;

    return EDMA3_OK;
}

int
edma3_request_channel (
    unsigned int ch_num,
    unsigned int tcc_num
    )
{
    /* Request DMA Channel and TCC */
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA, \
                        ch_num, tcc_num, event_queue);

    return EDMA3_OK;
}

int
edma3_param_set (
    unsigned int tcc_num,
    unsigned int channel_num,
    char volatile * src_buffer,
    char volatile * dest_buffer,
    unsigned short num_bytes_in_array,
    unsigned short num_arrays_in_frame,
    unsigned short num_frames,
    bool increment_src_buffer_ptr,
    bool increment_dest_buffer_ptr
    )
{
    EDMA3CCPaRAMEntry paramSet;

    /* Clean-up the contents of structure variable. */
    memset(&paramSet, 0, sizeof(EDMA3CCPaRAMEntry));

    /* Fill the PaRAM Set with transfer specific information. */

    /* srcAddr holds address of memory location src_buffer. */
    paramSet.srcAddr = (unsigned int) src_buffer;

    /* destAddr holds address of SPIDAT1 register. */
    paramSet.destAddr = (unsigned int) dest_buffer;

    /* aCnt holds the number of bytes in an array. */
    paramSet.aCnt = num_bytes_in_array;

    /* bCnt holds the number of such arrays to be transferred. */
    paramSet.bCnt = num_arrays_in_frame;

    /* cCnt holds the number of frames of aCnt*bBcnt bytes to be transferred. */
    paramSet.cCnt = num_frames;

    if (increment_src_buffer_ptr)
    {
        /*
         * The srcBidx should be incremented by aCnt number of bytes.
        */
        paramSet.srcBIdx = (short) num_bytes_in_array;
    }

    if (increment_dest_buffer_ptr)
    {
        /*
         * The destBidx should be incremented by aCnt number of bytes.
        */
        paramSet.destBIdx = (short) num_bytes_in_array;
    }

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

    return EDMA3_OK;
}
