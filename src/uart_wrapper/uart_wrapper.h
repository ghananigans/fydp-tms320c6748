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
	UART_OK,
	UART_NOT_INITIALIZED,
	UART_ALREADY_INITIALIZED
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

int
echo_uart (
	void
	);


#endif /* UART_WRAPPER_H_ */
