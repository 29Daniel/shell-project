/*
Quash Shell Project
Author: Daniel Webster
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128
#define TIMEOUT 10 // Time limit for foreground processes

char prompt[] = "> ";
char delimiters[] = " \t\r\n";
extern char **environ;
pid_t shell_pid;
pid_t fg_pid = -1; // PID of the foreground process

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    if (fg_pid != -1) {
        // Send SIGINT to the foreground process
        kill(fg_pid, SIGINT);
    } else {
        printf("%s> ", getcwd(NULL, 0)); // Reprint the prompt
        fflush(stdout);
    }
}

// Signal handler for SIGALRM (timeout)
void sigalrm_handler(int signum) {
    if (fg_pid != -1) {
        // Time limit exceeded, kill the foreground process
        printf("\nForeground process exceeded time limit of %d seconds. Terminating...\n", TIMEOUT);
        kill(fg_pid, SIGKILL); // Forcefully terminate the process
        fg_pid = -1;           // Reset foreground PID
    }
}

int main() {
    // Stores the string typed into the command line.
    char command_line[MAX_COMMAND_LINE_LEN];
  
    // Stores the tokenized command line input.
    char *arguments[MAX_COMMAND_LINE_ARGS];
    
    // Get the shell's PID
    shell_pid = getpid();

    // Install the SIGINT and SIGALRM handlers
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalrm_handler);
    	
    while (true) {
        do { 
            // Print the shell prompt with current working directory
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("%s> ", cwd);
            fflush(stdout);

            // Read input from stdin and store it in command_line.
            if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
                fprintf(stderr, "fgets error");
                exit(0);
            }
 
        } while (command_line[0] == 0x0A);  // while just ENTER pressed

        // If the user input was EOF (ctrl+d), exit the shell.
        if (feof(stdin)) {
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        // Tokenize the command line input (split it on whitespace)
        int i = 0;
        arguments[i] = strtok(command_line, delimiters);
        char *output_file = NULL; // To store the output file name
        bool redirect_output = false; // Flag for redirection

        while (arguments[i] != NULL) {
            if (strcmp(arguments[i], ">") == 0) {
                // If '>' is found, set redirection flag and get the output file name
                redirect_output = true;
                output_file = strtok(NULL, delimiters); // Get the next token (file name)
                if (output_file == NULL) {
                    fprintf(stderr, "Error: No output file specified after '>'\n");
                    redirect_output = false; // Reset if no file is specified
                }
                arguments[i] = NULL; // Remove '>' from arguments
                break;
            }
            i++;
            arguments[i] = strtok(NULL, delimiters);
        }

        // If the command is empty, continue to the next iteration
        if (arguments[0] == NULL) {
            continue;
        }

        // Check if the command should run in the background
        bool background = false;
        if (i > 0 && strcmp(arguments[i - 1], "&") == 0) {
            background = true;
            arguments[i - 1] = NULL;  // Remove the '&' from arguments
        }

        // Built-in command: cd
        if (strcmp(arguments[0], "cd") == 0) {
            if (arguments[1] != NULL) {
                if (chdir(arguments[1]) != 0) {
                    perror("cd: error");
                }
            } else {
                fprintf(stderr, "cd: missing argument\n");
            }
            continue;
        }

        // Built-in command: pwd
        if (strcmp(arguments[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd error");
            }
            continue;
        }

        // Built-in command: echo
        if (strcmp(arguments[0], "echo") == 0) {
          int i;
          for (i = 1; arguments[i] != NULL; i++) {
              if (arguments[i][0] == '$') {
                  // Handle environment variable
                  char *env_value = getenv(arguments[i] + 1); // Skip the '$'
                  if (env_value != NULL) {
                      printf("%s ", env_value);
                  } else {
                      printf(" ");
                  }
              } else {
                  printf("%s \n", arguments[i]);
              }
          }
          continue;
        }

        // Built-in command: exit
        if (strcmp(arguments[0], "exit") == 0) {
            return 0;
        }

        // Built-in command: env
        if (strcmp(arguments[0], "env") == 0) {
          int i;
          for (i = 0; environ[i] != NULL; i++) {
              printf("%s\n", environ[i]);
          }
          continue;
        }

        // Built-in command: setenv
        if (strcmp(arguments[0], "setenv") == 0) {
          if (arguments[1] != NULL && arguments[2] != NULL) {
              if (setenv(arguments[1], arguments[2], 1) == 0) {
                  printf("Environment variable set: %s=%s\n", arguments[1], arguments[2]);
              } else {
                  perror("setenv error");
              }
          } else {
              fprintf(stderr, "setenv: missing arguments\n");
          }
          continue;
        }
      
        // Fork a child process for other commands
        fg_pid = fork();
        if (fg_pid < 0) {
            perror("fork error");
        } else if (fg_pid == 0) {
            // Child process: Reset the SIGINT handler to the default so it can be killed
            signal(SIGINT, SIG_DFL);

            // Redirect output if needed
            if (redirect_output) {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("open error");
                    exit(EXIT_FAILURE);  // Exit if file open fails
                }
                // Redirect stdout to the file
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);  // Exit if dup2 fails
                }
                close(fd); // Close the file descriptor after duplicating
            }

            // Execute the command
            if (execvp(arguments[0], arguments) == -1) {
                perror("execvp error");
                exit(EXIT_FAILURE);  // Exit the child process if exec fails
            }
        } else {
            // Parent process: Wait for the child process if it's a foreground process
            if (!background) {
                // Set an alarm for the TIMEOUT
                alarm(TIMEOUT); // 10 seconds timeout

                int status;
                waitpid(fg_pid, &status, 0);  // Wait for the foreground process to finish

                // Reset the alarm once the process is done
                alarm(0);

                // Reset foreground PID after the process finishes
                fg_pid = -1;

                if (WIFEXITED(status)) {
                    int exit_status = WEXITSTATUS(status);
                    if (exit_status != 0) {
                        printf("An error occurred with status %d.\n", exit_status);
                    }
                }
            } else {
                // If the process is in the background, just print the PID
                printf("Process running in the background with PID: %d\n", fg_pid);
            }
        }
    }

    return -1; // Should never be reached
}
