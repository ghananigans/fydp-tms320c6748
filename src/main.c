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
#include <stdbool.h>
#include <time.h>
#include <math.h>


#define CHANNEL_POWER DAC_POWER_ON_CHANNEL_7
#define CHANNEL DAC_ADDRESS_CHANNEL_7

#ifndef PI
#define PI (3.14159265358979323846)
#endif

#define SAMPLING_FREQUENCY (50000)
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
     * Try reading uart and printing it back.
     */
    /*uart_print("Enter some text : ");
    uart_read(buffer, 50);

    for (a = 0; a < 50; ++a)
    {
        DEBUG_PRINT("buffer[%d] = %d\n", a, buffer[a]);
    }

    uart_print("%s\n", buffer);*/

    /*
     * Init the timer.
     */
    ret_val = timer_init(&timer_function, TIMER_MICROSECONDS);
    ASSERT(ret_val == TIMER_OK, "Timer Init failed! (%d)\n", ret_val);

    /*
     * Loop 10 times to see timer ticking 10 times.
     */
    /*
    timer_start();
    a = 0;
    while (a < 10)
    {
        while (timer_flag == 0);
        timer_flag = 0;

        DEBUG_PRINT("TIMER TICKED %d!\n", a++);
    }
    timer_stop();
    */

    // Power on dac channel
    ret_val = dac_power_up(CHANNEL_POWER);
    ASSERT(ret_val == DAC_OK, "DAC power on failed! (%d)\n", ret_val);

    /*
    // Channel to 0
    ret_val = dac_update(CHANNEL, (uint16_t) 0);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    // Channel ~ MAX_VOLTAGE * (1/5)
    ret_val = dac_update(CHANNEL, (uint16_t) 13000);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    // Channel ~ MAX_VOLTAGE * (2/5)
    ret_val = dac_update(CHANNEL, (uint16_t) 26000);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    // Channel ~ MAX_VOLTAGE * (3/5)
    ret_val = dac_update(CHANNEL, (uint16_t) 39000);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    // Channel ~ MAX_VOLTAGE * (4/5)
    ret_val = dac_update(CHANNEL, (uint16_t) 52000);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    // Channel ~ MAX_VOLTAGE
    ret_val = dac_update(CHANNEL, (uint16_t) 65000);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);
    */

    /*
     * Pre calculate samples for simple sinusoid output to dac.
     */
    x = 0;
    for (i = 0; i < NUM_SAMPLES; ++i)
    {
        y = (cos(2 * COS_FREQ * PI * x) + 1) / 0.00007629394;
        x += 1.0 / SAMPLING_FREQUENCY;
        val[i] = (uint16_t) y;
    }

    i = 0;
    timer_start();
    while (1)
    {
        ASSERT(timer_flag == 0, "TIMER IS TOO FAST!\n");
        while (timer_flag == 0);
        timer_flag = 0;

        ret_val = dac_update(CHANNEL, val[i]);
        ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

        if (++i == NUM_SAMPLES)
        {
            i = 0;
        }
    }
    timer_stop();

    /*
     * Check how long it takes to do float operations
     */
    x = 3.3;
    y = 3.5;
    double z = 3.8;
    time_t sec = time(NULL);
    time_t sec2;
    for (a = 0; a < 125000000; ++a)
    {
        z = ++x * y;

    }
    sec2 = time(NULL);

    DEBUG_PRINT("TIME DIFF = %d\n", sec2 - sec);

    /*
     * Loop forever.
     */
    DEBUG_PRINT("Looping forever...\n");
    while (1);

    return 0;
}
