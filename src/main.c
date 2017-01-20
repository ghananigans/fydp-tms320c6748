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
	int i, a;
	char buffer[50];
	char other_buffer[20];

	NORMAL_PRINT("Application Starting\n");
	DEBUG_PRINT("Debug prints are enabled\n");

	uart_init();

	/*
	 * Loop forever.
	 */
	i = 0;

	while (1)
	{
		i++;
		uart_print("Enter some text : ", -1);
		uart_read(buffer, 50);

		for (a = 0; a < 50; ++a)
		{
			DEBUG_PRINT("buffer[%d] = %d\n", a, buffer[a]);
		}

		uart_print(buffer, 50);
	}

	return 0;
}
