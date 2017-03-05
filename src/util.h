/*
 * util.h
 *
 *  Created on: Jan 19, 2017
 *      Author: Ghanan Gowripalan
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "config.h"
#include "uart_wrapper/uart_wrapper.h"
#include <stdio.h>

/*
 * Use this macro for regular output.
 */
#define NORMAL_PRINT(format, ...) printf(               \
            format, ##__VA_ARGS__)

/*
 * Use this macro for error output.
 */
#define ERROR_PRINT(format, ...) printf(                \
            format, ##__VA_ARGS__)

/*
 * If doing prints for debugging purposes, use the
 * below macro so all debug prints can be easily disabled
 * with minimal code change and effort.
 */
#ifdef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT(format, ...)                            \
    {                                                       \
        NORMAL_PRINT("[%s:%d] ", __FUNCTION__, __LINE__);   \
        NORMAL_PRINT(format, ##__VA_ARGS__);                \
    }
#else
#define DEBUG_PRINT(format, ...) ((void) 0)
#endif // #ifdef DEBUG_PRINT_ENABLED

#define ASSERT(result, format, ...)                         \
    {                                                       \
        if ((result) == 0)                                  \
        {                                                   \
            ERROR_PRINT("[%s:%d] ** Assertion Error: ",     \
                __FUNCTION__, __LINE__);                    \
            ERROR_PRINT(format, ##__VA_ARGS__);             \
            while (1);                                      \
        }                                                   \
    }

#define ENDIAN_SWAP_32(val) ((((val) & 0x000000FF) << 24)   \
                            | (((val) & 0x0000FF00) << 8)   \
                            | (((val) & 0x00FF0000) >> 8)   \
                            | (((val) & 0xFF000000) >> 24))

#define ENDIAN_SWAP_16(val) ((((val) & 0x00FF) << 8)        \
                            | (((val) & 0xFF00) >> 8))

#define NIBBLE_SWAP_16(val) ((((val) & 0x000F) << 12)       \
                            | (((val) & 0x00F0) << 4)       \
                            | (((val) & 0x0F00) >> 4)       \
                            | (((val) & 0xF000) >> 12))

#endif /* UTIL_H_ */
