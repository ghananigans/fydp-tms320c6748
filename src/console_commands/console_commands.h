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

typedef struct _console_command_t
{
    char const * const cmd;
    char const * const description;
    int (* const func)(void *);
} console_command_t;

int
console_commands_init (
    console_command_t const * const commands,
    unsigned int const num_commands
    );

void
console_commands_run (
    void
    );


#endif /* SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_ */
