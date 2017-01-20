/*
 * main.c
 */
#include "util.h"
#include <stdio.h>
#include "uart_wrapper/uart_wrapper.h"

int
main (
	void
	)
{
	NORMAL_PRINT("Application Starting\n");
	DEBUG_PRINT("Debug prints are enabled\n");

	echo_uart();

	/*
	 * Loop forever.
	 */
	while (1) {}

	return 0;
}
