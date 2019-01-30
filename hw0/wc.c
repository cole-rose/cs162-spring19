#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

void countLetters(FILE *input_file, char *file_name){
	int lines_count = 0;
	int words_count = 0;
	int characters_count = 0;
	bool is_whitespace = true;
	char c;
	char last_char = ' ';

	// read file
	while ((c = fgetc(input_file)) != EOF)  {
		if (c == '\n'){
			lines_count += 1;
			is_whitespace = true;
		} else if (c == ' ' || c == '\t' || c == '\r') {
			is_whitespace = true;
		} else if (c == '\x0'){
			// special case. Do nothing
			;
		}
		// ecounter a printable character (not a whitespace)
		else {
			if (is_whitespace){
				words_count += 1;
			}
			is_whitespace = false;
		}
		last_char = c;
		characters_count += 1;
	}
	// If input is standard input
	if (file_name == "stdin"){
		printf (" %d   %d   %d\n", lines_count, words_count, characters_count);
	} else{
		printf (" %d  %d %d %s\n", lines_count, words_count, characters_count, file_name);	
	}
}

int main(int argc, char *argv[]) {
    if (argc == 2){
    	FILE *file = fopen(argv[1], "r");
    	char *file_name = argv[1];
    	//if file open successfully
	    if (file){
			countLetters(file, file_name);
	    } else{
	    	printf("File %s failed to open.\n", file_name);
	    }
	    fclose(file);
    } 
    // No argument pass in. Get stdin input from keyboard
    else if (argc == 1){
    	// call count function with stdin as input file
    	countLetters(stdin, "stdin");
    }
    return 0;
}
