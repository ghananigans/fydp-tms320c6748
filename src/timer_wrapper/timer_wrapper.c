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
#include "timer.h"
#include "soc_c6748.h"

#define TICKS_PER_MICROSECOND (24)

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
    unsigned int microseconds
    )
{
    if (init_done)
    {
        DEBUG_PRINT("Timer is already initialized!\n");
        return TIMER_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing Timer...\n");

    TimerTickConfigure(func);
    //TimerTickPeriodSet(microseconds * 1000);
    TimerPeriodSet(SOC_TMR_0_REGS, TMR_TIMER34, (microseconds * TICKS_PER_MICROSECOND));
    TimerReloadSet(SOC_TMR_0_REGS, TMR_TIMER34, (microseconds * TICKS_PER_MICROSECOND));

    init_done = 1;

    DEBUG_PRINT("Done Initializing timer!\n");

    return TIMER_OK;
}

int
timer_start (
    void
    )
{
    TimerTickEnable();

    return TIMER_OK;
}

int
timer_stop (
    void
    )
{
    TimerTickDisable();

    return TIMER_OK;
}
