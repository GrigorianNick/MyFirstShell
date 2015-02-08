#include <stdio.h> // Needed for stdin/stdout
#include <string.h> // Needed for string manipulations, esp stktok
#include <stdlib.h> // Needed for forking and pipe support

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
	
	// Build the execv and run_program arguments
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
		// substitute the "< file" with the file's contents
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
		// Since this command is substituted adn appended to the previous one, act as if we were moving onto the next command.
		if (last_pipe != '<') {
			strcat(curr, tok);
			strcat(curr, " ");
		}
		tok = strtok(NULL, " ");
	}

	

	char ** args = NULL;
	int args_size = 0;
	// Building args for execv
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
			// The last pipe was >, so we're writing to an output file and then quitting.
			else if (last_pipe == '>') {
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
				exit(-1);
			}
			else {
				execv(args[0], args);
				perror( "Execv failed!" ); // We should never see this.
				exit(-1);
			}
		}
		else {
			perror( "Child status" );
			exit(-1);
		}
	}
}

int main() {
	char input[1024];
	char read_line[1024];
	int orig_stdin = dup(0);
	int orig_stdout = dup(1);
	int Pipe[2];
	int status;
	pid_t pid;
	// Adding some pazzaz
	printf("=> ");
	// Main loop
	while(1) {
		// In case fgets fails, we leave
		if (fgets(input, sizeof(input), stdin) == NULL) {
			break;
		}
		input[strcspn(input, "\n")] = '\0'; // Stripping newline char from fgets
		// Special exit command is processed by us, not the system.
		if (!strcmp(input, "exit")) {
			break;
		}
		// Need to fork, otherwise the execv will force us to quit after a single round.
		pid = fork();
		if (pid == 0) { //Child
			run_program(input);
			pipe(Pipe);
			close(0);
			dup2(Pipe[1], 1);
		}
		else { // Parent
			wait(&status); // Wait on child to avoid race conditions.
			printf("=> ");
		}
	}
}
