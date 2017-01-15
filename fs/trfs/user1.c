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
	printf("%s\n", buffer);
	
	int buffer_offset = 0;
	int record_id = -1;
	unsigned short record_size = 0;
	unsigned char record_type = 'a';
	int open_flags = -1;
	int permission_mode = -1;
	short pathname_size = 0;
	int return_value = -1;
	
	memcpy((void *)&record_id, (void *)(&buffer + buffer_offset), sizeof(int));
	buffer_offset = buffer_offset + sizeof(int);

	// memcpy((void *)&record_size, (void *)(&buffer + buffer_offset), sizeof(short));
	// buffer_offset = buffer_offset + sizeof(short);

	// memcpy((void *)&record_type, (void *)(&buffer + buffer_offset), sizeof(char));
	// buffer_offset = buffer_offset + sizeof(char);

	// memcpy((void *)&open_flags, (void *)(&buffer + buffer_offset), sizeof(int));
	// buffer_offset = buffer_offset + sizeof(int);

	// memcpy((void *)&permission_mode, (void *)(&buffer + buffer_offset), sizeof(int));
	// buffer_offset = buffer_offset + sizeof(int);

	// memcpy((void *)&pathname_size, (void *)(&buffer + buffer_offset), sizeof(short));
	// buffer_offset = buffer_offset + sizeof(short);

	// char pathname[pathname_size + 1];
	// memcpy((void *)&pathname[0], (void *)(&buffer + buffer_offset), pathname_size);
	// buffer_offset = buffer_offset + pathname_size;
	// pathname[pathname_size] = '\0';

	// memcpy((void *)&return_value, (void *)(&buffer + buffer_offset), sizeof(int));
	// buffer_offset = buffer_offset + sizeof(int);

	printf("%d\n", record_id);
	//printf("%s\n", pathname);
	
	fclose(fileptr); // Close the file

}
