#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "common.h"
#include "dsplib.h"

/* Prepend newest data and discards oldest data from the end of the array */
void add_new_mic_data(float mic_data_float[AUDIO_CHANNEL_COUNT])
{
	unsigned int ch;
	for(ch = 0; ch < AUDIO_CHANNEL_COUNT; ch++)
	{
		/* We shift by 2 since both real and imag. */
		memcpy(audio_data[ch].x + 2,audio_data[ch].x, (M - 2) * sizeof(float));
		/* Place new data at front of array */
		audio_data[ch].x[0] = mic_data_float[ch];
	}
}

/*
	Applies bandpass filter to input signal S.
		S is in the fourier domain and is of size M (when considering both real and imaginary parts.
		S is partitioned in the frequency domain appropriately.
	Anything outside of this pass-band is zeroed out for both positive and negative frequencies.
	
	@pass_start and @pass_stop are non-negative frequencies in Hertz.
*/
void fourier_bpf(float * S, float pass_start, float pass_stop)
{
	/* TODO: implement when we have determined how sp_FFT function partitions the signal S, so that we know where particular 
	frequencies are located in this fourier domain signal. Consider both negative and positive frequencies, and imaginary parts.
	Should use windowing such as a raised cosine, otherwise will get ringing in the time domain.
	*/
}

void fourier_lpf(float *S, float cutoff)
{
	fourier_bpf(S, 0, cutoff);
}

void apply_hpf(float *S, float cutoff)
{
	fourier_bpf(S,cutoff,FFT_FREQ_MAX);
}

/* Populates freq_all with the center frequencies of the FFT frequency domain vectors. */
void generate_fft_center_frequencies()
{
	unsigned int i;
	float f;
	float finc = (float) FS / (float) FFT_SIZE;
	/* This iterates and include f=FS/2. */
	i = 0;
	for(f = 0; f <= (float) (FS / 2) + 0.1f ; f += finc)
	{
		freq[i] = f;
		i++;
	}

	f = -1.0f * (float) (FS / 2);
	while(i < FFT_SIZE)
	{
		f += finc; /*we want to skip over -fs/2 */
		freq[i] = f;
		i++;
	}
}


/*
	NOTE: phase_sys[] array must have been populated first,
	as well as generate_fft_center_frequencies() must have been called.
inputs:
%	phase_sys is passed in as seconds
    freq is a vector representing each frequency bin in hz
		0 | positive | (fs/2 freq) | negative reversed
outputs:
    phase_adj is in radians, and is a ROW vector corresponding freq input
*/
void generate_antiphase_vector()
{
	unsigned int i;
	float period, periods_elapsed, periods_floor_diff, shift_correction_factor, out_phase_correction;
	const float PI = (float) 3.14159265;

	for(i = 0; i < FFT_SIZE; i++)
	{
		if(phase_sys[i] > 0)
		{
			period = 1 / fabsf(freq[i]);
            periods_elapsed = phase_sys[i] / period; /*this is the number of periods source is ahead of antisignal */
            periods_floor_diff = (float) (periods_elapsed - floor(periods_elapsed)); /*number between 0 and 1 */

            /*since source signal is ahead of antisignal by phase_sys[i] seconds, 
                %0.5 PLUS as opposed to minus is used
                %Equivalent to 1 - mod(0.5 - periods_floor_diff,1)
			*/
            shift_correction_factor = fmodf(0.5f + periods_floor_diff, 1); /*keep positive number between 0 and 1 */
            out_phase_correction = shift_correction_factor*2*PI;
            phase_adj[i] = out_phase_correction;
		}
		/*else
		{
			phase_adj[i] = PI;
		}
		*/
	}
}

void signal_processing_init()
{
	generate_fft_center_frequencies();
	generate_antiphase_vector();
	NORMAL_PRINT("Completed signal_processing_init.\n");
}

/*
	X is the signal to be applied in-place the phase shift specified at phase_adj[i]
		for elements X[2*i] and X[2*i + 1] (real and imag)
*/

void apply_phase_adj(float * X)
{
	unsigned int i;
	float angle, a, x_abs;

	for(i = 0; i < FFT_SIZE; i++)
	{	
		if(X[2*i] != 0.0f)
		{
			angle = (float) atan(X[2*i+1] / X[2*i]);
			a = angle + phase_adj[i];
			x_abs = (float) sqrt( pow(X[2*i], 2 ) + pow( X[2*i+1], 2 ) );
	
			X[2*i] = x_abs * (float) cos(a);
			X[2*i+1] = x_abs * (float) sin(a);
		}
		
	}
}

#endif /* SIGNAL_PROCESSING_H */



