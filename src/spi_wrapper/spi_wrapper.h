/*
 * spi_wrapper.h
 *
 *  Created on: Jan 22, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef SPI_WRAPPER_H_
#define SPI_WRAPPER_H_

enum SPI_ERROR_CODES
{
    SPI_OK,
    SPI_ALREADY_INITIALIZED,
    SPI_NOT_INTIAILZED,
    SPI_INVALID_CS,
    SPI_INTERNAL_FAILURE_EDMA3_PARAM_SET_FAILED
};

int
spi_init (
    void
    );

void
spi_wait_until_not_busy (
    void
    );

int
spi_send_and_receive_non_blocking (
    char volatile * data,
    unsigned int len,
    unsigned int cs
    );

int
spi_send_and_receive_blocking (
    char volatile * data,
    unsigned int len,
    unsigned int cs
    );

#endif /* SPI_WRAPPER_H_ */
