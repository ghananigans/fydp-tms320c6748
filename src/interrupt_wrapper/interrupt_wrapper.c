/*
 * interrupt_wrapper.c
 *
 *  Created on: Jan 20, 2017
 *      Author: Ghanan Gowripalan
 */

#include "interrupt_wrapper.h"
#include "interrupt.h"
#include "../util.h"
#include <stdbool.h>

static bool init_done = 0;

/*
** \brief   This function invokes necessary functions to configure the ARM
**          processor and ARM Interrupt Controller(AINTC) to receive and
**          handle interrupts.
*/
int
init_interrupt (
	void
	)
{
	if (init_done)
	{
		DEBUG_PRINT("Interrupts already initialized!\n");
		return INTERRUPT_ALREADY_INITIALIZED;
	}

	DEBUG_PRINT("Initializing system interrupts...\n");

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

    init_done = 1;

    DEBUG_PRINT("Done Initializing system interrupts!\n");

    return INTERRUPT_OK;
}

bool is_interrupt_init_done (
	void
	)
{
	return init_done;
}
