/*
 * console_commands.h
 *
 *  Created on: Mar 13, 2017
 *      Author: Ghanan Gowripalans
 */

#ifndef SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_
#define SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_

#define CONSOLE_COMMANDS_BUFFER_SIZE              (101)
#define CONSOLE_COMMANDS_TOKEN_DELIMITER          (" ")
#define CONSOLE_COMMANDS_MAX_PARAMS               (10)
#define CONSOLE_COMMANDS_HELP_COMMAND_CMD         ("help")
#define CONSOLE_COMMANDS_HELP_COMMAND_DESCRIPTION ("Print all commands.\n" \
                "        Usage: help\n")
#define CONSOLE_COMMANDS_QUIT_COMMAND_CMD         ("quit")
#define CONSOLE_COMMANDS_QUIT_COMMAND_DESCRIPTION ("Quit this command processing shell.\n" \
                "        Usage: quit\n")

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
console_commands_run (
    console_command_t const * const commands,
    unsigned int const num_commands
    );


#endif /* SRC_CONSOLE_COMMANDS_CONSOLE_COMMANDS_H_ */
