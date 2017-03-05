/*
 * mcasp_wrapper.h
 *
 *  Created on: March 5, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef MCASP_WRAPPER_H_
#define MCASP_WRAPPER_H_

enum MCASP_ERROR_CODES {
    MCASP_OK,
    MCASP_ALREADY_INITIALIZED
};

int
mcasp_init (
    void
    );

void 
mcasp_loopback_test (
	void
	);

#endif /* MCASP_WRAPPER_H_ */
