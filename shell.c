#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	// Adjustable input line length
	int input_line_length = 100;
	char input[input_line_length]; // 100 as per shell specs. This can be upped if necessary.
	char **arguments = NULL; // My super janky vector
	// Main shell loop
	do {
		// Read in a line.
		fgets(input, input_line_length, stdin); // Remember, it also reads in \n
		input[strcspn(input, "\n")] = '\0'; // Stripping the \n, since it does funky things to execv
		// Enforcing input length limit
		if (strlen(input) > input_line_length) {
			printf("Error: Input is too long.\n");
			int c;
			// If the line length is too long, chew through the excess sitting around in stdin
			while ((c = fgetc(stdin)) != EOF && c != '\n');
		}
		else {
			int word_size = 0;
			char *tok;
			tok = strtok(input, " ");
			while (tok) {
				// We've found our word
				if (!strcmp(tok, "|")  || !strcmp(tok, ">") || !strcmp(tok, "<")) {
					printf("pipe!\n");
					// 1. Run exec
					// 2. Clear arguments
					// 3. pipe exec's output back into arguments
					// 4. Carry on
				}
				// Add another element to arguments
				word_size++;
				arguments = (char **)realloc(arguments, sizeof(char*) * (word_size));
				// Adding tok to the end of arguments
				arguments[word_size - 1] = (char *)malloc(sizeof(char) * strlen(tok));
				strcpy(arguments[word_size - 1], tok);
				// Finding next token
				tok = strtok(NULL, " ");
			}
			execv(arguments[0], arguments);
			printf("tried it\n");
			if (arguments) {
				free(arguments);
				arguments = NULL;
			}
		}
	} while (strcmp(input, "exit\n")); // Exit command
	return 0;
}
