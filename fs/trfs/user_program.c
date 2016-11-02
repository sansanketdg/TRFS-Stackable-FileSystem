#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct trfs_record
{
int record_id;
};

void main(){
	FILE *fileptr;
	char *buffer;
	long filelen;
	struct trfs_record *samplerecord;
	fileptr = fopen("/usr/src/logfile1.txt", "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	filelen = ftell(fileptr);             // Get the current byte offset in the file
	rewind(fileptr);                      // Jump back to the beginning of the file
	samplerecord=(struct trfs_record*)malloc(sizeof(struct trfs_record));
	buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
	fread(buffer, filelen, 1, fileptr); // Read in the entire file
	printf("%s\n", buffer);
	
	int record_id = -1;
	int buffer_offset = 0;
	memcpy((void *)&samplerecord->record_id,(void *)(buffer + buffer_offset),  sizeof(int));

	
	
	printf("Record_id is %d\n", samplerecord->record_id);
	
	fclose(fileptr); // Close the file
	free(samplerecord);
	free(buffer);
}
