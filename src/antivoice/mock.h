#ifndef MOCK_DATA_H
#define MOCK_DATA_H

/* NOTE: this file is for simulation use only.  Not used on the board. */


#include "common.h"




/* Returns the raw mic data type (usually unsigned).
	The rawmic value will be unsigned within the range of 0 and 0xFFFF if it were 16-bits.
	Future: can use this function to read a .wav file if desired or a .txt file instead.
*/
rawmic_t mock_get_mic_input(audio_channel_t ch)
{
	double val;
	static unsigned int i = 0;

	/* Create sine wave with peak to peak amplitude of 1.0. and have a vertical offset of 1 so that signal is purely positive*/
	if(ch)
	{
		val = 1.0 * sin (i / (double) FFT_SIZE) + 1;
	}
	else
	{
		val = 1.0 * cos (i / (double) FFT_SIZE) + 1;
	}
	i++;

	/* Val is in the range of 0 and 2 */
	
	return (rawmic_t)(val * (RAWMIC_MAX_VAL/ 2));
}



#endif /* MOCK_DATA_H */
