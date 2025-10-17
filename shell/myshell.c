#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "myshell_parser.h"

void execute_pipeline(struct pipeline *my_pipeline) {
	// Find first command
	struct pipeline_command *current_command = my_pipeline->commands;
	bool is_first_command = true;
	// File descriptors
	int fd[2];
	int redirect_in_fd;
	int redirect_out_fd;
	bool redirect_in = false;
	bool redirect_out = false;

	if (current_command->next != NULL) {
		pipe(fd);
	}

	// Loop through command pipeline and execute
	while (current_command != NULL) {
		// Define variables for forking
		pid_t pid;
		int status;

		// Fork
		pid = fork();
		if (pid == 0){
			// In child process
			
			// Check for command redirect in path
			if (current_command->redirect_in_path != NULL) {
				redirect_in_fd = open(current_command->redirect_in_path, O_RDONLY | O_CREAT, 0777);
				if ((redirect_in_fd == -1)) {
					pipeline_free(my_pipeline);
					perror("ERROR: Could not open redirect in path");
					exit(EXIT_FAILURE);
				}
				dup2(redirect_in_fd, STDIN_FILENO);
				close(redirect_in_fd);
			}
			// Check for command redirect out path
			if (current_command->redirect_out_path != NULL) {
				redirect_out_fd = open(current_command->redirect_out_path, O_WRONLY | O_CREAT, 0777);
				if ((redirect_out_fd == -1)) {
					pipeline_free(my_pipeline);
					perror("ERROR: Could not open redirect out path");
					exit(EXIT_FAILURE);
				}
				dup2(redirect_out_fd, STDOUT_FILENO);
				close(redirect_out_fd);
				redirect_out = true;
			}
			
			// Check if it is the first command and if not have STDIN be fd[1] (output of previous command)
			if (!is_first_command) {
				if (!redirect_in) {
					dup2(fd[0], STDIN_FILENO);
					close(fd[0]);
				}
			}

			if (current_command->next != NULL) {
				if (!is_first_command) {
					pipe(fd);
				}
				if (!redirect_out) {
					dup2(fd[1], STDOUT_FILENO);
					close(fd[1]);
				}
			}
			// Execute command
			if (execvp(current_command->command_args[0], current_command->command_args) == -1) {
				// Return error
				perror("ERROR: Command execution failed");
				exit(EXIT_FAILURE);
			}
		}
		else {
			// In parent process
			if (!my_pipeline->is_background) {
				waitpid(pid, &status, 0);
				if (status != 0) {
					pipeline_free(my_pipeline);
					exit(EXIT_FAILURE);
				}
			}
			is_first_command = false;
			if (current_command->next != NULL) {
				close(fd[1]);
			}
		}
		
		// Go to next command
		current_command = current_command->next;
	}
}

void sigchld_handler(int signum) {
	int saved_errno = errno;
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
	errno = saved_errno;
}

int main(int argc, char* argv[]) {
	// Set up signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, 0) == -1) {
		perror("sigaction");
		exit(1);
	}
	char command[MAX_LINE_LENGTH];
	bool prompt = true;
	struct pipeline *my_pipeline = NULL;
	//Check for -n arguement
	if (argc > 1) {
		if (strcmp(argv[1], "-n") == 0) {
			prompt = false;
		}
	}
	//Print prompt
	if (prompt == true) {
		printf("my_shell$");
		fflush(stdout);
	}
	while(fgets(command, sizeof(command), stdin)) {
		//Check for EOF (CRTL-D entered)
		if (command == NULL) {
			if (my_pipeline != NULL) {
				pipeline_free(my_pipeline);
			}
			break;
		}
		//Generate pipeline from command, execute it, then free it
		my_pipeline = pipeline_build(command);
		execute_pipeline(my_pipeline);	
		pipeline_free(my_pipeline);
		//Print prompt
		if (prompt == true) {
			printf("my_shell$");
			fflush(stdout);
		}
	}
	printf("\n");
	return 0;
}
