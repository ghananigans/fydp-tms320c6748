#ifndef FFT_WRAP_H
#define FFT_WRAP_H

#include "common.h"
#include "dsplib.h"

/*
	This file contains wrapper functions for FFT and iFFT for easily calling DSPLIB functions.
*/

#ifdef _TMS320C6X
#pragma DATA_ALIGN(brev, 8);
#pragma DATA_ALIGN(w, 8);
#pragma DATA_ALIGN(h_padded, 8);
#pragma DATA_ALIGN(x_tmp_buffer, 8);
#pragma DATA_ALIGN(y_tmp_buffer, 8);
#endif /* _TMS320C6X */

/* Contains the twiddle factors used by the FFT radix algorithm. */
/* It's 2 * PAD, here because pad once for real, and once for imag. values 
	ptr_w is array of twiddle factors used by algorithm
		NOTE: this memory address should never be cleared after initialized
		This is done so that twiddle factors are not computed over and over again
*/
float w[M + 2 * PAD];
/* ptr_w will skip beyond the array padding and give useful aspect*/
float *const ptr_w = w + PAD;

/* Hilbert transform vector */
float h_padded[M + 2 * PAD];
float * h = h_padded + PAD;

/* brev is used by this specific butterfly radix algorithm for value lookup */
unsigned char brev[64] = {
    0x0, 0x20, 0x10, 0x30, 0x8, 0x28, 0x18, 0x38,
    0x4, 0x24, 0x14, 0x34, 0xc, 0x2c, 0x1c, 0x3c,
    0x2, 0x22, 0x12, 0x32, 0xa, 0x2a, 0x1a, 0x3a,
    0x6, 0x26, 0x16, 0x36, 0xe, 0x2e, 0x1e, 0x3e,
    0x1, 0x21, 0x11, 0x31, 0x9, 0x29, 0x19, 0x39,
    0x5, 0x25, 0x15, 0x35, 0xd, 0x2d, 0x1d, 0x3d,
    0x3, 0x23, 0x13, 0x33, 0xb, 0x2b, 0x1b, 0x3b,
    0x7, 0x27, 0x17, 0x37, 0xf, 0x2f, 0x1f, 0x3f
};

int rad = 2;


float x_tmp_buffer[M + 2 * PAD];
float * x_tmp_buffer_ptr = x_tmp_buffer + PAD;

float y_tmp_buffer[M + 2 * PAD];
float * y_tmp_buffer_ptr = y_tmp_buffer + PAD;



/* Function for generating Specialized sequence of twiddle factors */
void tw_gen (float *w, int n)
{
    int i, j, k;
    double x_t, y_t, theta1, theta2, theta3;
#ifndef PI
    const double PI = 3.141592654;
#endif
    for (j = 1, k = 0; j <= n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            theta1 = 2 * PI * i / n;
            x_t = cos (theta1);
            y_t = sin (theta1);
            w[k] = (float) x_t;
            w[k + 1] = (float) y_t;

            theta2 = 4 * PI * i / n;
            x_t = cos (theta2);
            y_t = sin (theta2);
            w[k + 2] = (float) x_t;
            w[k + 3] = (float) y_t;

            theta3 = 6 * PI * i / n;
            x_t = cos (theta3);
            y_t = sin (theta3);
            w[k + 4] = (float) x_t;
            w[k + 5] = (float) y_t;
            k += 6;
        }
    }
}

void generate_twiddle(int N, float * ptr_w)
{
	/* ---------------------------------------------------------------- */
    /* Generate twiddle factors.                                        */
    /* ---------------------------------------------------------------- */
	int i, j;
    j = 0;
    for (i = 0; i <= 31; i++)
        if ((N & (1 << i)) == 0)
            j++;
        else
            break;

    if (j % 2 == 0)
        rad = 4;
    else
        rad = 2;

    tw_gen (ptr_w, N);
}

void fft_wrap_init()
{
	unsigned int i;
	tw_gen(ptr_w, FFT_SIZE);

	/* Clear data structures and initialize usable (data aligned) array ptrs. */

	for(i = 0; i < AUDIO_CHANNEL_COUNT; i++)
	{
		memset(audio_data[i].x, 0, sizeof(audio_data[i].x));
		memset(audio_data[i].X, 0, sizeof(audio_data[i].X));
		memset(audio_data[i].Y, 0, sizeof(audio_data[i].Y));
		memset(audio_data[i].y, 0, sizeof(audio_data[i].y));
	}

	/* Populate hilbert transform vector */
	memset(h,0,sizeof(float) * M);
	h[0] = 1;
	h[MAXN] = 1;

	for(i = 2; i < MAXN; i+=2)
	{
		h[i] = 2;
	}
	NORMAL_PRINT("Completed FFT init.\n");
}

/*
	Performs an optimized version of the hilbert transform in-place such that:
		@X: this signal is in the fourier domain and will be complexified (applied hilbert)
	and @X is kept in the fourier domain and no ifft is taken.
	Typically fft and ifft are the first and last steps, and we skip over those as we want to work
	with the signal in the frequency domain as opposed to time domain.

	Based on Ghanan's implementation:
		https://github.com/ghanan94/fydp-tests-and-simulations/blob/master/fft_phase_shift_test/src/phase_shift.c

	Essentially this function calculates the element-wise product of X and h (transform vector).
*/
void fft_hilbert_transform(float * X)
{
	float re;
	float im;
	unsigned int i;

	for(i = 0; i < M; i+=2)
	{
		re = X[i];
		im = X[i + 1];

		X[i] = re * h[i] - im * h[i + 1];
		X[i + 1] = re * h[i + 1] + im * h[i];
	}
}


/*
	fft_wrap()
	fft_wrap_init() must be called before calling this function for the first time.
	Args:
		ptr_x is input array
		ptr_y is output array

*/
void fft_wrap(float *ptr_x, float *ptr_y)
{
	memset(x_tmp_buffer_ptr,0,sizeof(float) * M);

	/* The DSPF_sp_fftSPxSP modifies the input x!!! therefore copy to separate buffer instead for computation */
	/* TODO: need to test if M is correct number of data elements given that PAD many could still be left */
	memcpy(x_tmp_buffer_ptr,ptr_x,M* sizeof(float));

	DSPF_sp_fftSPxSP (FFT_SIZE, &x_tmp_buffer_ptr[0], &ptr_w[0], y_tmp_buffer_ptr, brev, rad, 0, FFT_SIZE);

	/* Perform hilbert transform on signal so that phase adjustments can be applied */
	fft_hilbert_transform(y_tmp_buffer_ptr);

	/* Copy the output to where it needs to be */
	memcpy(ptr_y,y_tmp_buffer_ptr,M* sizeof(float));
}


/*
	Args:
		ptr_x contains frequency domain information.
		ptr_y is where the inverse will be placed in time domain.
*/

void ifft_wrap(float *ptr_x, float *ptr_y)
{
	/* The DSPF_sp_ifftSPxSP modifies the input x!!! therefore copy to separate buffer instead for computation
		As well, the temporary buffer is data-aligned as the dsplib requires.
	*/
	memcpy(ptr_x,x_tmp_buffer_ptr,M* sizeof(float));

	/* ifft output is placed in y_tmp_buffer_ptr */
	DSPF_sp_ifftSPxSP(FFT_SIZE, &x_tmp_buffer_ptr[0], &ptr_w[0], y_tmp_buffer_ptr, brev, rad, 0, FFT_SIZE);

	/* Copy the output to where it needs to be */
	memcpy(ptr_y,y_tmp_buffer_ptr,M* sizeof(float));
}

#endif /* FFT_WRAP_H */

