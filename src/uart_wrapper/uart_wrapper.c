/*
 * uart_wrapper.c
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#include "uart_wrapper.h"
#include "hw_psc_C6748.h"
#include "soc_C6748.h"
#include "interrupt.h"
#include "lcdkC6748.h"
#include "hw_types.h"
#include "uart.h"
#include "psc.h"
#include "../util.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "../interrupt_wrapper/interrupt_wrapper.h"


/****************************************************************************/
/*                      GLOBAL VARIABLES                                    */
/****************************************************************************/
static bool init_done = 0;

static char tx_array[256];
static const unsigned int tx_length = 256;
static volatile unsigned int tx_index = 0;
static volatile bool tx_done = 1;

static char * rx_array = 0;
static unsigned int rx_length = 0;
static volatile unsigned int rx_index = 0;
static volatile bool rx_done = 1;


/****************************************************************************/
/*                      LOCAL FUNCTIONS                                     */
/****************************************************************************/

/*
** \brief   Interrupt Service Routine(ISR) to be executed on UART interrupts.
**          Depending on the source of interrupt, this
**          1> writes to the serial communication console, or
**          2> reads from the serial communication console, or
**          3> reads the byte in RBR if receiver line error has occured.
*/
static
void
uart_isr (
	void
	)
{
	unsigned int int_id = 0;

	/* This determines the cause of UART2 interrupt.*/
	int_id = UARTIntStatus(SOC_UART_2_REGS);

#ifdef _TMS320C6X
	// Clear UART2 system interrupt in DSPINTC
	IntEventClear(SYS_INT_UART2_INT);
#else
	/* Clears the system interupt status of UART2 in AINTC. */
	IntSystemStatusClear(SYS_INT_UARTINT2);
#endif

	/* Checked if the cause is transmitter empty condition.*/
	if (UART_INTID_TX_EMPTY == int_id)
	{
		if (tx_index < tx_length)
		{
			/* Write a byte into the THR if THR is free. */
			UARTCharPutNonBlocking(SOC_UART_2_REGS, tx_array[tx_index]);

			tx_index++;
		}

		/*
		 * If there is no array, or we sent the last character,
		 * disable furthur transmit interrupts.
		 */
		if (tx_index == tx_length || tx_array[tx_index] == '\0')
		{
			tx_done = 1;

			/* Disable the Transmitter interrupt in UART.*/
			UARTIntDisable(SOC_UART_2_REGS, UART_INT_TX_EMPTY);
		}
	 }

	/*
	 * Check if the cause is receiver data condition.
	 * If it is, return what was sent so it is visible
	 * on the UART terminal.
	 */
	if (UART_INTID_RX_DATA == int_id)
	{
		if (rx_array && rx_index < rx_length)
		{
			/* Store a bite into the receiver buffer */
			rx_array[rx_index] = UARTCharGetNonBlocking(SOC_UART_2_REGS);
			rx_index++;
		}

		/*
		 * If there is no array, or there is no more space in the receiving buffer
		 * or the last character was a newline, disable furthur read interrupts.
		 */
		if (!rx_array || rx_index == rx_length || (rx_array[rx_index - 1] == '\r'))
		{
			rx_done = 1;

			/* Disable the Transmitter interrupt in UART.*/
			UARTIntDisable(SOC_UART_2_REGS, UART_INT_RXDATA_CTI);
		}

		UARTCharPutNonBlocking(SOC_UART_2_REGS, rx_array[rx_index - 1]);
	}


	/* Check if the cause is receiver line error condition.*/
	if (UART_INTID_RX_LINE_STAT == int_id)
	{
		while (UARTRxErrorGet(SOC_UART_2_REGS))
		{
			/* Read a byte from the RBR if RBR has data.*/
			UARTCharGetNonBlocking(SOC_UART_2_REGS);
		}
	}

	return;
}

/*
** \brief  This function confiugres the AINTC to receive UART interrupts.
*/
static
void
uart_configure_interrupts (
	void
	)
{
#ifdef _TMS320C6X
	IntRegister(C674X_MASK_INT4, uart_isr);
	IntEventMap(C674X_MASK_INT4, SYS_INT_UART2_INT);
	IntEnable(C674X_MASK_INT4);
#else
    /* Registers the UARTIsr in the Interrupt Vector Table of AINTC. */
    IntRegister(SYS_INT_UARTINT2, uart_isr);

    /* Map the channel number 2 of AINTC to UART2 system interrupt. */
    IntChannelSet(SYS_INT_UARTINT2, 2);

    IntSystemEnable(SYS_INT_UARTINT2);
#endif
}


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
	unsigned int intFlags = 0;
	unsigned int config = 0;

	if (init_done)
	{
		DEBUG_PRINT("UART is already initialized!\n");
		return UART_ALREADY_INITIALIZED;
	}

	DEBUG_PRINT("Initializing UART...\n");

	if (is_interrupt_init_done() == 0)
	{
		DEBUG_PRINT("Interrupts have not been initialized!\n");
		return UART_INTERRUPTS_NOT_INITIALIZED;
	}

	/* Enabling the PSC for UART2.*/
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_UART2, PSC_POWERDOMAIN_ALWAYS_ON,
			 PSC_MDCTL_NEXT_ENABLE);

	/* Setup PINMUX */
	UARTPinMuxSetup(2, FALSE);

	/* Enabling the transmitter and receiver*/
	UARTEnable(SOC_UART_2_REGS);

	/* 1 stopbit, 8-bit character, no parity */
	config = UART_WORDL_8BITS;

	/* Configuring the UART parameters*/
	UARTConfigSetExpClk(SOC_UART_2_REGS, SOC_UART_2_MODULE_FREQ,
						BAUD_115200, config,
						UART_OVER_SAMP_RATE_16);

	/* Enabling the FIFO and flushing the Tx and Rx FIFOs.*/
	UARTFIFOEnable(SOC_UART_2_REGS);

	/* Setting the UART Receiver Trigger Level*/
	UARTFIFOLevelSet(SOC_UART_2_REGS, UART_RX_TRIG_LEVEL_1);

	/* Configure AINTC to receive and handle UART interrupts. */
	uart_configure_interrupts();

	/* Preparing the 'intFlags' variable to be passed as an argument.*/
	intFlags |= (UART_INT_LINE_STAT);

	/* Enable the Interrupts in UART.*/
	UARTIntEnable(SOC_UART_2_REGS, intFlags);

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
	tx_index = 0;
	tx_done = 0;

	/* Enable the Transmitter interrupt in UART.*/
	UARTIntEnable(SOC_UART_2_REGS, UART_INT_TX_EMPTY);

	//
	// Wait to finish sending data (blocking).
	//
	while (tx_done == 0);

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
	rx_length = buffer_size - 1;
	rx_array = buffer;
	rx_index = 0;
	rx_done = 0;

	/* Enable the Transmitter interrupt in UART.*/
	UARTIntEnable(SOC_UART_2_REGS, UART_INT_RXDATA_CTI);

	//
	// Wait to finish receiving data (blocking).
	//
	while (rx_done == 0);

	rx_array = 0;
	rx_length = 0;
	rx_index = 0;

	return UART_OK;
}
