/*
 * main.c
 */

#ifndef _TMS320C6X
#error Macro _TMS320C6X is not defined!
#endif

#include "util.h"
#include "interrupt_wrapper/interrupt_wrapper.h"
#include "uart_wrapper/uart_wrapper.h"
#include "timer_wrapper/timer_wrapper.h"
#include "spi_wrapper/spi_wrapper.h"
#include "dac_wrapper/dac_wrapper.h"
#include "edma3_wrapper/edma3_wrapper.h"
#include "mcasp_wrapper/mcasp_wrapper.h"
#include "console_commands/console_commands.h"
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define CHANNEL_POWER DAC_POWER_ON_CHANNEL_7
#define CHANNEL DAC_ADDRESS_CHANNEL_7

#ifndef PI
#define PI (3.14159265358979323846)
#endif

//
// Output frequency of DAC
//
#define SAMPLING_FREQUENCY (8000)

//
// Single tone frequency
//
#define COS_FREQ           (500)

//
// Do not touch the below two
//
#define NUM_SAMPLES        (SAMPLING_FREQUENCY / COS_FREQ)
#define TIMER_MICROSECONDS (1000000 / SAMPLING_FREQUENCY)

static bool volatile timer_flag = 0;

static
void
timer_function (
    void
    )
{
    timer_flag = 1;
}

static
int
test_command (
    char ** params,
    unsigned int num_params
    )
{
    int i;

    NORMAL_PRINT("Test command\n");
    NORMAL_PRINT("    Number of params: %d\n", num_params);

    for (i = 0; i < num_params; ++i)
    {
        NORMAL_PRINT("    Param #%d: %s\n", i, params[i]);
    }

    return 0;
}

static
int
play_single_tone (
    char ** params,
    unsigned int num_params
    )
{
    int ret_val;
    int i;
    unsigned int counter;
    unsigned int max_seconds;
    unsigned int seconds;
    double x;
    double y;
    uint16_t val[NUM_SAMPLES];

    NORMAL_PRINT("Play single tone command\n");

    //
    // Max number of seconds is done by first param
    //
    max_seconds = atoi(params[0]);
    counter = 0;
    seconds = 0;

    NORMAL_PRINT("    Playing single tone of %d Hz with a sampling frequency of %d Hz "
            "for %d seconds\n", COS_FREQ, SAMPLING_FREQUENCY, max_seconds);

    /*
     * Pre calculate samples for simple sinusoid output to dac.
     */
    x = 0;
    for (i = 0; i < NUM_SAMPLES; ++i)
    {
        y = (cos(2 * COS_FREQ * PI * x) + 1) * 32767;
        x += 1.0 / SAMPLING_FREQUENCY;
        val[i] = (uint16_t) y;
    }

    timer_start();
    while (seconds < max_seconds)
    {
        ASSERT(timer_flag == 0, "TIMER IS TOO FAST!\n");
        while (timer_flag == 0);
        timer_flag = 0;

        ret_val = dac_update(CHANNEL, val[i], 0);
        ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

        if (++i == NUM_SAMPLES)
        {
            i = 0;
        }

        if (++counter == SAMPLING_FREQUENCY)
        {
            ++seconds;
            counter = 0;
        }
    }
    timer_stop();

    return 0;
}

static
int
print_mic_data (
    char ** params,
    unsigned int num_params
    )
{
    int ret_val;
    int i;
    unsigned int counter;
    unsigned int max_count;
    uint32_t mic_data[2];

    NORMAL_PRINT("Print mic data command\n");

    //
    // Max number of counds is done by first param
    //
    max_count = atoi(params[0]);
    counter = 0;

    NORMAL_PRINT("Printing mic to data for %d iterations!\n", max_count);

    while (counter++ < max_count)
    {
        //
        // See mic data as binary and shifted
        //
        char s0[33] = "00000000000000000000000000000000";
        char s1[33] = "00000000000000000000000000000000";

        ret_val = mcasp_latest_rx_data((uint32_t *) &mic_data);
        ASSERT(ret_val == MCASP_OK, "MCASP getting latest rx data failed! (%d)\n", ret_val);

        for (i = 0; i < 32; ++i)
        {
            if (mic_data[0] & (0x1 << i))
            {
                s0[31 - i] = '1';
            }

            if (mic_data[1] & (0x1 << i))
            {
                s1[31 - i] = '1';
            }
        }

        NORMAL_PRINT("0: %s %d %d\n", s0, (int16_t) mic_data[0], (mic_data[0] + 32767) & 0xFFFF);
        NORMAL_PRINT("1: %s %d %d\n", s1, (int16_t) mic_data[1], (mic_data[1] + 32767) & 0xFFFF);
    }

    return 0;
}

static
int
play_mic_to_dac (
    char ** params,
    unsigned int num_params
    )
{
    int ret_val;
    uint32_t mic_data[2];
    unsigned int counter;
    unsigned int max_seconds;
    unsigned int seconds;

    NORMAL_PRINT("Play mic to dac command\n");

    //
    // Max number of seconds is done by first param
    //
    max_seconds = atoi(params[0]);
    counter = 0;
    seconds = 0;

    NORMAL_PRINT("Playing mic to data for %d seconds (outputting at a frequency of %d Hz)\n",
            max_seconds, SAMPLING_FREQUENCY);

    timer_start();
    while (seconds < max_seconds)
    {
        ASSERT(timer_flag == 0, "TIMER IS TOO FAST!\n");
        while (timer_flag == 0);
        timer_flag = 0;

        ret_val = mcasp_latest_rx_data((uint32_t *) &mic_data);
        ASSERT(ret_val == MCASP_OK, "MCASP getting latest rx data failed! (%d)\n", ret_val);

        //
        // This adds a Vref/2 DC offset
        //
        ret_val = dac_update(CHANNEL, (uint16_t)((mic_data[0] + 32767) & 0xFFFF), 1);
        ASSERT(ret_val == DAC_OK, "DAC update failed! (%d)\n", ret_val);

        if (++counter == SAMPLING_FREQUENCY)
        {
            ++seconds;
            counter = 0;
        }
    }
    timer_stop();

    return 0;
}

#define NUM_COMMANDS (4)
#define NUM_CAL_COMMANDS (3)

int
main (
    void
    )
{
    int ret_val;
    console_command_t const cal_commands[NUM_CAL_COMMANDS] = {
    		{
    			(char *) "n", // command token
				(char *) "Command to play the next phase increment.\n",
				(console_command_func_t) (0)//TODO
    		},
			{
				(char *) "r", //command token
				(char *) "Command to replay the tone with the current phase difference.\n",
				(console_command_func_t) (0)//TODO
			},
			{
				(char *) "q", // command token
				(char *) "Exit calibration shell, back to main shell.\n",
				(console_command_func_t) (0)//TODO
			},
    };
    console_command_t const commands[NUM_COMMANDS] = {
        {   // 0
            (char *) "test", // command_token
            (char *) "Command to simply test the console command api.\n        "
                "Usage: test [param0] [param1] ... [param9]\n", // description
            (console_command_func_t) &test_command // function pointer
        },
        {   // 1
            (char *) "play-single-tone",
            (char *) "Command to play a single tone.\n        "
                "Usage: play-single-tone <number of seconds>\n",
            (console_command_func_t) &play_single_tone
        },
        {   // 2
            (char *) "print-mic-data",
            (char *) "Command to see mic data in binary and decimal.\n        "
                "Usage: print-mic-data <number of samples>\n",
            (console_command_func_t) &print_mic_data
        },
        {   // 3
            (char *) "play-mic-to-dac",
            (char *) "Output mic data through the DAC.\n        "
                "Usage: play-mic-to-dac <number of seconds>\n",
            (console_command_func_t) &play_mic_to_dac
        },
		{
			(char *) "cal",
			(char *) "Enter the calibration shell.\n        "
				"Usage: cal <frequencyHz of Tone>\n",
			(console_command_func_t) (0)//TODO
		}
    };

    /*
     * Init system interrupts.
     */
    ret_val = init_interrupt();
    ASSERT(ret_val == INTERRUPT_OK, "Interrupt init failed! (%d)\n", ret_val);

    /*
     * Init uart before everything else so debug
     * statements can be seen
     */
    ret_val = uart_init();
    ASSERT(ret_val == UART_OK, "UART Init failed! (%d)\n", ret_val);

    NORMAL_PRINT("\n\nApplication Starting\n");
    DEBUG_PRINT("Debug prints are enabled\n");

    /*
     * Init system interrupts.
     */
    ret_val = edma3_init();
    ASSERT(ret_val == EDMA3_OK, "EDMA3 init failed! (%d)\n", ret_val);

    /*
     * Init McASP
     */
    ret_val = mcasp_init();
    ASSERT(ret_val == DAC_OK, "McASP init failed! (%d)\n", ret_val);

#ifdef MCASP_LOOPBACK_TEST
    /*
     * Loop back test for McASP
     */
    mcasp_loopback_test();
#endif

    /*
     * Init spi.
     */
    ret_val = spi_init();
    ASSERT(ret_val == SPI_OK, "SPI init failed! (%d)\n", ret_val);

    /*
     * Init dac.
     */
    ret_val = dac_init(1);
    ASSERT(ret_val == DAC_OK, "DAC init failed! (%d)\n", ret_val);

    /*
     * Init Timer
     */
    ret_val = timer_init(&timer_function, TIMER_MICROSECONDS);
    ASSERT(ret_val == TIMER_OK, "Timer Init failed! (%d)\n", ret_val);

    // Power on dac channel
    ret_val = dac_power_up(CHANNEL_POWER, 1);
    ASSERT(ret_val == DAC_OK, "DAC power on failed! (%d)\n", ret_val);

#ifdef DAC_DO_NOT_USE_INTERNAL_REFERENCE
    ret_val = dac_internal_reference_power_down(1);
    ASSERT(ret_val == DAC_OK, "DAC internal reference power down failed! (%d)\n", ret_val);
#endif // #ifdef DAC_DO_NOT_USE_INTERNAL_REFERENCE

    /*
     * Init console command
     */
    ret_val = console_commands_init((console_command_t *) &commands, NUM_COMMANDS);
    ASSERT(ret_val == CONSOLE_COMMANDS_OK, "Console commands init failed! (%d)\n", ret_val);

    /*
     * Do whatever commands tell us to do.
     */
    ret_val = console_commands_run();
    ASSERT(ret_val == CONSOLE_COMMANDS_OK, "Console commands run failed! (%d)\n", ret_val);

    /*
     * Loop forever.
     */
    NORMAL_PRINT("Looping forever...\n");
    while (1);

    return 0;
}
