/*
 * timer_wrapper.c
 *
 *  Created on: Jan 20, 2017
 *      Author: Ghanan Gowripalan
 */

#include "timer_wrapper.h"

#include <stdbool.h>
#include "../interrupt_wrapper/interrupt_wrapper.h"
#include "../uart_wrapper/uart_wrapper.h"
#include "../util.h"
#include <stdio.h>
#include "systick.h"

/****************************************************************************/
/*                      GLOBAL VARIABLES                                    */
/****************************************************************************/
static bool init_done = 0;

/******************************************************************************
**                          FUNCTION DEFINITIONS
*******************************************************************************/
int
timer_init (
	void (* func)(void),
	unsigned int milliseconds
	)
{
	if (init_done)
	{
		DEBUG_PRINT("Timer is already initialized!\n");
		return TIMER_ALREADY_INITIALIZED;
	}

	DEBUG_PRINT("Initializing Timer...\n");

	if (is_interrupt_init_done() == 0)
	{
		DEBUG_PRINT("Interrupts have not been initialized!\n");
		return TIMER_INTERRUPTS_NOT_INITIALIZED;
	}

	TimerTickConfigure(func);
	TimerTickPeriodSet(milliseconds);
	TimerTickEnable();

	init_done = 0;

	DEBUG_PRINT("Done Initializing timer!\n");

	return TIMER_OK;
}

int
timer_destroy (
	void
	)
{
	/* Disable the timer. No more timer interrupts */
	TimerTickDisable();
	init_done = 0;

	return TIMER_OK;
}


/***************************** End Of File ***********************************/
