
# Quash Shell Project

Author: Daniel Webster

## Introduction

The design of the shell followed a step by step approach, based on the tasks given in the Codio. Below, each task is discussed in detail, highlighting the design justifications and decisions made during the implementation.

## Task 1: The Prompt and Built in Commands

The shell begins by printing a prompt that includes the current working directory, enhancing user experience by providing context. This is achieved using the getcwd function, which retrieves the current directory.

The shell supports built-in commands. These include: 

- cd: Changes the current directory.
- pwd: Prints the current working directory.
- echo: Displays text or environment variables.
- exit: Terminates the shell.
- env: Displays all environment variables.
- setenv: Sets environment variables.

These functions where implemented by using system calls like chdir, getenv and getcwd. These commands are executed without having to make a new processes.

## Task 2: Adding Processes

The shell handles process creation and execution through the fork() and execvp() system calls. When a command is entered, the shell first uses fork() to create a new child process. This ensures that the parent shell process remains active while the child executes the given command. The child process then uses execvp() to replace its own image with the specified command, allowing the system to run the desired program. This design ensures that each command runs independently, preventing the shell from being blocked by long-running tasks, while allowing for seamless management of multiple commands.

## Task 3: Adding Background Processes

To allow for background processes, the shell checks for an & at the end of the arguemnt. This was done by adding an if statment to when the command is being parsed and then setting the background boolean to true. When a command is to be executed, the shell forks a new process using fork(). The child process executes the command with execvp(). If the command is designated to run in the background, the parent process does not wait for it to finish.

## Task 4: Signal Handling

This task focuses on preventing the user from exiting the shell using Crtl+c. Instead of terminating the shell, a sigint_handler catches the signal. If no foreground process is running, the handler simply reprints the prompt, allowing continued input. If a foreground process is active, the signal is forwarded using the kill() system call to terminate it without affecting the shell. This approach enhances usability, enabling graceful handling of interruptions and better control over running processes.

## Task 5: Killing off Long Running Processes

This is similar to task 4 as it uses signal handling; but in this case its used to end processes taking an extendded amount of time. When a foreground process is started, the shell sets an alarm using the alarm() system call. This call triggers a SIGALRM signal after 10 seconds has past. The shell registers a signal handler for SIGALRM that responds to the alarm signal. If the foreground process exceeds 10 seconds, the signal handler is invoked. The kill system call is then called to terminate the foreground processe forcefully.

## Task 6: Implement Output Redirection

Output redirection allows users to send command output to a file rather than displaying it on the screen. This is done by parsing commands for the > operator. When the '>' is detected, the shell flags this as an indication to redirect output and then takes in the subsequent token as the target output file. To manage the file for output, the shell uses the open() system call. This call opens the specified file in write mode, creating it if it does not exist, or truncating it (clearing its contents) if it does. The flags used with open() include:

- O_WRONLY: Open the file for writing.
- O_CREAT: Create the file if it does not exist.
- O_TRUNC: Truncate the file to zero length if it already exists.

The permissions for the newly created file are set to 0644, which allows the owner to read and write the file while giving read access to others.

## Error Handling

The shell checks for common errors, such as invalid directory paths in the cd command or missing output file names in redirection. Error messages are outputted to the console telling the user these is an issue with their input.

## Documentation

Each function is accompanied by comments explaining its purpose and functionality, while consistent naming conventions aid in readability.




