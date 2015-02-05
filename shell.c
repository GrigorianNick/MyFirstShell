/*

My Simple Shell

Author: Nicholas Grigorian
Computing ID: ngg3vm

The following code implements a simple shell, as specified in the hw1 assignment.

To compile: `make`

Modifications:
	Feb 1 - Created project
	Feb 2 - Added tokenization
	Feb 3 - Added execv functionality
	Feb 4/5 - Added piping functionality
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	// Adjustable input line length
	size_t input_line_length = 100;
	ssize_t read;
	char *input;
	char **arguments = NULL; // My super janky vector
	char **append = NULL; // A vector for piping
	char cwd[1024]; // Current working directory
	char buffer[1024]; // Used for catching execv's output
	int status = 0;
	if (!getcwd(cwd, sizeof(cwd))) {
		perror( "Unable to get CWD!" );
		exit(-1);
	}
	strcat(cwd, "/");
	// Main shell loop
	while ((read = getline(&input, &input_line_length, stdin)) != -1) {
		input[strcspn(input, "\n")] = '\0'; // Stripping the \n, since it does funky things to execv
		// Enforcing input length limit
		if (strlen(input) > input_line_length) {
			printf("Error: Input is too long.\n");
			int c;
			// If the line length is too long, chew through the excess sitting around in stdin
			while ((c = fgetc(stdin)) != EOF && c != '\n');
		}
		else {
			// Number of args in current executable
			int word_size = 0;
			int append_size = 0;
			char *tok;
			tok = strtok(input, " ");
			while (tok) {
				if (!strcmp(tok, "exit")) {
					if (append) {
						free(append);
					}
					if (arguments) {
						free(arguments);
					}
					return(0);
				}
				// We've found our word
				else if (!strcmp(tok, "|")  || !strcmp(tok, ">") || !strcmp(tok, "<")) {
					// Reading stdin from a file
					if (!strcmp(tok, "<")) {
						FILE * source = fopen(arguments[0], "r");
						char buff[1024]; // Buffer to hold file contents
						if (source) {
							fread(buff, 1, sizeof(buff), source);
							char * pre_tok; // Token to parse the input file
							pre_tok = strtok(buff, " ");
							// Break file into tokens
							while (pre_tok) {
								append_size++;
								append = realloc(append, sizeof(char*) * append_size);
								append[append_size - 1] = malloc(sizeof(char*) * strlen(pre_tok));
								strcpy(append[append_size - 1], pre_tok);
								pre_tok = strtok(NULL, " ");
							}
							fclose(source);
							// We're done with the arguments
							word_size = 0;
							free(arguments);
							arguments = NULL;
							//tok = strtok(NULL, " ");
							//continue;
						}
					}
					
					else if (!strcmp(tok, ">")) {
						tok = strtok(NULL, " ");
						char target[1024];
						char buff[1024];
						if (tok[0] != '/') {
							strcpy(target, cwd);
							strcat(target, "/");
							strcat(target, tok);
						}
						else {
							strcpy(target, tok);
						}
						pid_t pid;
						int Pipe[2];
						// Check for error during pipe creation
						if (pipe(Pipe) == -1) {
							perror( "Pipe error" );
							exit(-1);
						}
						//Check for error during forking
						pid = fork();
						if (pid == -1) {
							perror( "Fork error" );
							exit(-1);
						}
						else if (pid ==0) { // Child
							close(Pipe[0]);
							dup2(Pipe[1], 1);
							arguments = realloc(arguments, sizeof(char*) * word_size + 1);
							arguments[word_size] = NULL;
							execv(arguments[0], arguments);
							perror( "execv error" );
							exit(-1);
						}
						else if (pid != 0) { // Parent
							close(Pipe[1]);
							dup2(Pipe[0], 0);
							wait(&status);
							printf("child ended with status %i\n", status);
							fgets(buff, sizeof(buff), stdin);
							free(arguments);
							arguments = NULL;
						}
						FILE * destination = fopen(target, "w");
						if (destination) {
							fwrite(buff, 1, sizeof(char) * strlen(buff), destination);
							fclose(destination);
						}
						break;
					}

					pid_t pid;
					int Pipe[2];
					// Check for error during pipe creation
					if (pipe(Pipe) == -1) {
						perror( "Pipe error" );
						exit(-1);
					}

					// Check for error during forking
					pid = fork();
					if (pid == -1) {
						perror( "Fork error" );
						exit(-1);
					}
					else if (pid == 0) { // Child
						close(Pipe[0]); // Closing read pipe
						dup2(Pipe[1], 1); // Dup2'ing stdout into the write pipe
						if (append) {
							// Appending the append to arguments
							int i;
							for (i = 0; i < append_size; i++) {
								word_size++;
								arguments = realloc(arguments, sizeof(char*) * word_size);
								arguments[word_size - 1] = malloc(strlen(append[i]));
								strcpy(arguments[word_size -1], append[i]);
							}
							for (i = 0; i < word_size; i++) {
								printf("voodoo: %s\n", arguments[i]);
							}
							arguments = realloc(arguments, sizeof(char*) * (word_size + 1));
							arguments[word_size] = NULL; // NULL terminating the list
							free(append); // Need to clear out append
							append = NULL;
							append_size = 0;
						}
						else {
							arguments = realloc(arguments, sizeof(char*) * (word_size + 1));
							arguments[word_size] = NULL;
						}
						execv(arguments[0], arguments);
						perror( "execv error" );
						exit(-1);
					}
					else if (pid != 0) { // Parent
						close(Pipe[1]); // Closing write pipe
						dup2(Pipe[0], 0); // Dup2'ing stdin into the read pipe
						wait(&status);
						printf("Child ended with status %i\n", status);
						fgets(buffer, sizeof(buffer), stdin); // Grab executable's output
						printf("output: %s\n", buffer);
						word_size = 0;
						free(arguments);
						arguments = NULL;
						//break;
					}

				}
				else {
					// Add another element to arguments
					word_size++;
					arguments = (char **)realloc(arguments, sizeof(char*) * (word_size));
					// Detect if we need relative pathing except for exit
					if (tok[0] != '/' && strcmp(tok, "exit") && tok[0] != '-') {
						arguments[word_size - 1] = (char *)malloc((sizeof(char) * strlen(tok)) + (sizeof(char) * strlen(cwd)));
						strcpy(arguments[word_size - 1], cwd);
						strcat(arguments[word_size - 1], tok);
					}
					else {
						// Adding tok to the end of arguments
						arguments[word_size - 1] = (char *)malloc(sizeof(char) * strlen(tok));
						strcpy(arguments[word_size - 1], tok);
					}
				}
				// Finding next token
				tok = strtok(NULL, " ");
			}
			if (arguments) {
				pid_t pid;
				int Pipe[2];
				if (pipe(Pipe) == -1) {
					perror( "Pipe error" );
					exit(-1);
				}

				pid = fork();
				if (pid == -1) {
					perror( "Fork error" );
					exit(-1);
				}
				else if (pid == 0) { // Child
					close(Pipe[0]);
					dup2(Pipe[1], 1);
					// if (append) goes here
					arguments = realloc(arguments, word_size + 1);
					arguments[word_size] = NULL;
					execv(arguments[0], arguments);
					perror( "execv error" );
					exit(-1);
				}
				else if (pid != 0) { // Parent
					close(Pipe[1]);
					dup2(Pipe[0], 0);
					wait(&status);
					printf("Child ended with status %i\n", status);
					fgets(buffer, sizeof(buffer), stdin);
					printf("%s", buffer);
					free(arguments);
					arguments = NULL;
				}
			}
		}
	}
	return 0;
}
