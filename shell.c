#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	// Adjustable input line length
	int input_line_length = 100;
	char input[input_line_length]; // 100 as per shell specs. This can be upped if necessary.
	char **arguments; // My super janky vector
	arguments = malloc(0);
	// Main shell loop
	do {
		// Read in a line.
		fgets(input, input_line_length, stdin); // Remember, it also reads in \n
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
			bool to_free = false;
			tok = strtok(input, " ");
			while (tok) {
				printf("Token: %s\n", tok);
				// We've found our word
				//if (strcmp(tok, "|") && strcmp(tok, ">") && strcmp(tok, "<")) {
				//	printf("blah\n");
				//}
				// Add another element to arguments
				arguments = realloc(arguments, sizeof(char*) * (word_size + 1));
				// Adding tok to the end of arguments
				arguments[word_size] = malloc(sizeof(char) * strlen(tok));
				to_free = true;
				strcpy(arguments[word_size], tok);
				word_size++;
				// Finding next token
				tok = strtok(NULL, " ");
			}
			if (to_free) {
				free(arguments);
			}
		}
		// Example ls. Note the final NULL argument and the duplicated binary path
		//execl("/bin/ls", "/bin/ls", "/home/nick/MyFirstShell", NULL);
	} while (strcmp(input, "exit\n")); // Exit command
	return 0;
}
