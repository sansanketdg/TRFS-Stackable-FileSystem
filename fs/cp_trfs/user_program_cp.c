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

typedef enum { false, true } bool;

void printRecord(struct trfs_record *samplerecord){
	printf("Record_id is %d\n", samplerecord->record_id);
	printf("Record_type is %d\n", (int)samplerecord->record_type);
	printf("open_flags is %d\n", samplerecord->open_flags);
	printf("permission_mode is %d\n", samplerecord->permission_mode);
	printf("pathname_size is %d\n", samplerecord->pathname_size);
	printf("Pathname is %s\n", samplerecord->pathname);
	printf("size is %d\n", samplerecord->size);
	printf("return_value is %d\n", samplerecord->return_value);
	printf("file_address is %d\n", samplerecord->file_address);
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

	printf("TFILE is %s\n", argv[optind]);
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
	
	fileptr = fopen(argv[optind], "rb");  // Open the file in binary mode
	if(fileptr == NULL){
		printf("Coudn't open the trace file...Give valid path\n");
		return 0;
	}
	//fileptr = fopen("/usr/src/logfile.txt", "rb");  // Open the file in binary mode
	//fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	//filelen = ftell(fileptr);             // Get the current byte offset in the file
	//rewind(fileptr);                      // Jump back to the beginning of the file

	printf("Opened the file...reading first byte size\n");
	int loopc = 0;
	unsigned short record_size;
	while(bytesRead = fread(&record_size, sizeof(short), 1, fileptr) > 0){

		loopc++;
		if(loopc >100){
			break;
		}
		printf("First record size is %d\n", record_size);
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
			printf("Mallocing for write buffer\n");
			samplerecord->wr_buff = malloc(samplerecord->size);
        	memcpy((void *)samplerecord->wr_buff, (void *)(buff + buffer_offset), samplerecord->size);
        	buffer_offset = buffer_offset + samplerecord->size;
		}

		memcpy((void *)&samplerecord->return_value, (void *)(buff + buffer_offset), sizeof(int));
		buffer_offset = buffer_offset + sizeof(int);

		memcpy((void *)&samplerecord->file_address, (void *)(buff + buffer_offset), sizeof(unsigned long long));
		buffer_offset = buffer_offset + sizeof(unsigned long long);
		
		if(buff != NULL)
			free(buff);
		buff = NULL;

		if(n_flag){
			printf("Record_id is %d\n", samplerecord->record_id);
			printf("Record_type is %d\n", (int)samplerecord->record_type);
			printf("open_flags is %d\n", samplerecord->open_flags);
			printf("permission_mode is %d\n", samplerecord->permission_mode);
			printf("pathname_size is %d\n", samplerecord->pathname_size);
			printf("Pathname is %s\n", samplerecord->pathname);
			printf("size is %d\n", samplerecord->size);
			printf("return_value is %d\n", samplerecord->return_value);
			printf("file_address is %d\n", samplerecord->file_address);
		}
		else{
			switch((int)samplerecord->record_type){
				case READ_TR:
					//call read_syscall
					buff = malloc(samplerecord->size);
					//rc = open(samplerecord->pathname, O_CREAT | O_RDONLY, 0644);
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord->file_address)
							rc = mapping_arr[i][1];
					}
					
					if(rc != -1 ){
						// if(n_flag == true){
						// 	printRecord(samplerecord);
						// }
						// else{
							read_ret = read(rc, buff, samplerecord->size);
							printf("No of bytes read %d\n Should be equal to count %d\n", read_ret, samplerecord->size);
						//}
					}
					else if(s_flag == true){
						printf("Aborting the replay program...Deviation occured\n");
						goto strict_clean_label;
					}
					
					//close(rc);
					break;
				case OPEN_TR:
					// if(n_flag == true){
					// 	printRecord(samplerecord);
					// 		mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
					// 		mapping_arr[mapping_count] = (int *)malloc(2*sizeof(int));
					// 	//Fill the row with address and corresponding fd
					// 		mapping_arr[mapping_count][0] = samplerecord->file_address;
					// 		mapping_arr[mapping_count][1] = 0;
					// 		mapping_count++;
					// }
					// else{
						rc = open(samplerecord->pathname, samplerecord->open_flags, samplerecord->permission_mode);
					//Create a row entry in mapping table
						mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
						mapping_arr[mapping_count] = (int *)malloc(2*sizeof(int));
					//Fill the row with address and corresponding fd
						mapping_arr[mapping_count][0] = samplerecord->file_address;
						mapping_arr[mapping_count][1] = rc;
						mapping_count++;
						printf("Mapping table entries count %d\n", mapping_count);	
					//}
					
					//close(rc);
					break;

				case WRITE_TR:
					buff = malloc(samplerecord->size);
					//rc = open(samplerecord->pathname, O_CREAT|O_APPEND|O_WRONLY, 0644);
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord->file_address)
							rc = mapping_arr[i][1];
					}
					if(rc != -1){
						// if(n_flag == true){
						// 	printRecord(samplerecord);
						// }
						// else{
							strcpy(buff, samplerecord->wr_buff);
							read_ret = write(rc, buff, samplerecord->size);
							printf("No of bytes written %d\n Should be equal to count %d\n", read_ret, samplerecord->size);
						//}
						
					}
					else if(s_flag == true){
						printf("Aborting the replay program...Deviation occured\n");
						goto strict_clean_label;
					}
					
					//close(rc);
					break;

				case CLOSE_TR:
		            //rc = open(samplerecord->pathname, O_CREAT, 0644);
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord->file_address)
							rc = mapping_arr[i][1];
							mapping_arr[i][0] = 0;
					}
					if(rc != -1){
						// if(n_flag == true){
						// 	printRecord(samplerecord);
						// }
						// else{
							printf("Closing the file\n");
							close(rc);	
						//}
					}
					else if(s_flag == true){
						printf("Aborting the replay program...Deviation occured\n");
						goto strict_clean_label;
					}
					
					//Delete this mapping entry from table
					// if(mapping_arr[mapping_count]){
					// 	free();
					// }
					break;

				default:
					printf("Invalid record_type\n");
			}

			//printf("Freeing the memory\n");
			if(samplerecord != NULL){
				printf("inside samplerecord\n");
				if(samplerecord->pathname != NULL){
					printf("inside sample record -> pathname\n");
					free(samplerecord->pathname);
					samplerecord->pathname = NULL;
				}
				if(samplerecord->wr_buff != NULL){
					printf("inside sample record -> wr_buff\n");
					free(samplerecord->wr_buff);
					samplerecord->wr_buff = NULL;
					printf("Successfully fuckn clearned wr_buff\n");
				}
				free(samplerecord);
				samplerecord = NULL;
			}
			if(buff != NULL){
				printf("inside buff\n");
				free(buff);
				buff = NULL;
			}
		}

		// switch((int)samplerecord->record_type){
		// 	case READ_TR:
		// 		//call read_syscall
		// 		buff = malloc(samplerecord->size);
		// 		//rc = open(samplerecord->pathname, O_CREAT | O_RDONLY, 0644);
		// 		for(i = 0; i < mapping_count; i++){
		// 			if(mapping_arr[i][0] == samplerecord->file_address)
		// 				rc = mapping_arr[i][1];
		// 		}
				
		// 		if(rc != -1 ){
		// 			if(n_flag == true){
		// 				printRecord(samplerecord);
		// 			}
		// 			else{
		// 				read_ret = read(rc, buff, samplerecord->size);
		// 				printf("No of bytes read %d\n Should be equal to count %d\n", read_ret, samplerecord->size);
		// 			}
		// 		}
		// 		else if(s_flag == true){
		// 			printf("Aborting the replay program...Deviation occured\n");
		// 			goto strict_clean_label;
		// 		}
				
		// 		//close(rc);
		// 		break;
		// 	case OPEN_TR:
		// 		if(n_flag == true){
		// 			printRecord(samplerecord);
		// 				mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
		// 				mapping_arr[mapping_count] = (int *)malloc(2*sizeof(int));
		// 			//Fill the row with address and corresponding fd
		// 				mapping_arr[mapping_count][0] = samplerecord->file_address;
		// 				mapping_arr[mapping_count][1] = 0;
		// 				mapping_count++;
		// 		}
		// 		else{
		// 			rc = open(samplerecord->pathname, samplerecord->open_flags, samplerecord->permission_mode);
		// 		//Create a row entry in mapping table
		// 			mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
		// 			mapping_arr[mapping_count] = (int *)malloc(2*sizeof(int));
		// 		//Fill the row with address and corresponding fd
		// 			mapping_arr[mapping_count][0] = samplerecord->file_address;
		// 			mapping_arr[mapping_count][1] = rc;
		// 			mapping_count++;
		// 			printf("Mapping table entries count %d\n", mapping_count);	
		// 		}
				
		// 		//close(rc);
		// 		break;

		// 	case WRITE_TR:
		// 		buff = malloc(samplerecord->size);
		// 		//rc = open(samplerecord->pathname, O_CREAT|O_APPEND|O_WRONLY, 0644);
		// 		for(i = 0; i < mapping_count; i++){
		// 			if(mapping_arr[i][0] == samplerecord->file_address)
		// 				rc = mapping_arr[i][1];
		// 		}
		// 		if(rc != -1){
		// 			if(n_flag == true){
		// 				printRecord(samplerecord);
		// 			}
		// 			else{
		// 				strcpy(buff, samplerecord->wr_buff);
		// 				read_ret = write(rc, buff, samplerecord->size);
		// 				printf("No of bytes written %d\n Should be equal to count %d\n", read_ret, samplerecord->size);
		// 			}
					
		// 		}
		// 		else if(s_flag == true){
		// 			printf("Aborting the replay program...Deviation occured\n");
		// 			goto strict_clean_label;
		// 		}
				
		// 		//close(rc);
		// 		break;

		// 	case CLOSE_TR:
	 //            //rc = open(samplerecord->pathname, O_CREAT, 0644);
		// 		for(i = 0; i < mapping_count; i++){
		// 			if(mapping_arr[i][0] == samplerecord->file_address)
		// 				rc = mapping_arr[i][1];
		// 				mapping_arr[i][0] = 0;
		// 		}
		// 		if(rc != -1){
		// 			if(n_flag == true){
		// 				printRecord(samplerecord);
		// 			}
		// 			else{
		// 				printf("Closing the file\n");
		// 				close(rc);	
		// 			}
		// 		}
		// 		else if(s_flag == true){
		// 			printf("Aborting the replay program...Deviation occured\n");
		// 			goto strict_clean_label;
		// 		}
				
		// 		//Delete this mapping entry from table
		// 		// if(mapping_arr[mapping_count]){
		// 		// 	free();
		// 		// }
		// 		break;

		// 	default:
		// 		printf("Invalid record_type\n");
		// }

		// //printf("Freeing the memory\n");
		// if(samplerecord){
		// 	printf("inside samplerecord\n");
		// 	if(samplerecord->pathname){
		// 		printf("inside sample record -> pathname\n");
		// 		free(samplerecord->pathname);
		// 		samplerecord->pathname = NULL;
		// 	}
		// 	if(samplerecord->wr_buff){
		// 		printf("inside sample record -> wr_buff\n");
		// 		free(samplerecord->wr_buff);
		// 		samplerecord->wr_buff = NULL;
		// 	}
		// 	free(samplerecord);
		// 	samplerecord = NULL;
		// }
		// if(buff){
		// 	printf("inside buff\n");
		// 	free(buff);
		// 	buff = NULL;
		// }
	}
	goto normal_clean_label;
	
strict_clean_label:
	printf("Inside fuckin strict\n");
	if(samplerecord != NULL){
		if(samplerecord->pathname != NULL)
			free(samplerecord->pathname);
		if(samplerecord->wr_buff != NULL)
			free(samplerecord->wr_buff);
		free(samplerecord);
	}
	if(buff != NULL)
		free(buff);
normal_clean_label:
	if(mapping_arr != NULL){
		printf("Freeing table\n");
		for (i = 0; i < mapping_count; i++){
			if(mapping_arr[i]!= NULL){
				printf("Freeing %d elemet\n", i);
         		free(mapping_arr[i]);
			}
		}
    	free(mapping_arr);
	}

	fclose(fileptr); // Close the file
	
	return 0;
}
