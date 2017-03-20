/*
 * mcasp_wrapper.h
 *
 *  Created on: March 5, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef MCASP_WRAPPER_H_
#define MCASP_WRAPPER_H_

#include <stdint.h>

enum MCASP_ERROR_CODES {
    MCASP_OK,
    MCASP_ALREADY_INITIALIZED,
    MCASP_INTERNAL_FAILURE_UNRECOGNIZED_AUDIO_BUF_SIZE_VALUE
};

int
mcasp_init (
    void
    );

//
// Get the most recent mcasp data
// ptr should be a 2 element array where
// the first element is one channel, and the
// second element is the other channel.
//
int
mcasp_latest_rx_data (
    uint32_t * ptr
    );

#endif /* MCASP_WRAPPER_H_ */
