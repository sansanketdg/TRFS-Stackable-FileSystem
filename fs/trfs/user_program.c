#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record.h"
// struct trfs_record
// {
// 	int record_id;
// 	unsigned short record_size;
// 	unsigned char record_type;
// 	int open_flags;
// 	int permission_mode;
// 	short pathname_size;
// 	char *pathname;
// 	int return_value;

// 	unsigned char mybitmap;
// };

void main(){
	FILE *fileptr;
	char *buffer;
	long filelen;
	struct trfs_record *samplerecord;
	fileptr = fopen("/usr/src/logfile.txt", "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	filelen = ftell(fileptr);             // Get the current byte offset in the file
	rewind(fileptr);                      // Jump back to the beginning of the file
	samplerecord=(struct trfs_record*)malloc(sizeof(struct trfs_record));
	buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
	fread(buffer, filelen, 1, fileptr); // Read in the entire file
	//printf("%s\n", buffer);
	
	int record_id = -1;
	int buffer_offset = 0;
	memcpy((void *)&samplerecord->record_id,(void *)(buffer + buffer_offset),  sizeof(int));
	buffer_offset = buffer_offset + sizeof(int);

	memcpy((void *)&samplerecord->record_size, (void *)(buffer + buffer_offset), sizeof(short));
	buffer_offset = buffer_offset + sizeof(short);

	memcpy((void *)&samplerecord->record_type, (void *)(buffer + buffer_offset), sizeof(char));
	buffer_offset = buffer_offset + sizeof(char);

	memcpy((void *)&samplerecord->open_flags, (void *)(buffer + buffer_offset), sizeof(int));
	buffer_offset = buffer_offset + sizeof(int);

	memcpy((void *)&samplerecord->permission_mode, (void *)(buffer + buffer_offset), sizeof(int));
	buffer_offset = buffer_offset + sizeof(int);

	memcpy((void *)&samplerecord->pathname_size, (void *)(buffer + buffer_offset), sizeof(short));
	buffer_offset = buffer_offset + sizeof(short);

	//char pathname[samplerecord->pathname_size + 1];
	samplerecord->pathname = malloc(samplerecord->pathname_size + 1);
	memcpy((void *)samplerecord->pathname, (void *)(buffer + buffer_offset), samplerecord->pathname_size);
	buffer_offset = buffer_offset + samplerecord->pathname_size;
	samplerecord->pathname[samplerecord->pathname_size] = '\0';

	memcpy((void *)&samplerecord->return_value, (void *)(buffer + buffer_offset), sizeof(int));
	buffer_offset = buffer_offset + sizeof(int);

	memcpy((void *)&samplerecord->mybitmap, (void *)(buffer + buffer_offset), sizeof(char));
	buffer_offset = buffer_offset + sizeof(char);
	
	
	printf("Record_id is %d\n", samplerecord->record_id);
	printf("Record_size is %d\n", samplerecord->record_size);
	printf("Record_type is %c\n", samplerecord->record_type);
	printf("open_flags is %d\n", samplerecord->open_flags);
	printf("permission_mode is %d\n", samplerecord->permission_mode);
	printf("pathname_size is %d\n", samplerecord->pathname_size);
	printf("Pathname is %s\n", samplerecord->pathname);
	printf("return_value is %d\n", samplerecord->return_value);
	printf("mybitmap is %d\n", samplerecord->mybitmap);


	fclose(fileptr); // Close the file
	free(samplerecord->pathname);
	free(samplerecord);
	free(buffer);
}
