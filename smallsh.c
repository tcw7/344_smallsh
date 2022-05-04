/* 
* Author: Tyler Wennstrom
* Contact: tylerwennstrom@gmail.com | wennstrt@oregonstate.edu
* Class: Operating Systems I
* Assignment: smallsh
* Description: 
*   This file contains an implementation of a custom shell for use
*     with UNIX operating systems.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>


// TODO: provide prompt for running commands
int main(int argc, char *argv[])
{
    // endless loop until break
    int status = 0;
    while (!status)
    {
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
            return(0);
        } 
        if (input_str_buffer[0] == "#")
        {
            return(0);
        }

        // check command
        char *argument = strtok(input_str_buffer, " ");
        char *argument_arr[50] = {0};
        int i = 0;
        while (argument != NULL)
        {
            argument_arr[i] = argument;
            argument = strtok(input_str_buffer, " ");
        }

        // fork into child process
        int child_status;
        pid_t child_PID = fork();

        switch (child_PID)
        {
        // an error occured
        case (-1):
            free(input_str_buffer);
            perror("Could not implement a proper fork.\n"
                   "Shell exiting.");
            exit(1);

        // child process
        case (0):
            return(0);
            

        // parent process
        default:
            // wait for child process to return
            waitpid(child_PID, &child_status, 0);

            free(input_str_buffer);
            break;
        }
    }

    return(0);
}


// TODO: provide expansion for the variable `$$'

// TODO: execute 3 commands: `exit' `cd' `status' via built in code

// TODO: execture other commands by creating new processes using a function from the `exec()' family of functions

// TODO: support input and output redirection

// TODO: implement custom handlers for 2 signals, SIGINT and SIGSTP

