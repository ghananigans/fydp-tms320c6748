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
//#define SPI_EDMA

//
// Comment this out if you want the DAC to use it's own calculated voltage
// to use as the internal reference voltage. (It should by default be 2.5 V).
//
#define DAC_DO_NOT_USE_INTERNAL_REFERENCE

// This does the loopback test (Mic input connects to codec,
// Codec goes to dsp, Dsp outputs sound by going through the CODEC.
// If this macro is enabled, the applications for the below two macros
// (SINGLE_TONE_SIGNAL_THROUGH_DAC and MIC_TO_DAC) won't have any effect since
// the MCASP_LOOPBACK_TEST involves an infinite loop.
// To do any of the DAC related stuff, make sure the MCASP_LOOPBACK_TEST is undefined
// (Comment it out to make it undefined).
//#define MCASP_LOOPBACK_TEST

/*
 * Uncomment the macro definition below to disable debug output.
 */
//#define DEBUG_PRINT_ENABLED

#endif /* CONFIG_H_ */
