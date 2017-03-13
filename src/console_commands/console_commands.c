/*
 * console_commands.c
 *
 *  Created on: Mar 13, 2017
 *      Author: Ghanan Gowripalan
 */

#include "../config.h"
#include "../util.h"
#include "console_commands.h"
#include <stdbool.h>
#include <string.h>
#define HELP_COMMAND_CMD ("help")
#define HELP_COMMAND_DESCRIPTION ("Print all commands")
#define BUFFER_SIZE (101)
#define TOKEN_DELIMITER (" ")
#define MAX_PARAMS (10)

static bool init_done = 0;

console_command_t const * all_commands = 0;
unsigned int num_all_commands = 0;

static
void
console_commands_print_all (
    void
    )
{
    int i;

    NORMAL_PRINT("Commands List:\n");
    NORMAL_PRINT("<cmd> - <description>\n\n");

    NORMAL_PRINT("    %s - %s\n", HELP_COMMAND_CMD, HELP_COMMAND_DESCRIPTION);

    for (i = 0; i < num_all_commands; ++i)
    {
        NORMAL_PRINT("    %s - %s\n", all_commands[i].cmd, all_commands[i].description);
    }
}

int
console_commands_init (
    console_command_t const * const commands,
    unsigned int const num_commands
    )
{
    if (init_done)
    {
        DEBUG_PRINT("DAC already initialized!\n");
        return CONSOLE_COMMANDS_ALREADY_INITIALIZED;
    }

    DEBUG_PRINT("Initializing console commands...\n");

    all_commands = commands;
    num_all_commands = num_commands;

    return CONSOLE_COMMANDS_OK;
}

void
console_commands_run (
    void
    )
{
    int i;
    int j;
    char buffer[BUFFER_SIZE];
    char * cmd;
    char * params[MAX_PARAMS];

    while (1)
    {
        NORMAL_PRINT("Enter a command: > ");
        NORMAL_READ((char *) &buffer, BUFFER_SIZE);
        NORMAL_PRINT("\n");

        cmd = strtok(buffer, TOKEN_DELIMITER);

        if (strcmp(cmd, HELP_COMMAND_CMD) == 0)
        {
            console_commands_print_all();
        }
        else
        {
            for (i = 0; i < num_all_commands; ++i)
            {
                if (strcmp(cmd, all_commands[i].cmd) == 0)
                {
                    //
                    // Get all params
                    //
                    for (j = 0; j < MAX_PARAMS; ++j)
                    {
                        params[j] = strtok(0, TOKEN_DELIMITER);

                        if (params[j] == 0)
                        {
                            // No more tokens
                            break;
                        }
                    }

                    all_commands[i].func((char **) &params, j);

                    break;
                }
            }

            if (i == num_all_commands)
            {
                NORMAL_PRINT("Invalid Command; use the help command to see all commands!\n");
            }
        }
    }
}
