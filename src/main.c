/*
 * main.c
 */

#ifndef _TMS320C6X
#error Macro _TMS320C6X is not defined!
#endif

#include "util.h"
#include "uart_wrapper/uart_wrapper.h"
#include "timer_wrapper/timer_wrapper.h"
#include "spi_wrapper/spi_wrapper.h"
#include <stdbool.h>

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
    ASSERT(ret_val == UART_OK, "UART Init failed!\n");

    NORMAL_PRINT("Application Starting\n");
    DEBUG_PRINT("Debug prints are enabled\n");

    /*
     * Try reading uart and printing it back.
     */
    uart_print("Enter some text : ");
    uart_read(buffer, 50);

    for (a = 0; a < 50; ++a)
    {
        DEBUG_PRINT("buffer[%d] = %d\n", a, buffer[a]);
    }

    uart_print("%s\n", buffer);

    /*
     * Init the timer.
     */
    ret_val = timer_init(&timer_function, 1000);
    ASSERT(ret_val == TIMER_OK, "Timer Init failed!\n");

    /*
     * Loop 10 times to see timer ticking 10 times.
     */
    a = 0;
    while (a < 10)
    {
        while (timer_flag == 0);
        timer_flag = 0;

        DEBUG_PRINT("TIMER TICKED %d!\n", a++);
    }

    /*
     * Init spi.
     */
    ret_val = spi_init();
    ASSERT(ret_val == SPI_OK, "SPI init failed!\n");

    DEBUG_PRINT("Sending spi data\n");
    spi_send(buffer, 50);

    /*
     * Loop forever.
     */
    DEBUG_PRINT("Looping forever...\n");
    while (1);

    return 0;
}
