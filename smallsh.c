/**
 * * Contact: tylerwennstrom@gmail.com | wennstrt@oregonstate.edu
 * * Date: Spring 2022
 * * Class: Operating Systems I
 * * Assignment: smallsh
 * * Description: 
 * *   This file contains an implementation of a custom shell for use
 * *   with UNIX operating systems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

// function prototypes here
void PID_to_str(pid_t pid_num, char *buffer);


// TODO: provide prompt for running commands
int main(void)
{
    int break_status = 0;
    int error_status = 0;
    pid_t current_PID = getpid();
    char current_PID_str[50];
    PID_to_str(current_PID, current_PID_str);

    
    // endless loop until break
    while (!break_status)
    {
        // var to hold background or foreground status
        int background_status = 0;
        int fork_status = 0;

        // use the colon symbol as a prompt for each command line
        printf(": ");

        // get input from command line
        FILE *input_file_desc = 0; // set input fd to stdin
        char *input_str_buffer = malloc(250);
        memset(input_str_buffer, 0, 250);
        int scan_items = fscanf(input_file_desc, "%s", input_str_buffer);

        // TODO: handle blank lines and comments, which are lines beginning with the character `#'
        // blank line or comment
        if (scan_items == 0)
        {
            continue;
        } 
        if (input_str_buffer[0] == '#')
        {
            continue;;
        }

        // check command input
        char *argument = strtok(input_str_buffer, " ");
        char *argument_arr[50] = {0};
        int arg_count = 0;
        while (argument != NULL)
        {
            // parse argument for "$$"
            int length = strlen(argument);
            char *temp = malloc(250);
            char temp_char[2] = "\0\0";
            for (int j = 0; j < length - 1; j++)
            {
                if (argument[j] == '$' && 
                    argument[j+1] == '$')
                {
                    strcat(temp, current_PID_str);
                    j++;
                }
                else
                {
                    temp_char[0] = argument[j];
                    strcat(temp, temp_char);
                }
            }
            int temp_len = strlen(temp);
            char new_arg[temp_len + 1];
            new_arg[temp_len] = 0;
            strcpy(new_arg, temp);

            // add argument to vector/array
            argument_arr[arg_count] = new_arg;
            arg_count++;
            argument = strtok(input_str_buffer, " ");
            free(temp);
        }
        free(input_str_buffer);

        // check if background process
        if (strcmp(argument_arr[arg_count - 1], "&") == 0)
        {
            background_status = WNOHANG;
        }

        // TODO: execute 3 commands: `exit' `cd' `status' via built in code
        // check if internal process
        if (strcmp(argument_arr[0], "exit") == 0)
        {
            if ((arg_count - background_status) > 1)
            {
                printf("ERROR\n"
                "`exit' usage: exit\n"
                "`exit' takes no arguments\n");
                continue;
            }
            smallsh_exit(&break_status);
            continue;
        }
        else if (strcmp(argument_arr[0], "cd") == 0)
        {
            if ((arg_count - background_status) > 2)
            {
                printf("ERROR\n"
                "`cd' usage: cd [path to directory]\n");
                continue;
            }
            smallsh_cd(argument_arr[1]);
            continue;
        }
        else if (strcmp(argument_arr[0], "status") == 0)
        {
            if ((arg_count - background_status) > 1)
            {
                printf("ERROR\n"
                "`status' usage: status\n"
                "`status' takes no arguments");
                continue;
            }
            smallsh_status();
            continue;
        }
        else
        {
            fork_status = 1;
        }

        if (fork_status)
        {
            // fork into child process
            int child_status;
            pid_t child_PID = fork();

            switch (child_PID)
            {
            // an error occured
            case (-1):
                perror("Could not implement a proper fork.\n"
                    "Shell exiting.");
                break_status = 1;
                error_status = 1;
                break;

            // child process
            case (0):
                return(0);
                

            // parent process
            default:

                // wait for child process to return
                waitpid(child_PID, &child_status, background_status);
                break;
            }
        }
    }

    return(error_status);
}


// TODO: provide expansion for the variable `$$'


// TODO: execture other commands by creating new processes using a function from the `exec()' family of functions

// TODO: support input and output redirection

// TODO: implement custom handlers for 2 signals, SIGINT and SIGSTP

/**
 * @brief exits smallsh. takes no arguments. when this command is run,
 * the smallsh will kill any other processes or jobs that were started
 * before terminating itself.
 * 
 * @param shell_status pointer for shell status to end the process.
 */
void smallsh_exit(int *shell_status)
{
    *shell_status = 1;
    return;
}

/**
 * @brief changes the working directory of smallsh to path.
 * 
 * @param argv if path set to an empty string, then cd will change 
 * directory to path specified in the HOME environment variable. 
 * allowed to be either absolute or relative.
 * 
 */
void smallsh_cd(char *path)
{
    return;
}

/**
 * @brief prints out either the exit status or the terminating signal of 
 * the last foreground process ran by smallsh.
 * it will return 0 if run before any foreground process, or
 * in the case where any of the functions `exit', `cd', or `status'
 * have been called but no others.
 * 
 */
int smallsh_status(void)
{
    return;
}

/**
 * @brief converts a pid_t number to a string
 * 
 * @param pid_num 
 * @param buffer holds the result string
 */
void PID_to_str(pid_t pid_num, char *buffer)
{
    int pid_int = (int) pid_num;
    sprintf(buffer, "%d", pid_int);
    return;
}