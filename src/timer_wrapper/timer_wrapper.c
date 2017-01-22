/*
 * timer_wrapper.c
 *
 *  Created on: Jan 20, 2017
 *      Author: Ghanan Gowripalan
 */

#include "timer_wrapper.h"
#include <stdbool.h>
#include "../interrupt_wrapper/interrupt_wrapper.h"
#include "../util.h"
#include "systick.h"

/****************************************************************************/
/*                      GLOBAL VARIABLES                                    */
/****************************************************************************/
static bool init_done = 0;

/******************************************************************************/
/*                          FUNCTION DEFINITIONS                              */
/******************************************************************************/
int
timer_init (
	void (* func)(void),
	unsigned int milliseconds
	)
{
	int ret_val;

	if (init_done)
	{
		DEBUG_PRINT("Timer is already initialized!\n");
		return TIMER_ALREADY_INITIALIZED;
	}

	/*
	 * Make sure global interrupt have been initialized.
	 */
	if (is_interrupt_init_done())
	{
		DEBUG_PRINT("Interrupts are already initialized!\n");
	}
	else
	{
		DEBUG_PRINT("Initializing interrupts...\n");

		ret_val = init_interrupt();
		if (ret_val != INTERRUPT_OK)
		{
			if (ret_val == INTERRUPT_ALREADY_INITIALIZED)
			{
				DEBUG_PRINT("Interrupts already initialized", ret_val);
			}
			else
			{
				DEBUG_PRINT("Interrupts failed to initialize with error code %d", ret_val);
				return TIMER_FAILED_TO_INITIALIZE;
			}
		}
	}

	DEBUG_PRINT("Initializing Timer...\n");

	TimerTickConfigure(func);
	TimerTickPeriodSet(milliseconds);
	TimerTickEnable();

	init_done = 1;

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
