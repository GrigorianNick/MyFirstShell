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
			tok = strtok(NULL, " ");
			FILE * fp;
			fp = fopen(tok, "r");
			char c[1024];
			while (fgets(c, sizeof(c), fp)) {
				c[strcspn(c, "\n")] = '\0'; // Stripping the newline
				strcat(curr, c);
				strcat(curr, " ");
			}
			last_pipe = '<';
		}
		if (last_pipe != '<') {
			strcat(curr, tok);
			strcat(curr, " ");
			//last_pipe = '0';
		}
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
	// Adding relative pathing to args[0], which is either a command or a file
	if (args[0][0] != '/') {
		char cwd[1024];
		// did cwd error out?
		if (!getcwd(cwd, sizeof(cwd))) {
			perror( "Unable to get cwd!" );
			exit(-1);
		}
		char temp[1024];
		strcpy(temp, cwd);
		strcat(temp, "/");
		strcat(temp, args[0]);
		args[0] = realloc(args[0], sizeof(char) * strlen(temp));
		strcpy(args[0], temp);
	}

	/*int i;
	for (i = 0; i < args_size; i++) {
		printf("args: %s\n", args[0]);
	}
	printf("last_pipe: %c\n============\n", last_pipe);*/

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
		run_program(lag); // Down the rabbit hole we go!
	}
	else { // Parent
		close(Pipe[1]);
		dup2(Pipe[0], 0);
		wait(&status);
		if(!status) { // Child exited fine, so we're good to go
			perror("Child status");
			if (last_pipe == '<') {
				char read_line[1024];
				char read_total[1024];
				strcpy(read_total, "");
				while(fgets(read_line, sizeof(read_line), stdin)) {
					read_line[strcspn(read_line, "\n")] = '\0';
					strcat(read_total, read_line);
					strcat(read_total, " ");
				}
				printf("%s\n", read_total);
			}
			else if (last_pipe == '>') {
				perror("Writing thingy");
				FILE * fp = fopen(args[0], "w+");
				char read_line[1024];
				char read_total[1024];
				strcpy(read_total, "");
				while(fgets(read_line, sizeof(read_line), stdin)) {
					read_line[strcspn(read_line, "\n")] = '\0';
					strcat(read_total, read_line);
					strcat(read_total, " ");
				}
				fwrite(read_total, 1, strlen(read_total), fp);
			}
			else {
				execv(args[0], args);
			}
		}
		else {
			perror( "Child status:" );
			exit(-1);
		}
	}
}

int main() {
	char input[1024];
	char read_line[1024];
	int orig_stdin = dup(0);
	size_t n;// = sizeof(input);
	printf("=> ");
	while(1) {
		if (fgets(input, sizeof(input), stdin) == NULL) {
			break;
		}
		input[strcspn(input, "\n")] = '\0';
		printf("fgets: %s\n", input);
		if (!strcmp(input, "exit")) {
			break;
		}
		printf("running program");
		run_program(input);
		printf("finished running program");
		fgets(read_line, sizeof(read_line), stdin);
		printf("read_line: %s\n", read_line);
		printf("=> ");
	}
}
