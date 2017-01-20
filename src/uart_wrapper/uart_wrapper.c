/*
 * uart_wrapper.c
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#include "uart_wrapper.h"

/**
 * \file  uartEcho.c
 *
 * \brief This is a sample application file which invokes some APIs
 *        from the UART device abstraction library to perform configuration,
 *        transmission and reception operations.
 */

/*
* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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


/****************************************************************************/
/*                      GLOBAL VARIABLES                                    */
/****************************************************************************/
static char * tx_array;
static unsigned int tx_length;
static volatile unsigned int tx_index;
static volatile uint8_t tx_done;

static char * rx_array;
static unsigned int rx_length;
static volatile unsigned int rx_index;
static volatile uint8_t rx_done;

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
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
		if (tx_array && tx_index < tx_length)
		{
			/* Write a byte into the THR if THR is free. */
			UARTCharPutNonBlocking(SOC_UART_2_REGS, tx_array[tx_index]);
			tx_index++;
		}

		/*
		 * If there is no array, or we sent the last character,
		 * disable furthur transmit interrupts.
		 */
		if (!tx_array || tx_index == tx_length || tx_array[tx_index] == '\0')
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
		if (!rx_array || rx_index == rx_length || (rx_array[rx_index - 1] == 13))
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
** \brief   This function invokes necessary functions to configure the ARM
**          processor and ARM Interrupt Controller(AINTC) to receive and
**          handle interrupts.
*/
static
void
setup_interrupts (
	void
	)
{
#ifdef _TMS320C6X
	// Initialize the DSP INTC
	IntDSPINTCInit();

	// Enable DSP interrupts globally
	IntGlobalEnable();
#else
    /* Initialize the ARM Interrupt Controller(AINTC). */
    IntAINTCInit();

    /* Enable IRQ in CPSR.*/
    IntMasterIRQEnable();

    /* Enable the interrupts in GER of AINTC.*/
    IntGlobalEnable();

    /* Enable the interrupts in HIER of AINTC.*/
    IntIRQEnable();
#endif
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
    IntRegister(SYS_INT_UARTINT2, UARTIsr);

    /* Map the channel number 2 of AINTC to UART2 system interrupt. */
    IntChannelSet(SYS_INT_UARTINT2, 2);

    IntSystemEnable(SYS_INT_UARTINT2);
#endif
}


/****************************************************************************/
/*                      LOCAL FUNCTION DEFINITIONS                          */
/****************************************************************************/
void
uart_init (
	void
	)
{
	DEBUG_PRINT("Initializing UART...\n");

	unsigned int intFlags = 0;
	unsigned int config = 0;

	tx_array = 0;
	tx_length = 0;
	tx_index = 0;

	rx_array = 0;
	rx_length = 0;
	rx_index = 0;
	rx_done = 0;

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

	/*
	** Enable AINTC to handle interrupts. Also enable IRQ interrupt in ARM
	** processor.
	*/
	setup_interrupts();

	/* Configure AINTC to receive and handle UART interrupts. */
	uart_configure_interrupts();

	/* Preparing the 'intFlags' variable to be passed as an argument.*/
	intFlags |= (UART_INT_LINE_STAT);

	/* Enable the Interrupts in UART.*/
	UARTIntEnable(SOC_UART_2_REGS, intFlags);

	uart_print("UART Initialized!\r\n", 19);
}

void
uart_print (
	char * str,
	unsigned int length
	)
{
	tx_array = str;
	tx_length = length;
	tx_index = 0;
	tx_done = 0;

	/* Enable the Transmitter interrupt in UART.*/
	UARTIntEnable(SOC_UART_2_REGS, UART_INT_TX_EMPTY);

	while (tx_done == 0);

	tx_array = 0;
	tx_length = 0;
	tx_index = 0;
}

void
uart_read (
	char * buffer,
	unsigned int buffer_size
	)
{
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

	while (rx_done == 0);

	rx_array = 0;
	rx_length = 0;
	rx_index = 0;
}

int
echo_uart (
	void
	)
{
	uart_init();
    while (1);
}
