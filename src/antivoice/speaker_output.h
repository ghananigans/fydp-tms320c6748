#ifndef SPEAKER_OUTPUT_H
#define SPEAKER_OUTPUT_H

#include "common.h"


/* Scaling required as per:
		http://stackoverflow.com/questions/15087668/how-to-convert-pcm-samples-in-byte-array-as-floating-point-numbers-in-the-range
		Input f is expected to be between -1 and 1.
	*/
speakerout_t convert_float_to_speakerout(float f)
{
	/* We add 1 since it's unsigned number we are converting to and the number can be purely positive*/
	f = (f + 1);
	/* We now have f between 0 and 2.  Scale up to be between 0 and MAX_VAL. */
	f = f * (SPEAKEROUT_MAX_VAL / 2);
	if( f > SPEAKEROUT_MAX_VAL )
	{
		ERROR_PRINT("convert_float_to_speakerout resulted in value > SPEAKEROUT_MAX_VAL of %f\n", f);
		f = SPEAKEROUT_MAX_VAL;
	}

	return (speakerout_t) f;
}

void send_speaker_output(float * data)
{
	speakerout_t out_sample = convert_float_to_speakerout(data[SPEAKEROUT_CHOSEN_SAMPLE * 2]);

	/* STUB: Send out_sample to DAC */
}


#endif /* SPEAKER_OUTPUT_H */

