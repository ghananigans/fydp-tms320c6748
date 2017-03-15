#ifndef MIC_INPUT_H
#define MIC_INPUT_H

#include "common.h"
#include "mock.h"


#define RAWMIC_UPPER_BOUND (0.8f) /* This is the max value a mic reading can take in float representation.  Note: this reading may be amplified, so it is worthwhile keeping it between 0.7 and 1.0 at most */
#define RAWMIC_SCALE_DOWN (RAWMIC_MAX_VAL / RAWMIC_UPPER_BOUND) /*Gives resulting float range between 0 and RAWMIC_UPPER_BOUND */


float convert_rawmic_to_float(rawmic_t data)
{
	/* scaling required for it to be valid FFT input
		http://stackoverflow.com/questions/15087668/how-to-convert-pcm-samples-in-byte-array-as-floating-point-numbers-in-the-range
		This mentions after the cast, divide by int16_t MAX , and then when about to create speaker output should multiply back.
		Also verified matlab audioread function uses similar strategy, our MATLAB simulation used floats in the range of -1.0f to 1.0f
	*/
	float f;

	/* yields f in the range of -RAWMIC_UPPER_BOUND and +RAWMIC_UPPER_BOUND and therefore the signal is zero-centered*/
	f = ((float) data) /((float) RAWMIC_SCALE_DOWN);

	return f;
}


void get_mic_data_float(float mic_data_float[AUDIO_CHANNEL_COUNT])
{
	unsigned int i;
	int ret_val;
	uint32_t bufferdata[2];
	rawmic_t rawdata[2];

	/* Gets the reading from the microphone */
	ret_val = mcasp_latest_rx_data((uint32_t *) &bufferdata);
	ASSERT(ret_val == MCASP_OK, "MCASP getting latest rx data failed! (%d)\n", ret_val);


	for(i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		rawdata[i] = (int16_t) (bufferdata[i] & 0xFFFF);
		mic_data_float[i] = convert_rawmic_to_float(rawdata[i]);
	}
}




#endif /* MIC_INPUT_H */
