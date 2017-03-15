/*
 * antivoice.h
 *
 *  Created on: Mar 14, 2017
 *      Author: rishi
 */

#ifndef SRC_ANTIVOICE_ANTIVOICE_H_
#define SRC_ANTIVOICE_ANTIVOICE_H_


#include "common.h"
#include "mode_of_operation.h"
#include "mic_input.h"
#include "speaker_output.h"
#include "fft_wrap.h"
#include "signal_processing.h"
#include "matlab_helper.h"

#define MATLAB_DEBUG_ENABLED 1

unsigned int iteration_count = 0;
#define NUM_SIMULATION_CYCLES 400


void run_calibration()
{
    unsigned int i;

    /*Populate calibration data structures.
        TODO: perhaps replace this function with a separate header file altogether with an assignment of calibration results to the calibration variables.
            create calibration.h
    */
    for(i = 0; i < FFT_SIZE; i++)
    {
        phase_sys[i] = 0.0001f;
        mag_sys[i] = 1.0f;
    }

    NORMAL_PRINT("run_calibration() complete.\n");
}

void run_main_algorithm()
{
    float mic_data_float[AUDIO_CHANNEL_COUNT]; /* gets populated with newest mic samples as float */
    unsigned int ch;

    /*Get mic input */
    /*  returns an array of floats, 1 float per mic
        this is assuming that reading McBSP buffered input takes minimal clock cycles, otherwise we may need to process mic one by one
    */
    get_mic_data_float(mic_data_float);

    /*NORMAL_PRINT("run_main_algorithm audio_data[01].x[0] = %f, %f \n", audio_data[0].x[0], audio_data[1].x[0]);*/
    /* append to working signal to operate on (signal is allocated in common.h) */
    add_new_mic_data(mic_data_float);
    /*NORMAL_PRINT("run_main_algorithm mic_data_float = %f, %f \n", mic_data_float[0], mic_data_float[1]);*/


    for(ch = 0; ch < AUDIO_CHANNEL_COUNT; ch++)
    {
        /* NOTE: hilbert transform is performed by fft_wrap to make analytical signal X*/
        fft_wrap(audio_data[ch].x,audio_data[ch].X);

        /* copy X to Y, we leave X untouched for debugging purposes */
        memcpy(audio_data[ch].Y, audio_data[ch].X, M*sizeof(float));

        /* Apply corresponding mag & phase adjustments to Y */
        apply_mag_phase_adj(audio_data[ch].Y);

        /*Take ifft of Y to get output y in time domain */
        ifft_wrap(audio_data[ch].Y,audio_data[ch].y);
    }

    /*Write to speakers*/
	send_speaker_output(audio_data);


#if MATLAB_DEBUG_ENABLED
    if(iteration_count == 300)
    {
        NORMAL_PRINT("x signal:\n");
        matlab_float_print_complex(audio_data[0].x, M);
        NORMAL_PRINT("\n\n\n\n X signal: \n");
        matlab_float_print_complex(audio_data[0].X, M);

        NORMAL_PRINT("\n\n\n\n phase_adj signal: \n");
        matlab_float_print_array(phase_adj, FFT_SIZE);

        NORMAL_PRINT("\n\n\n\n Y signal: \n");
        matlab_float_print_complex(audio_data[0].Y, M);
        NORMAL_PRINT("\n\n\n\n y signal: \n");
        matlab_float_print_complex(audio_data[0].y, M);
    }
#endif /* MATLAB_DEBUG_ENABLED */


}




#endif /* SRC_ANTIVOICE_ANTIVOICE_H_ */
