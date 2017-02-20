/*
 * main.c
 */

#ifndef _TMS320C6X
#error Macro _TMS320C6X is not defined!
#endif

#include "util.h"
#include "interrupt_wrapper/interrupt_wrapper.h"
#include "uart_wrapper/uart_wrapper.h"
#include "timer_wrapper/timer_wrapper.h"
#include "spi_wrapper/spi_wrapper.h"
#include "dac_wrapper/dac_wrapper.h"
#include "edma3_wrapper/edma3_wrapper.h"
#include "mcasp/mcaspPlayBk.h"
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <stdint.h>


#define CHANNEL_POWER DAC_POWER_ON_CHANNEL_7
#define CHANNEL DAC_ADDRESS_CHANNEL_7

#ifndef PI
#define PI (3.14159265358979323846)
#endif

#define SAMPLING_FREQUENCY (10000)
#define COS_FREQ           (5000)
#define NUM_SAMPLES        (SAMPLING_FREQUENCY / COS_FREQ)
#define TIMER_MICROSECONDS (1000000 / SAMPLING_FREQUENCY)

static bool volatile timer_flag = 0;

static inline
void
timer_function (
    void
    )
{
    timer_flag = 1;
}

int
main (
    void
    )
{
    int a;
    int ret_val;
    char buffer[50];
    double x;
    double y;
    uint16_t val[NUM_SAMPLES];
    int i;

    /*
     * Init uart before everything else so debug
     * statements can be seen
     */
    ret_val = uart_init();
    ASSERT(ret_val == UART_OK, "UART Init failed! (%d)\n", ret_val);

    NORMAL_PRINT("Application Starting\n");
    DEBUG_PRINT("Debug prints are enabled\n");
    /*
     * Init system interrupts.
     */
    ret_val = init_interrupt();
    ASSERT(ret_val == INTERRUPT_OK, "Interrupt init failed! (%d)\n", ret_val);

    /*
     * Init system interrupts.
     */
    ret_val = edma3_init();
    ASSERT(ret_val == EDMA3_OK, "EDMA3 init failed! (%d)\n", ret_val);


    /*
     * Init spi.
     */
    ret_val = spi_init();
    ASSERT(ret_val == SPI_OK, "SPI init failed! (%d)\n", ret_val);

    /*
     * Init dac.
     */
    ret_val = dac_init(1);
    ASSERT(ret_val == DAC_OK, "DAC init failed! (%d)\n", ret_val);


    ret_val = timer_init(&timer_function, TIMER_MICROSECONDS);
    ASSERT(ret_val == TIMER_OK, "Timer Init failed! (%d)\n", ret_val);

    // Power on dac channel
    ret_val = dac_power_up(CHANNEL_POWER);
    ASSERT(ret_val == DAC_OK, "DAC power on failed! (%d)\n", ret_val);

   DEBUG_PRINT("about to do i2c pinmuxsetup.\n");

    unsigned short parToSend;
    unsigned short parToLink;

    /* Set up pin mux for I2C module 0 */
    I2CPinMuxSetup(0);
    DEBUG_PRINT("about to do McASPPinMuxSetup.\n");
    McASPPinMuxSetup();

    DEBUG_PRINT("about to do PSCModuleControl.\n");
    /* Power up the McASP module */
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_MCASP0, PSC_POWERDOMAIN_ALWAYS_ON,
             PSC_MDCTL_NEXT_ENABLE);

    /* Power up EDMA3CC_0 and EDMA3TC_0 */
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_CC0, PSC_POWERDOMAIN_ALWAYS_ON,
             PSC_MDCTL_NEXT_ENABLE);
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_TC0, PSC_POWERDOMAIN_ALWAYS_ON,
             PSC_MDCTL_NEXT_ENABLE);

    DEBUG_PRINT("about to do IntDSPINTCInit.\n");
    // Initialize the DSP interrupt controller
    IntDSPINTCInit();

    DEBUG_PRINT("about to do codec ifinit.\n");
    /* Initialize the I2C 0 interface for the codec AIC31 */
    I2CCodecIfInit(SOC_I2C_0_REGS, INT_CHANNEL_I2C, I2C_SLAVE_CODEC_AIC31);

    DEBUG_PRINT("edma3initi.\n");
    EDMA3Init(SOC_EDMA30CC_0_REGS, 0);
    EDMA3IntSetup();

    DEBUG_PRINT("about to do mcasperrorintsetup.\n");
    McASPErrorIntSetup();

    IntGlobalEnable();

    DEBUG_PRINT("about to do EDMA3RequestChannel.\n");

    /*
    ** Request EDMA channels. Channel 0 is used for reception and
    ** Channel 1 is used for transmission
    */
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA,
                        EDMA3_CHA_MCASP0_TX, EDMA3_CHA_MCASP0_TX, 0);
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA,
                        EDMA3_CHA_MCASP0_RX, EDMA3_CHA_MCASP0_RX, 0);

    /* Initialize the DMA parameters */
    I2SDMAParamInit();

    /* Configure the Codec for I2S mode */
    AIC31I2SConfigure();

    /* Configure the McASP for I2S */
    McASPI2SConfigure();

    /* Activate the audio transmission and reception */
    I2SDataTxRxActivate();

    /*
    ** Looop forever. if a new buffer is received, the lastFullRxBuf will be
    ** updated in the rx completion ISR. if it is not the lastSentTxBuf,
    ** buffer is to be sent. This has to be mapped to proper paRAM set.
    */
    while(1)
    {
        if(lastFullRxBuf != lastSentTxBuf)
        {
            /*
            ** Start the transmission from the link paramset. The param set
            ** 1 will be linked to param set at PAR_TX_START. So do not
            ** update paRAM set1.
            */
            parToSend =  PAR_TX_START + (parOffTxToSend % NUM_PAR);
            parOffTxToSend = (parOffTxToSend + 1) % NUM_PAR;
            parToLink  = PAR_TX_START + parOffTxToSend;

            lastSentTxBuf = (lastSentTxBuf + 1) % NUM_BUF;

            /* Copy the buffer */
            memcpy((void *)txBufPtr[lastSentTxBuf],
                   (void *)rxBufPtr[lastFullRxBuf],
                   AUDIO_BUF_SIZE);

            /*
            ** Send the buffer by setting the DMA params accordingly.
            ** Here the buffer to send and number of samples are passed as
            ** parameters. This is important, if only transmit section
            ** is to be used.
            */
            BufferTxDMAActivate(lastSentTxBuf, NUM_SAMPLES_PER_AUDIO_BUF,
                                (unsigned short)parToSend,
                                (unsigned short)parToLink);
        }
    }
    /*
     * Loop forever.
     */
    DEBUG_PRINT("Looping forever...\n");
    while (1);

    return 0;
}
