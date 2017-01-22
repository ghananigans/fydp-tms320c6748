/*
 * interrupt_wrapper.h
 *
 *  Created on: Jan 20, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef INTERRUPT_WRAPPER_H_
#define INTERRUPT_WRAPPER_H_

#include <stdbool.h>

enum INTERRUPT_ERROR_CODES {
    INTERRUPT_OK,
    INTERRUPT_NOT_INITIALIZED,
    INTERRUPT_ALREADY_INITIALIZED
};

int
init_interrupt (
    void
    );

bool
is_interrupt_init_done (
    void
    );

#endif /* INTERRUPT_WRAPPER_H_ */
