/*
 * uart_wrapper.h
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef UART_WRAPPER_H_
#define UART_WRAPPER_H_

enum UART_ERROR_CODES
{
	UART_OK,							// Everything is OK,
	UART_NOT_INITIALIZED,				// UART has not been initialized,
	UART_ALREADY_INITIALIZED,			// UART is already initialized (trying to init again),
	UART_INTERRUPTS_NOT_INITIALIZED		// Trying to init but system interrupts have not been initialized.
};

int
uart_init (
	void
	);

int
uart_print (
	char const * format,
	...
	);

int
uart_read (
	char * buffer,
	unsigned int buffer_size
	);


#endif /* UART_WRAPPER_H_ */
