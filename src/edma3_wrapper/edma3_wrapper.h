/*
 * edma3_wrapper.h
 *
 *  Created on: Feb 8, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef EDMA3_WRAPPER_H_
#define EDMA3_WRAPPER_H_

#include <stdbool.h>

enum EDMA3_ERROR_CODES
{
    EDMA3_OK,
    EDMA3_ALREADY_INITIALIZED
};

int
edma3_init (
    void
    );

int
edma3_set_callback (
    unsigned int tcc_num,
    void (*func) (
        unsigned int,
        unsigned int
    )
    );

int
edma3_request_channel (
    unsigned int ch_num,
    unsigned int tcc_num
    );

int
edma3_param_set (
    unsigned int tcc_num,
    unsigned int channel_num,
    char volatile * src_buffer,
    char volatile * dest_buffer,
    unsigned short num_bytes_in_array,
    unsigned short num_arrays_in_frame,
    unsigned short num_frames,
    bool increment_src_buffer_ptr,
    bool increment_dest_buffer_ptr
    );

#endif /* EDMA3_WRAPPER_H_ */
