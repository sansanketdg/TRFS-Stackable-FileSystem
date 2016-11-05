#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "record.h"


typedef enum { false, true } bool;

void printRecord(struct trfs_record *samplerecord){
	printf("---------------------------------------------\n");
	printf("Record_id : %d\n", samplerecord->record_id);
	printf("Record_type : %d\n", (int)samplerecord->record_type);
	printf("open_flags : %d\n", samplerecord->open_flags);
	printf("permission_mode : %d\n", samplerecord->permission_mode);
	printf("pathname_size : %d\n", samplerecord->pathname_size);
	printf("pathname : %s\n", samplerecord->pathname);
	printf("buffer size : %d\n", samplerecord->size);
	printf("return_value : %d\n", samplerecord->return_value);
	printf("file_address : %d\n", samplerecord->file_address);
	switch((int)samplerecord->record_type){
		case READ_TR:
			printf("System call to be executed - read(2)\n");
			break;
		case OPEN_TR:
			printf("System call to be executed - open(2)\n");
			break;
		case WRITE_TR:
			printf("System call to be executed - write(2)\n");
			break;
		case CLOSE_TR:
			printf("System call to be executed - close(2)\n");
			break;
	}
}

int main(int argc, char *argv[]){

	int opt;
	bool  n_flag = false;
	bool s_flag = false;

	int read_ret;
	FILE *fileptr;
	//char *buffer;
	long filelen;
	char *buff = NULL;
	int rc=-1;
	size_t bytesRead = 0;
	struct trfs_record *samplerecord;

	int mapping_count = 0;
	int address, fd;
	int **mapping_arr = NULL;
	int i;

	while((opt = getopt(argc, argv, "ns")) != -1){
		switch(opt){
			case 'n':
				n_flag = true;
				break;
			case 's':
				s_flag = true;
				break;
		}
	}
	if(s_flag)
		printf("s flag is set\n");
	if(n_flag)
		printf("n flag is set\n");

	if(s_flag && n_flag){
		printf("Choose either of s or n flags....Both can't be set together\n");
		return 0;
	}

	printf("TFILE is %s\n", argv[optind]);
	
	
	fileptr = fopen(argv[optind], "rb");  // Open the file in binary mode
	if(fileptr == NULL){
		printf("Coudn't open the trace file...Give valid path\n");
		return 0;
	}
	
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
		
		if(buff)
			free(buff);

		printRecord(samplerecord);
		//if(n_flag){
		//	printRecord(samplerecord);
		//}
		//else{
		if(!n_flag){
			switch((int)samplerecord->record_type){
				case READ_TR:
					//call read_syscall
					buff = malloc(samplerecord->size);
					
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord->file_address)
							rc = mapping_arr[i][1];
					}
					
					if(rc != -1 ){
						printf("Reading the file\n");
						read_ret = read(rc, buff, samplerecord->size);
						printf("old read-return value is %d\n", samplerecord->return_value);
						printf("new read-return value is %d\n", read_ret);
						if(s_flag == true){
							if(read_ret != samplerecord->return_value){
								printf("Aborting the replay program...Deviation occured\n");
								if(buff)
									free(buff);
								goto strict_clean_label;
							}
						}
						printf("No of bytes read %d\n Should be equal to count %d\n", read_ret, samplerecord->size);
					}
					else if(s_flag == true){
						printf("Aborting the replay program...Deviation occured\n");
						if(buff)
							free(buff);
						goto strict_clean_label;
					}

					if(buff)
						free(buff);
					break;

				case OPEN_TR:
						printf("Opening the file\n");
						rc = open(samplerecord->pathname, samplerecord->open_flags, samplerecord->permission_mode);
						printf("Old file descriptor value is %d\n", samplerecord->return_value);
						printf("New file descriptor value is %d\n", rc);
						if(s_flag == true){
							if(rc < 0){
								//printf("old return value is %d\n", samplerecord->return_value);
								//printf("new return value is %d\n", read_ret);
								printf("Aborting the replay program...Deviation occured\n");
								goto strict_clean_label;
							}
						}
					//Create a row entry in mapping table
						mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
						mapping_arr[mapping_count] = (int *)malloc(2*sizeof(int));
					//Fill the row with address and corresponding fd
						mapping_arr[mapping_count][0] = samplerecord->file_address;
						mapping_arr[mapping_count][1] = rc;
						mapping_count++;
						printf("Mapping table entries count %d\n", mapping_count);	
					break;

				case WRITE_TR:
					buff = malloc(samplerecord->size);
					
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord->file_address)
							rc = mapping_arr[i][1];
					}
					if(rc != -1){
						strcpy(buff, samplerecord->wr_buff);
						printf("Writing in the file\n");
						read_ret = write(rc, buff, samplerecord->size);
						printf("old write-return value is %d\n", samplerecord->return_value);
						printf("new write-return value is %d\n", read_ret);
						if(s_flag == true){
							if(read_ret != samplerecord->return_value){
								printf("Aborting the replay program...Deviation occured\n");
								if(buff)
									free(buff);
								if(samplerecord->wr_buff)
									free(samplerecord->wr_buff);
								goto strict_clean_label;
							}
						}
						printf("No of bytes written %d\n Should be equal to count %d\n", read_ret, samplerecord->size);
					
					}
					else if(s_flag == true){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord->wr_buff)
							free(samplerecord->wr_buff);
						if(buff)
							free(buff);
						goto strict_clean_label;
					}
					
					if(buff)
						free(buff);
					if(samplerecord->wr_buff)
						free(samplerecord->wr_buff);
					break;

				case CLOSE_TR:
		      
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord->file_address)
							rc = mapping_arr[i][1];
							mapping_arr[i][0] = 0;
					}
					if(rc != -1){
						printf("Closing the file\n");
						close(rc);	
					}
					else if(s_flag == true){
						printf("Aborting the replay program...Deviation occured\n");
						goto strict_clean_label;
					}
					break;

				default:
					printf("Invalid record_type\n");
			}

			if(samplerecord){
				//printf("inside samplerecord\n");
				if(samplerecord->pathname){
					//printf("inside sample record -> pathname\n");
					free(samplerecord->pathname);
				}
				free(samplerecord);
			}
		}

	}
	goto normal_clean_label;
	
strict_clean_label:
	//printf("Inside fuckin strict\n");
	if(samplerecord){
		if(samplerecord->pathname)
			free(samplerecord->pathname);
		free(samplerecord);
	}
normal_clean_label:
	if(mapping_arr){
		for (i = 0; i < mapping_count; i++){
			if(mapping_arr[i]){
         		free(mapping_arr[i]);
			}
		}
    	free(mapping_arr);
	}

	fclose(fileptr); // Close the file
	
	return 0;
}
