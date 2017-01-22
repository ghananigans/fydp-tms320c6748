/*
 * uart_wrapper.c
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#include "uartStdio.h"
#include "uart_wrapper.h"
#include "../util.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>


/****************************************************************************/
/*                      GLOBAL VARIABLES                                    */
/****************************************************************************/
static bool init_done = 0;

static char tx_array[256];
static const unsigned int tx_length = 256;

/****************************************************************************/
/*                      GLOBAL FUNCTION DEFINITIONS                         */
/****************************************************************************/

/*
 * Initialize UART.
 */
int
uart_init (
	void
	)
{
	if (init_done)
	{
		DEBUG_PRINT("UART is already initialized!\n");
		return UART_ALREADY_INITIALIZED;
	}

	DEBUG_PRINT("Initializing UART...\n");

	UARTStdioInit();

	init_done = 1;

	DEBUG_PRINT("UART Initialized!\n");

	return UART_OK;
}

/*
 * Print a formatted string to uart if uart is initialized.
 *
 * Use the same as printf.
 *
 * *** Max length of string can be 256 (including null terminator '\0).
 *
 * This function only returns after the full formatted string has
 * been sent (blocking).
 */
int
uart_print (
	char const * format,
	...
	)
{
	va_list args;

	if (init_done == 0)
	{
		return UART_NOT_INITIALIZED;
	}

	va_start(args, format);
	vsprintf(tx_array, format, args);
	va_end(args);

	UARTPuts(tx_array, tx_length);

	return UART_OK;
}

/*
 * Read something from UART.
 *
 * This function only returns after a CR is received or
 * bytes received equals to the buffer_size - 1. Last
 * element in the buffer will be the null terminator
 * character '\0'.
 */
int
uart_read (
	char * buffer,
	unsigned int buffer_size
	)
{
	if (init_done == 0)
	{
		return UART_NOT_INITIALIZED;
	}

	/*
	 * Always make sure last character in buffer
	 * is the null terminator character.
	 */
	memset(buffer, 0, buffer_size);

	UARTGets(buffer, buffer_size - 1);

	return UART_OK;
}
