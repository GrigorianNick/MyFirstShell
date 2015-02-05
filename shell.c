#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int run_program(char *input, char* buffer, char **append, int append_size) {
	char * real_input = strdup(input);
	int Pipe[2];
	pid_t pid;
	char ** arguments = NULL;
	char * tok;
	char cwd[1024];
	int arguments_size = 0;
	int status = 0;
	if (!getcwd(cwd, sizeof(cwd))) {
		perror( "Unable to get cwd!" );
		exit(-1);
	}
	printf("input: %s\n", input);
	// Building arguments
	tok = strtok(real_input, " ");
	while (tok) {
		char buff[1024];
		// Adding another element to arguments
		arguments_size++;
		arguments = realloc(arguments, sizeof(char*) * arguments_size);
		
		// Relative pathing
		if (tok[0] != '/' && tok[0] != '-') { // Don't want already addressed paths or options
			strcpy(buff, cwd);
			strcat(buff, "/");
			strcat(buff, tok);
		}
		else {
			strcpy(buff, tok);
		}
		
		// Actually inserting element into arguments
		arguments[arguments_size - 1] = malloc(sizeof(char) * strlen(buff));
		strcpy(arguments[arguments_size - 1], buff);
		
		// Find the next token
		tok = strtok(NULL, " ");
	}

	// Append anything being piped into this system call
	if (!append) { // We're being passed something
		// Cycle through and concatenate append onto arguments
		int i;
		for (i = 0; i < append_size; i++) {
			arguments_size++;
			arguments = realloc(arguments, sizeof(char*) * arguments_size);
			arguments[arguments_size - 1] = malloc(strlen(append[i]));
			strcpy(arguments[arguments_size - 1], append[i]);
		}
	}

	// Null terminate arguments so it'll play nice with execv
	arguments_size++;
	arguments = realloc(arguments, sizeof(char*) *arguments_size);
	arguments[arguments_size - 1] = NULL;

	// Set up pipe and check for errors
	if (pipe(Pipe) == -1) {
		perror( "Pipe error!" );
		exit(-1);
	}

	// Fork here
	pid = fork();
	
	// Check for errors
	if (pid == -1) {
		perror( "Fork error!" );
		exit(-1);
	}
	else if (pid == 0) { // Child
		// Direct stdout into pipe
		close(Pipe[0]); // Don't need to read
		dup2(Pipe[1], 1); // Hooking stdout up to pipe
		execv(arguments[0], arguments);
		perror( "execv error!" ); // We should never see this.
		exit(-1);
	}
	else { //Parent
		// Set up read pipe with stdin
		close(Pipe[1]); // Don't need to write
		dup2(Pipe[0], 0); // Hooking stdin up to pipe
		wait(&status); // Politely look after the kiddos
		char net[1024];
		// Grab the child's output
		while(scanf("%s\n", net) > 0) {
			strcat(buffer, net);
			strcat(buffer, " ");
		}	
		// Clean up after ourselves
		int i;
		for (i = 0; i < arguments_size; i++) {
			free(arguments[i]);
		}
		free(arguments);

		return status; // Report back with the child's exit status
	}
	return -1;
}

int main() {
	char * input = "/bin/pwd"; //ls /home/nick";
	char * buffer = NULL;
	buffer = malloc(sizeof(char) * 1024);
	run_program(input, buffer, NULL, 0);
	printf("Result: %s\n", buffer);
}
