#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

	int read_ret;
	FILE *fileptr;
	//char *buffer;
	long filelen;
	char *buff = NULL;
	int rc;
	size_t bytesRead = 0;
	struct trfs_record *samplerecord;

	int count = 0;
	int address, fd;
	int **mapping_arr = 0;
	int i;

	// while(count < 10){
	// 	mapping_arr = (int **)realloc(mapping_arr, (count + 1)*sizeof(mapping_arr));
	// 	if(mapping_arr == NULL){
	// 		printf("No More memory TABLE\n");
	// 		break;
	// 	}
	// 	mapping_arr[count] = (int *)malloc(2*sizeof(int));
	// 	if(mapping_arr[count] == 0){
	// 		printf("No More memory ROW\n");
	// 	}
	// 	mapping_arr[count][0] = count;
	// 	mapping_arr[count][1] = count + 1;
	// 	count++;
	// }

	// for (i = 0; i < count; i++)
 //        printf("(%d, %d)\n", mapping_arr[i][0], mapping_arr[i][1]);

 //    for (i = 0; i < count; i++)
 //        free(mapping_arr[i]);
 //    free(mapping_arr);
	
	
	fileptr = fopen("/usr/src/logfile.txt", "rb");  // Open the file in binary mode
	//fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	//filelen = ftell(fileptr);             // Get the current byte offset in the file
	//rewind(fileptr);                      // Jump back to the beginning of the file

	int loopc = 0;
	unsigned short record_size;
	while(bytesRead = fread(&record_size, sizeof(short), 1, fileptr) > 0){

		loopc++;
		if(loopc >100){
			break;
		}

		//Read rest of this single record
		buff = (char *)malloc(record_size - sizeof(short));
		fread(buff, record_size - sizeof(short), 1, fileptr);

		samplerecord=(struct trfs_record*)malloc(sizeof(struct trfs_record));

		int buffer_offset = 0;

		memcpy((void *)&samplerecord->record_id,(void *)(buff + buffer_offset),  sizeof(int));
		buffer_offset = buffer_offset + sizeof(int);

		memcpy((void *)&samplerecord->record_type, (void *)(buff + buffer_offset), sizeof(char));
		buffer_offset = buffer_offset + sizeof(char);

		memcpy((void *)&samplerecord->open_flags, (void *)(buff + buffer_offset), sizeof(int));
		buffer_offset = buffer_offset + sizeof(int);

		memcpy((void *)&samplerecord->permission_mode, (void *)(buff + buffer_offset), sizeof(int));
		buffer_offset = buffer_offset + sizeof(int);

		memcpy((void *)&samplerecord->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
		buffer_offset = buffer_offset + sizeof(short);

		samplerecord->pathname = malloc(samplerecord->pathname_size + 1);
		memcpy((void *)samplerecord->pathname, (void *)(buff + buffer_offset), samplerecord->pathname_size);
		buffer_offset = buffer_offset + samplerecord->pathname_size;
		
		samplerecord->pathname[samplerecord->pathname_size] = '\0';

		memcpy((void *)&samplerecord->size, (void *)(buff + buffer_offset), sizeof(unsigned long long));
		buffer_offset = buffer_offset + sizeof(unsigned long long);	

		if(samplerecord->size > 0 && (int)samplerecord->record_type == WRITE_TR){
			samplerecord->wr_buff = malloc(samplerecord->size);
        	memcpy((void *)samplerecord->wr_buff, (void *)(buff + buffer_offset), samplerecord->size);
        	buffer_offset = buffer_offset + samplerecord->size;
		}

		memcpy((void *)&samplerecord->return_value, (void *)(buff + buffer_offset), sizeof(int));
		buffer_offset = buffer_offset + sizeof(int);

		memcpy((void *)&samplerecord->file_address, (void *)(buff + buffer_offset), sizeof(unsigned long long));
		buffer_offset = buffer_offset + sizeof(unsigned long long);
		
		
		
		// printf("Record_size is %d\n", samplerecord->record_size);
		printf("Record_id is %d\n", samplerecord->record_id);
		printf("Record_type is %d\n", (int)samplerecord->record_type);
		printf("open_flags is %d\n", samplerecord->open_flags);
		printf("permission_mode is %d\n", samplerecord->permission_mode);
		printf("pathname_size is %d\n", samplerecord->pathname_size);
		printf("Pathname is %s\n", samplerecord->pathname);
		printf("size is %d\n", samplerecord->size);
		printf("return_value is %d\n", samplerecord->return_value);
		printf("file_address is %d\n", samplerecord->file_address);

		free(buff);

		switch((int)samplerecord->record_type){
			case READ_TR:
				//call read_syscall
				buff = malloc(samplerecord->size);
				rc = open(samplerecord->pathname, O_CREAT | O_RDONLY, 0644);
				read_ret = read(rc, buff, samplerecord->size);
				close(rc);
				break;
			case OPEN_TR:
				rc = open(samplerecord->pathname, samplerecord->open_flags, samplerecord->permission_mode);
				//Create a row entry in mapping table
					//mapping_arr = (int **)realloc(mapping_arr, (count + 1)*sizeof(mapping_arr));
					//mapping_arr[count] = (int *)malloc(2*sizeof(int));
				//Fill the row with address and corresponding fd
					//mapping_arr[count][0] = samplerecord->file_address;
					//mapping_arr[count][1] = rc;
					//count++;
				close(rc);
				break;

			case WRITE_TR:
				buff = malloc(samplerecord->size);
				rc = open(samplerecord->pathname, O_CREAT|O_APPEND|O_WRONLY, 0644);
				strcpy(buff, samplerecord->wr_buff);
				read_ret = write(rc, buff, samplerecord->size);
				close(rc);
				break;

			case CLOSE_TR:
	            rc = open(samplerecord->pathname, O_CREAT, 0644);
				close(rc);
				break;

			default:
				printf("Invalid record_type\n");
		}

		if(samplerecord){
			if(samplerecord->pathname)
				free(samplerecord->pathname);
			if(samplerecord->wr_buff)
				free(samplerecord->wr_buff);
			free(samplerecord);
		}
		if(buff)
			free(buff);
		
	}
	
	//fread(&record_size, sizeof(short), 1, fileptr);
	//printf("%d\n", record_size);

	//buff = (char *)malloc(record_size - sizeof(short));
	//fread(buff, record_size - sizeof(short), 1, fileptr);

	//samplerecord=(struct trfs_record*)malloc(sizeof(struct trfs_record));
	// buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
	// fread(buffer, filelen, 1, fileptr); // Read in the entire file
	// //printf("%s\n", buffer);

	//int buffer_offset = 0;

	fclose(fileptr); // Close the file
	//free(buffer);
	
}
