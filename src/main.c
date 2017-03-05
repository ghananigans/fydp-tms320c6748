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
#include "mcasp_wrapper/mcasp_wrapper.h"

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
     * Init system interrupts.
     */
    ret_val = init_interrupt();
    ASSERT(ret_val == INTERRUPT_OK, "Interrupt init failed! (%d)\n", ret_val);

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

    /*
     * Init McASP
     */
    ret_val = mcasp_init();
    ASSERT(ret_val == DAC_OK, "McASP init failed! (%d)\n", ret_val);

    /*
     * Init Timer
     */
    ret_val = timer_init(&timer_function, TIMER_MICROSECONDS);
    ASSERT(ret_val == TIMER_OK, "Timer Init failed! (%d)\n", ret_val);

    // Power on dac channel
    ret_val = dac_power_up(CHANNEL_POWER);
    ASSERT(ret_val == DAC_OK, "DAC power on failed! (%d)\n", ret_val);

    /*
     * Loop back test for McASP
     */
    mcasp_loopback_test();

    /*
     * Loop forever.
     */
    DEBUG_PRINT("Looping forever...\n");
    while (1);

    return 0;
}
