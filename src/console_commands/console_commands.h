/*
 * console_commands.h
 *
 *  Created on: Mar 13, 2017
 *      Author: Ghanan Gowripalans
 */

#ifndef SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_
#define SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_

enum CONSOLE_COMMANDS_ERROR_CODES
{
    CONSOLE_COMMANDS_OK,
    CONSOLE_COMMANDS_ALREADY_INITIALIZED
};

typedef int (* console_command_func_t) (
    char **,
    unsigned int
    );

typedef struct _console_command_t
{
    char const * const command_token;
    char const * const description;
    console_command_func_t func;
} console_command_t;

int
console_commands_init (
    console_command_t const * const commands,
    unsigned int const num_commands
    );

int
console_commands_calibrate_init (
		console_command_t const * const commands,
		unsigned int const num_commands
		);

int
console_commands_calibrate (
		char ** params,
		unsigned int num_params
		);

int
console_commands_run (
    void
    );


#endif /* SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_ */
