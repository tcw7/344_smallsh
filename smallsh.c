/**
 * * Contact: tylerwennstrom@gmail.com | wennstrt@oregonstate.edu
 * * Date: Spring 2022
 * * Class: Operating Systems I
 * * Assignment: smallsh
 * * Description: 
 * *   This file contains an implementation of a custom shell for use
 * *   with UNIX operating systems.
 */


#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>

// function prototypes here
void PID_to_str(pid_t pid_num, char *buffer);
int smallsh_status(int *ch_status);
void smallsh_cd(char *path);
void smallsh_exit(int *break_status, int *error_status);
void handle_SIGTSTP(int signo);
static volatile sig_atomic_t foreground_only_switch = 0;


int main(void)
{
    int child_status = 0;
    int break_status = 0;
    int error_status = 0;
    pid_t current_PID = getpid();
    char current_PID_str[50];
    PID_to_str(current_PID, current_PID_str);
    pid_t child_PID;

    // signal handlers
    struct sigaction ignore_sig_action = {0};
    ignore_sig_action.sa_handler = SIG_IGN;
    sigfillset(&ignore_sig_action.sa_mask);
    struct sigaction default_sig_action = {0};
    default_sig_action.sa_handler = SIG_DFL;
    struct sigaction handle_SIGTSTP_action = {0};
    handle_SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&handle_SIGTSTP_action.sa_mask);
    handle_SIGTSTP_action.sa_flags = 0;
    sigaction(SIGINT, &ignore_sig_action, NULL);
    sigaction(SIGTSTP, &handle_SIGTSTP_action, NULL);
    
    // endless loop until break
    while (!break_status)
    {
        // check for child termination
        int child_exit_status;
        pid_t returned_child_pid = waitpid(-1, &child_exit_status, WNOHANG);
        if (returned_child_pid != 0 && returned_child_pid != -1) {
            printf("Child process #%d terminated with exit status: %d\n", (int) returned_child_pid, (int) child_exit_status);
        }

        // var to hold background or foreground status
        int background_status = 0;
        int fork_status = 0;

        // use the colon symbol as a prompt for each command line
        printf("smallsh: ");
        fflush(stdout);

        // get input from command line
        FILE *input_file_desc = stdin; // set input fd to stdin
        char *input_str_buffer = malloc(2064);
        memset(input_str_buffer, 0, 2064);
        input_str_buffer = fgets(input_str_buffer, 2064, input_file_desc);
        if (input_str_buffer == NULL) {
            continue;
        }

        // handle blank lines or comment lines
        if ((strcmp(input_str_buffer, "\n")) == 0)
        {
            free(input_str_buffer);
            continue;
        } if (input_str_buffer[0] == '#')
        {
            free(input_str_buffer);
            continue;
        }

        // check command input
        char *argument = strtok(input_str_buffer, " ");
        argument[strcspn(argument, "\n")] = 0; // remove trailing \n 
        char *argument_arr[524] = {0};
        int arg_count = 0;
        int length;
        while (argument != NULL)
        {
            // parse argument for "$$"
            length = strlen(argument);
            char *temp = malloc(2064);
            memset(temp, 0, 2064);
            char temp_char[2] = "\0\0";
            for (int j = 0; j < (length); j++)
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
            temp = realloc(temp, (temp_len + 1));

            // add argument to vector/array
            argument_arr[arg_count] = temp;
            arg_count++;
            argument = strtok(NULL, " ");
            if (argument != NULL)
            {
                argument[strcspn(argument, "\n")] = 0;
            }
        }
        free(input_str_buffer);

        // check if background process
        if (strcmp(argument_arr[arg_count - 1], "&") == 0)
        {
            background_status = WNOHANG;
            argument_arr[arg_count - 1] = NULL;
            arg_count--;

            if (foreground_only_switch == 1) {
                background_status = 0;
            }
        }

        // TODO: execute 3 commands: `exit' `cd' `status' via built in code
        // check if internal process

        // exit internal process
        if ((strcmp(argument_arr[0], "exit")) == 0)
        {
            if ((arg_count - background_status) > 1)
            {
                printf("ERROR\n"
                "`exit' usage: exit\n"
                "`exit' takes no arguments\n");
            }
            smallsh_exit(&break_status, &error_status);
        }

        // cd internal process
        else if ((strcmp(argument_arr[0], "cd")) == 0)
        {
            if ((arg_count - background_status) > 2)
            {
                printf("ERROR\n"
                "`cd' usage: cd [path to directory]\n");
            }
            smallsh_cd(argument_arr[1]);
            char *temp_cwd = malloc(100);
            printf("The cwd is: %s\n", getcwd(temp_cwd, 100));
            fflush(stdout);
            free(temp_cwd);
        }

        // status internal process
        else if ((strcmp(argument_arr[0], "status")) == 0)
        {
            if ((arg_count - background_status) > 1)
            {
                printf("ERROR\n"
                "`status' usage: status\n"
                "`status' takes no arguments");
            }
            smallsh_status(&child_status);
        }
        else
        {
            fork_status = 1;
        }

        // child process required to exec external command
        if (fork_status == 1)
        {
            // change signal handlers for all child procs
            sigaction(SIGTSTP, &ignore_sig_action, NULL);
            sigaction(SIGINT, &default_sig_action, NULL);

            // if child is background, ignore SIGINT
            if (background_status == WNOHANG) {
                sigaction(SIGINT, &ignore_sig_action, NULL);
            }
            
            // fork into child process
            child_PID = fork();

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
            case (0): ;

                // check if I/O redirection
                char *arg_io;
                char *new_arg_arr[524] = {0};
                int new_arg_count = 0;
                int in_redirect = 0;
                int out_redirect = 0;
                for (int k = 0; k < arg_count; k++) {
                    arg_io = argument_arr[k];
                    
                    // case if input redirection is requested
                    if (strcmp(arg_io, "<") == 0) {

                        // open filepath of next argument if possible
                        int new_fd_in = open(argument_arr[k+1], O_RDONLY);
                        if (new_fd_in == -1) {
                            perror("Could not open input file for"
                                   " redirection");
                            exit(1);

                        // redirect input to new fd
                        } else {
                            if ((dup2(new_fd_in, 0)) == -1) {
                                perror("Could not redirect input");
                                exit(1);
                            }

                            // set bool for input redirect set
                            in_redirect = 1;
                        }
                        k++;

                    // case if output redirection is requested
                    } else if (strcmp(arg_io, ">") == 0) {

                        // open filepath of next argument if possible
                        int new_fd_out = open(argument_arr[k+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (new_fd_out == -1) {
                            perror("Could not open output file"
                                   " for redirection");
                            exit(1);

                        // redirect output to output fd
                        } else {
                            if ((dup2(new_fd_out, 1)) == -1) {
                                perror("Could not redirect output");
                                exit(1);
                            }

                            // set bool for output redirect set
                            out_redirect = 1;
                        }
                        k++;
                    } else {
                        new_arg_arr[new_arg_count] = arg_io;
                        new_arg_count++;
                    }
                }

                // check for background process redirection
                if (background_status == WNOHANG) {

                    // no input redirection given
                    if (in_redirect == 0) {
                        
                        // open /dev/null
                        int new_fd_in = open("/dev/null", O_RDONLY);
                        if (new_fd_in == -1) {
                            perror("Could not open /dev/null for"
                                    " redirection");
                            exit(1);

                        // redirect input
                        } else {
                            if ((dup2(new_fd_in, 0)) == -1) {
                                perror("Could not redirect input to /dev/null");
                                exit(1);
                            }
                        }
                    }
                    
                    // no output redirection given
                    if (out_redirect == 0) {

                        // no output redirection given
                        int new_fd_out = open("/dev/null", O_WRONLY);
                        if (new_fd_out == -1) {
                            perror("Could not open /dev/null"
                                    " for redirection");
                            exit(1);

                        // redirect output
                        } else {
                            if ((dup2(new_fd_out, 1)) == -1) {
                                perror("Could not redirect output to /dev/null");
                                exit(1);
                            }
                        }
                    }
                }

                int exit_status = execvp(new_arg_arr[0], new_arg_arr);
                if (exit_status == -1) {
                    exit(1);
                }
                exit(exit_status);
                

            // parent process
            default:
                sigaction(SIGINT, &ignore_sig_action, NULL);

                // wait for child process to return
                waitpid(child_PID, &child_status, background_status);
                if (background_status != WNOHANG) {
                    if (WIFEXITED(child_status)) {
                        if (WEXITSTATUS(child_status) == 1) {
                            error(0, WEXITSTATUS(child_status), "ERROR");
                        }
                    }
                    if (WIFSIGNALED(child_status)) {
                        printf("terminated by signal %d\n", (int) WTERMSIG(child_status));
                    }
                }
                
                if (background_status == WNOHANG) {
                    printf("Background process initialized: %d\n", (int) child_PID);
                }

                // restore signal handlers
                sigaction(SIGTSTP, &handle_SIGTSTP_action, NULL);
                break;
            }
        }

        // free argument pointers
        for (int i = 0; i < arg_count; i++)
        {
            free(argument_arr[i]);
        }
    }

    return(error_status);
}

// TODO: implement custom handlers for 2 signals, SIGINT and SIGSTP

/**
 * @brief exits smallsh. takes no arguments. when this command is run,
 * the smallsh will kill any other processes or jobs that were started
 * before terminating itself.
 * 
 * @param shell_status pointer for shell status to end the process.
 */
void smallsh_exit(int *break_status, int *error_status)
{
    *error_status = kill(0, SIGTERM);
    if (*error_status != 0)
    {
        perror("ERROR: could not terminate shell processes correctly\n");
    }
    *break_status = 1;
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
    char *home_path = getenv("HOME");
    if (path == NULL) {
        chdir(home_path);
    } else if (strcmp(path, "&") == 0) {
        chdir(home_path);
    } else {
        if (chdir(path)) {
            perror("ERROR: could not change directory");
        }
    }
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
int smallsh_status(int *ch_status)
{
    if (WIFEXITED(*ch_status)) {
        printf("last foreground process exit value: %d\n", WEXITSTATUS(*ch_status));
    } else {
        printf("last foreground process terminated with signal value: %d\n", WTERMSIG(*ch_status));
    }
    fflush(stdout);
    return *ch_status;
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

// Signal Handler
/**
 * @brief signal handler for parent process SIGTSTP
 * 
 * @param signo 
 */
void handle_SIGTSTP(int signo){
    if (foreground_only_switch == 0) {
        foreground_only_switch = 1;
        char* message = "\nForeground-only mode activated.\n\0";
        write(STDOUT_FILENO, message, 35);
    } else {
        foreground_only_switch = 0;
        char* message = "\nForeground-only mode disabled.\n\0";
        write(STDOUT_FILENO, message, 34);
    }
    return;
}