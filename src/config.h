/*
 * config.h
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef CONFIG_H_
#define CONFIG_H_


//
// Vinay, Look here.
//

/*
 * Uncomment the macro definition below to make SPI use interrupts
 * And be blocking, instead of using the EDMA and be asynchronous.
 * Comment this out if it is too slow.
 */
#define SPI_EDMA

//
// Comment this out if you want the DAC to use it's own calculated voltage
// to use as the internal reference voltage. (It should by default be 2.5 V).
//
#define DAC_DO_NOT_USE_INTERNAL_REFERENCE

/*
 * Uncomment the macro definition below to disable debug output.
 */
//#define DEBUG_PRINT_ENABLED

#endif /* CONFIG_H_ */
