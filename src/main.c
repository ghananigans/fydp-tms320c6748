/*
 * main.c
 */

#ifndef _TMS320C6X
#error Macro _TMS320C6X is not defined!
#endif

#include "util.h"
#include <stdio.h>
#include "uart_wrapper/uart_wrapper.h"

int
main (
	void
	)
{
	int a;
	char buffer[50];

	/*
	 * Init uart before everything else so debug
	 * statements can be seen
	 */
	uart_init();

	NORMAL_PRINT("Application Starting\n");
	DEBUG_PRINT("Debug prints are enabled\n");

	/*
	 * Loop forever.
	 * Simply read uart and output.
	 */
	while (1)
	{
		uart_print("Enter some text : ");
		uart_read(buffer, 50);

		for (a = 0; a < 50; ++a)
		{
			DEBUG_PRINT("buffer[%d] = %d\n", a, buffer[a]);
		}

		uart_print("%s\n", buffer);
	}

	return 0;
}
