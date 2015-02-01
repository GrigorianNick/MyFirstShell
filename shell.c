#include <stdio.h>
#include <string.h>

int main() {
	// Adjustable input line length
	int input_line_length = 100;
	char input[input_line_length]; // 100 as per shell specs. This can be upped if necessary.
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
			printf("input: %s", input);
		}
	} while (strcmp(input, "exit\n")  != 0); // Exit command
	return 0;
}
