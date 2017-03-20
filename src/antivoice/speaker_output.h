#ifndef SPEAKER_OUTPUT_H
#define SPEAKER_OUTPUT_H

#include "common.h"


/* Scaling required as per:
		http://stackoverflow.com/questions/15087668/how-to-convert-pcm-samples-in-byte-array-as-floating-point-numbers-in-the-range
		Input f is expected to be between -1 and 1.
	*/
speakerout_t convert_float_to_speakerout(float f)
{
	/* We add 1 since it's unsigned number we are converting to and the number should become purely non-negative*/
	f = (f + 1);
	/* We now have f between 0 and 2.  Scale up to be between 0 and SPEAKEROUT_MAX_VAL. */
	f = f * (SPEAKEROUT_MAX_VAL / 2);
	if( f > SPEAKEROUT_MAX_VAL )
	{
		ERROR_PRINT("convert_float_to_speakerout resulted in value > SPEAKEROUT_MAX_VAL of %f\n", f);
		f = SPEAKEROUT_MAX_VAL;
	}

	return (speakerout_t) f;
}

void send_speaker_output(audio_data_t audio_data[AUDIO_CHANNEL_COUNT])
{
	int ret_val;
	unsigned int ch;
	speakerout_t out_samples[AUDIO_CHANNEL_COUNT];

	/*TODO: modify so that both channels are outputted to the speaker simultaneously!!! */

	for(ch = 0; ch < AUDIO_CHANNEL_COUNT; ch++)
	{
		out_samples[ch] = convert_float_to_speakerout(audio_data[ch].y[SPEAKEROUT_CHOSEN_SAMPLE * 2]);

		/* NOTE: Vref/2 DC offset for int16_t to uint16_t has already been considered when mic data was parsed and no need to add 32676 here*/
		if(ch == LEFT)
		{
			ret_val = dac_update(CHANNEL_LC, out_samples[ch], 1);
		}
		else
		{
			ret_val = dac_update(CHANNEL_RC, out_samples[ch], 1);
		}
		
		ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);
	}

}


#endif /* SPEAKER_OUTPUT_H */

