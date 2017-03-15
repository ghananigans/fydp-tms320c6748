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

static
void
console_commands_print_all (
    console_command_t const * const all_commands,
    unsigned int const num_all_commands
    )
{
    int i;

    NORMAL_PRINT("Commands List:\n");
    NORMAL_PRINT("    <cmd> - <description>\n\n");

    NORMAL_PRINT("    %s - %s\n",
            CONSOLE_COMMANDS_HELP_COMMAND_CMD, CONSOLE_COMMANDS_HELP_COMMAND_DESCRIPTION);
    NORMAL_PRINT("    %s - %s\n",
            CONSOLE_COMMANDS_QUIT_COMMAND_CMD, CONSOLE_COMMANDS_QUIT_COMMAND_DESCRIPTION);

    for (i = 0; i < num_all_commands; ++i)
    {
        NORMAL_PRINT("    %s - %s\n", all_commands[i].command_token,
                all_commands[i].description);
    }
}

int
console_commands_run (
    console_command_t const * const all_commands,
    unsigned int const num_all_commands
    )
{
    int i;
    int j;
    char buffer[CONSOLE_COMMANDS_BUFFER_SIZE];
    char * command_token;
    char * params[CONSOLE_COMMANDS_MAX_PARAMS];
    int ret_val;

    NORMAL_PRINT("Starting interactive console... Hi :)\n");

    while (1)
    {
        NORMAL_PRINT("Enter a command: > ");
        NORMAL_READ((char *) &buffer, CONSOLE_COMMANDS_BUFFER_SIZE);
        NORMAL_PRINT("\n");

        command_token = strtok(buffer, CONSOLE_COMMANDS_TOKEN_DELIMITER);

        if (strcmp(command_token, CONSOLE_COMMANDS_HELP_COMMAND_CMD) == 0)
        {
            console_commands_print_all(all_commands, num_all_commands);
        }
        else if (strcmp(command_token, CONSOLE_COMMANDS_QUIT_COMMAND_CMD) == 0)
        {
            NORMAL_PRINT("Quitting interactive console... Bye :(\n");
            break;
        }
        else
        {
            for (i = 0; i < num_all_commands; ++i)
            {
                if (strcmp(command_token, all_commands[i].command_token) == 0)
                {
                    //
                    // Get all params
                    //
                    for (j = 0; j < CONSOLE_COMMANDS_MAX_PARAMS; ++j)
                    {
                        params[j] = strtok(0, CONSOLE_COMMANDS_TOKEN_DELIMITER);

                        if (params[j] == 0)
                        {
                            // No more tokens
                            break;
                        }
                    }

                    ret_val = all_commands[i].func((char **) &params, j);

                    if (ret_val == 0)
                    {
                        NORMAL_PRINT("Command returned successfully.\n");
                    }
                    else
                    {
                        NORMAL_PRINT("Command returned with error code %d.\n", ret_val);
                    }

                    break;
                }
            }

            if (i == num_all_commands)
            {
                NORMAL_PRINT("Invalid Command; use the help command to see all commands!\n");
            }
        }
    }

    return CONSOLE_COMMANDS_OK;
}
