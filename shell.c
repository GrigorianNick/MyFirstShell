#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int run_program(char *input) {
	if (!strcmp(input, "")) {
		return 0;
	}
	int Pipe[2];
	int status;
	char * tok;
	char last_pipe;
	char lag[1024];
	char curr[1024];
	strcpy(curr, "");
	pid_t pid;
	
	tok = strtok(input, " ");
	while(tok) {
		if (!strcmp(tok, "|")) {
			last_pipe = '|';
			strcat(lag, curr);
			strcat(lag, " ");
			strcpy(curr, "");
		}
		else if (!strcmp(tok, ">")) {
			last_pipe = '>';
			strcat(lag, curr);
			strcat(lag, " ");
			strcpy(curr, "");
		}
		else if (!strcmp(tok, "<")) {
			last_pipe = '<';
			strcat(lag, curr);
			strcat(lag, " ");
			strcpy(curr, "");
		}
		strcat(curr, tok);
		strcat(curr, " ");
		tok = strtok(NULL, " ");
	}

	char ** args = NULL;
	int args_size = 0;
	// Building args
	tok = strtok(curr, " ");
	while(tok) {
		if (strcmp(tok, "<") && strcmp(tok, ">") && strcmp(tok, "|")) {
			args_size++;
			args = realloc(args, sizeof(char*));
			args[args_size - 1] = malloc(sizeof(char*) * strlen(tok));
			strcpy(args[args_size - 1],tok);
		}
		tok = strtok(NULL, " ");
	}
	// Null terminating args so it'll play nice with execv
	args_size++;
	args = realloc(args, sizeof(char*));
	args[args_size - 1] = NULL;

	// Set up pipe
	if (pipe(Pipe) == -1) {
		perror( "Pipe error!" );
		exit(-1);
	}

	// Fork here
	pid = fork();
	
	if (pid == -1) {
		perror( "Fork error!" );
		exit(-1);
	}
	else if (pid == 0) { // Child
		close(Pipe[0]);
		dup2(Pipe[1], 1);
		// See if we're being piped anything else need to look at anything else
		run_program(lag); // Down the rabbit hole we go!
	}
	else { // Parent
		close(Pipe[1]);
		dup2(Pipe[0], 0);
		wait(&status);
		if(!status) { // Child exited fine, so we're good to go
			execv(args[0], args);
		}
		else {
			exit(-1);
		}
	}
}

int main() {
	char input[1024];
	strcpy(input, "/bin/ls /home/nick | /bin/grep D | /bin/grep o");
	//strcpy(input, "/bin/ls /home/nick");
	run_program(input);
}
