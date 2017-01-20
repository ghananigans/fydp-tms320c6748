/*
 * uart_wrapper.h
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef UART_WRAPPER_H_
#define UART_WRAPPER_H_

void
uart_init (
	void
	);

void
uart_print (
	char * str,
	unsigned int length
	);

void
uart_read (
	char * buffer,
	unsigned int buffer_size
	);

int
echo_uart (
	void
	);


#endif /* UART_WRAPPER_H_ */
