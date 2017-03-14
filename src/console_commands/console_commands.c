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

#define BUFFER_SIZE              (101)
#define TOKEN_DELIMITER          (" ")
#define MAX_PARAMS               (10)
#define HELP_COMMAND_CMD         ("help")
#define HELP_COMMAND_DESCRIPTION ("Print all commands.\n" \
                "        Usage: help\n")
#define QUIT_COMMAND_CMD         ("quit")
#define QUIT_COMMAND_DESCRIPTION ("Quit this command processing shell.\n" \
                "        Usage: quit\n")

static bool init_done = 0;
static console_command_t const * all_commands = 0;
static unsigned int num_all_commands = 0;

static
void
console_commands_print_all (
    void
    )
{
    int i;

    NORMAL_PRINT("Commands List:\n");
    NORMAL_PRINT("    <cmd> - <description>\n\n");

    NORMAL_PRINT("    %s - %s\n", HELP_COMMAND_CMD, HELP_COMMAND_DESCRIPTION);
    NORMAL_PRINT("    %s - %s\n", QUIT_COMMAND_CMD, QUIT_COMMAND_DESCRIPTION);

    for (i = 0; i < num_all_commands; ++i)
    {
        NORMAL_PRINT("    %s - %s\n", all_commands[i].command_token,
                all_commands[i].description);
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

int
console_commands_run (
    void
    )
{
    int i;
    int j;
    char buffer[BUFFER_SIZE];
    char * command_token;
    char * params[MAX_PARAMS];
    int ret_val;

    NORMAL_PRINT("Starting interactive console... Hi :)\n");

    while (1)
    {
        NORMAL_PRINT("Enter a command: > ");
        NORMAL_READ((char *) &buffer, BUFFER_SIZE);
        NORMAL_PRINT("\n");

        command_token = strtok(buffer, TOKEN_DELIMITER);

        if (strcmp(command_token, HELP_COMMAND_CMD) == 0)
        {
            console_commands_print_all();
        }
        else if (strcmp(command_token, QUIT_COMMAND_CMD) == 0)
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
                    for (j = 0; j < MAX_PARAMS; ++j)
                    {
                        params[j] = strtok(0, TOKEN_DELIMITER);

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
