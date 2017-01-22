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
    SPI_NOT_INTIAILZED
};

int
spi_init (
    void
    );

int
spi_send (
    unsigned char * data,
    unsigned int len
    );


#endif /* SPI_WRAPPER_H_ */
