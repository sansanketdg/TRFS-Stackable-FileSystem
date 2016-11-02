#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(){
	FILE *fileptr;
	char *buffer;
	long filelen;

	fileptr = fopen("/usr/src/logfile.txt", "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	filelen = ftell(fileptr);             // Get the current byte offset in the file
	rewind(fileptr);                      // Jump back to the beginning of the file

	buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
	fread(buffer, filelen, 1, fileptr); // Read in the entire file
	int i = 9999;	
	memcpy((void *)&i, (void *)buffer, sizeof(int));
	printf("%d\n", i);
	fclose(fileptr); // Close the file
}
