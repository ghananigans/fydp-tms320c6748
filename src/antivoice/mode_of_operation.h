#ifndef MODE_OF_OPERATION_H
#define MODE_OF_OPERATION_H

#include "common.h"

/* Set to 1 if using ISR for mode of operation, or 0 if using polling */
#define USING_ISR 0


#if USING_ISR

/*TODO: modify these in an ISR if using ISR*/
volatile static bool calibration_enabled = 0;
volatile static bool algorithm_enabled = 1;


bool is_algorithm_enabled()
{
	return calibration_enabled;
}


bool is_calibration_enabled()
{
	return calibration_enabled;
}




#else /* We are using polling */

bool is_algorithm_enabled()
{
	bool algorithm_enabled = true;
	/* TODO: perform polling for status */
	return algorithm_enabled;
}


bool is_calibration_enabled()
{
	bool calibration_enabled = true;
	/* TODO: perform polling for status */
	return calibration_enabled;
}

#endif /* USING_ISR */




#endif /*MODE_OF_OPERATION_H*/
