/*
 * timer_wrapper.h
 *
 *  Created on: Jan 20, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef TIMER_WRAPPER_H_
#define TIMER_WRAPPER_H_

enum TIMER_ERROR_CODES {
    TIMER_OK,
    TIMER_NOT_INITIALIZED,
    TIMER_ALREADY_INITIALIZED,
    TIMER_FAILED_TO_INITIALIZE
};

int
timer_init (
    void (* func)(void),
    unsigned int milliseconds
    );

int
timer_destroy (
    void
    );

#endif /* TIMER_WRAPPER_H_ */
