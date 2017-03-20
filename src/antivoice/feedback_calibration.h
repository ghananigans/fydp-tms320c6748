#ifndef FEEDBACK_CALIBRATION_H
#define FEEDBACK_CALIBRATION_H

#include "common.h"
#include "speaker_output.h"
#include "mic_input.h"
#include "signal_processing.h"

typedef struct feedback_calib {
	//both speaker and mic samples are purely real (no imaginary parts)
	float sent;
	float received;
} feedback_calib_t;

feedback_calib_t feedback_calib_data[FEEDBACK_CALIB_SAMPLES];

void timer_wait_num_samples(unsigned int num_samples)
{
	unsigned int i;
	for(i = 0; i < num_samples; i++)
	{
		ASSERT(timer_flag == 0, "TIMER IS TOO FAST!\n");
		while (timer_flag == 0);
		timer_flag = 0;
	}
}

void send_feedback_calibration_to_pc(float a, float f)
{
	unsigned int i;
	NORMAL_PRINT("amp_index=%u\n",(unsigned int)(a*100));
	NORMAL_PRINT("freq_index=%u\n",get_fft_bin_by_freq_pos(fabsf(f)));
	NORMAL_PRINT("sent\n",a);
	for(i=0; i < FEEDBACK_CALIB_SAMPLES; i++)
	{
		if(i == FEEDBACK_CALIB_SAMPLES - 1)
		{
			NORMAL_PRINT("%f",feedback_calib_data[i].sent);
		}
		else
		{
			NORMAL_PRINT("%f,",feedback_calib_data[i].sent);
		}
	}

	NORMAL_PRINT("received\n",a);
	for(i=0; i < FEEDBACK_CALIB_SAMPLES; i++)
	{
		if(i == FEEDBACK_CALIB_SAMPLES - 1)
		{
			NORMAL_PRINT("%f",feedback_calib_data[i].received);
		}
		else
		{
			NORMAL_PRINT("%f,",feedback_calib_data[i].received);
		}
	}
}

float generate_feedback_calibration_float(unsigned int sample_number, float a, float f)
{
	double arg;
	float out_float;
#ifndef PI
	const double PI = 3.141592654;
#endif

	arg = 2* PI * (double) f * ((double) sample_number / (double) FS); 
	out_float = (float)((double)a * (float) cos (arg)); //use cosine so that first sample in feedback is 1 and easily recognizable

	feedback_calib_data[sample_number].sent = out_float;
	return out_float;
}

void collect_mic_readings(unsigned int sample_number)
{
	float mic_data_float[AUDIO_CHANNEL_COUNT]; /* gets populated with newest mic samples as float */
	
	/* Gets the reading from the microphone */
	get_mic_data_float(mic_data_float);

	feedback_calib_data[sample_number].received = mic_data_float[LEFT];
}


/* f is the frequency to generate tone at and a is the amplitude */
void collect_feedback_calibration(float a, float f)
{
	int ret_val;
	float out_float;
	speakerout_t out_speaker;
	unsigned int sample_number;

	//NORMAL_PRINT("Collect Feedback Calibration start: a = %f f = %f\n",a,f);
	
	for(sample_number = 0; sample_number < (unsigned int) FEEDBACK_CALIB_SAMPLES; sample_number++)
	{
		timer_wait_num_samples(1);

		//collect microphone reading into calibration data structure
		collect_mic_readings(sample_number);

		//send to speaker (and records the generated/sent float in the calibration data structure)
		out_float = generate_feedback_calibration_float(sample_number, a,f);
		out_speaker = convert_float_to_speakerout(out_float);
		ret_val = dac_update(CHANNEL_LC, out_speaker, 1);
		ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);
		
	}
	//NORMAL_PRINT("Collect Feedback Calibration stop: a = %f f = %f\n",a,f);
}




void run_feedback_calibration()
{
	/* Implement a per frequency, amplitude sweep */
	unsigned int i;
	float f, a;
	unsigned int sleep_count = 0;

	for(i = 0; i < FFT_SIZE; i++)
	{
		f = freq[i];
		//we only need to do this for the positive frequencies
		//and the frequencies our speakers respond well to or we care about
		if(f > MIN_PRACTICAL_FREQ && f < MAX_PRACTICAL_FREQ)
		{
			//sweep over different amplitudes
			for(a = 0.1f; a <= 1.0f; a+=0.01f)
			{
				collect_feedback_calibration(a,f);
				//now, we should send this data to the PC so that it can be stored for analysis
				send_feedback_calibration_to_pc(a,f);

				//// Wait 500 samples to be clean for next calibration
				timer_wait_num_samples(500);
			}
		}
	}
}


#endif /* FEEDBACK_CALIBRATION_H */

