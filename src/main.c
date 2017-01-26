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
    ret_val = timer_init(&timer_function, 1000);
    ASSERT(ret_val == TIMER_OK, "Timer Init failed! (%d)\n", ret_val);

    /*
     * Loop 10 times to see timer ticking 10 times.
     */
    a = 10;
    while (a < 10)
    {
        while (timer_flag == 0);
        timer_flag = 0;

        DEBUG_PRINT("TIMER TICKED %d!\n", a++);
    }

    DEBUG_PRINT("Sending spi data\n");
    buffer[0] = 0xff;
    buffer[1] = 0xaa;
    buffer[2] = 0x00;
    buffer[3] = 0x55;
    buffer[4] = 0xff;

    ret_val = spi_send_and_receive(buffer, 5, 1);
    ASSERT(ret_val == SPI_OK, "SPI send failed! (%d)\n", ret_val);
    for (a = 0; a < 50; ++a)
	{
		DEBUG_PRINT("buffer[%d] = %d\n", a, buffer[a]);
	}

    ret_val = dac_update(0, 0);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    ret_val = dac_update(5, 0x1234);
    ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

    /*
     * Check how long it takes to do float operations
     */
    double x = 3.3;
    double y = 3.5;
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
