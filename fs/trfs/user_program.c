#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "record.h"

typedef enum { false, true } bool;

int main(int argc, char *argv[]){

	int opt;
	bool  n_flag = false;
	bool s_flag = false;

	int read_ret;
	FILE *fileptr;
	long filelen;
	char *buff = NULL;
	int rc=-1;
	size_t bytesRead = 0;

	int mapping_count = 0;
	int address, fd;
	int **mapping_arr = NULL;
	//int *mapping_just_address = NULL;
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

	if(s_flag && n_flag){
		printf("Both flags 'n' and 's' can't be set together!\n");
		return 0;
	}
	
	
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

		int buffer_offset = 0;
		int record_type; 

		memcpy((void *)&record_type, (void *)(buff + buffer_offset), sizeof(int));
		buffer_offset = buffer_offset + sizeof(int);

		//printf("record type is %d\n", record_type);

		switch((int)record_type){
			struct trfs_read_record *samplerecord_read;
			struct trfs_open_record *samplerecord_open;
			struct trfs_write_record *samplerecord_write;
			struct trfs_close_record *samplerecord_close;
			struct trfs_link_record *samplerecord_link;
			struct trfs_mkdir_record *samplerecord_mkdir;
			struct trfs_unlink_record *samplerecord_unlink;
			struct trfs_rmdir_record *samplerecord_rmdir;
			struct trfs_symlink_record *samplerecord_symlink;
			struct trfs_rename_record *samplerecord_rename;
			struct trfs_readlink_record *samplerecord_readlink;

			case READ_TR:
				samplerecord_read = (struct trfs_read_record*)malloc(sizeof(struct trfs_read_record));

				memcpy((void *)&samplerecord_read->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_read->pathname = malloc(samplerecord_read->pathname_size + 2);
				samplerecord_read->pathname[0] = '.';
				memcpy((void *)samplerecord_read->pathname + 1, (void *)(buff + buffer_offset), samplerecord_read->pathname_size);
				buffer_offset = buffer_offset + samplerecord_read->pathname_size;
				samplerecord_read->pathname[samplerecord_read->pathname_size + 1] = '\0';

				memcpy((void *)&samplerecord_read->size, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_read->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_read->file_address, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_read->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_read->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("pathname_size is %d\n", samplerecord_read->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_read->pathname);
				printf("buffer size is %d\n", samplerecord_read->size);
				printf("return_value is %d\n", samplerecord_read->return_value);
				printf("file_address is %d\n", samplerecord_read->file_address);
				printf("Syscall for this record is READ(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == (int)samplerecord_read->file_address)
							break;
					}
					if(i == mapping_count)
						printf("Record cannot replay...No OPEN(2) call before this call traced.\n");
					else
						printf("Record can be replayed\n");
				}
				

				if(!n_flag){
					//Execute the sys call as n flag is not set
					free(buff);
					buff = malloc(samplerecord_read->size);
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord_read->file_address){
							rc = mapping_arr[i][1];
							break;
						}
					}
					if(rc != -1){
						read_ret = read(rc, buff, samplerecord_read->size);
						printf("The old return value was %d\n", samplerecord_read->return_value);
						printf("The new return value is %d\n", read_ret);
						if(s_flag == true && read_ret != samplerecord_read->return_value){
							printf("Aborting the replay program...Deviation occured\n");
							if(samplerecord_read){
								if(samplerecord_read->pathname)
									free(samplerecord_read->pathname);
								free(samplerecord_read);
							}
							goto strict_clean_label;
						}
					}

				}

				if(samplerecord_read){
					if(samplerecord_read->pathname)
						free(samplerecord_read->pathname);
					free(samplerecord_read);
				}
				break;

			case OPEN_TR:
				samplerecord_open = (struct trfs_open_record*)malloc(sizeof(struct trfs_open_record));

				memcpy((void *)&samplerecord_open->open_flags, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_open->permission_mode, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_open->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_open->pathname = malloc(samplerecord_open->pathname_size + 2);
				samplerecord_open->pathname[0] = '.';
				memcpy((void *)samplerecord_open->pathname + 1, (void *)(buff + buffer_offset), samplerecord_open->pathname_size);
				buffer_offset = buffer_offset + samplerecord_open->pathname_size;
				samplerecord_open->pathname[samplerecord_open->pathname_size + 1] = '\0';

				memcpy((void *)&samplerecord_open->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_open->file_address, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_open->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_open->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("open_flags is %d\n", samplerecord_open->open_flags);
				printf("permission_mode is %d\n", samplerecord_open->permission_mode);
				printf("pathname_size is %d\n", samplerecord_open->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_open->pathname);
				printf("return_value is %d\n", samplerecord_open->return_value);
				printf("file_address is %d\n", samplerecord_open->file_address);
				printf("Syscall for this record is OPEN(2)\n");
				printf("----------------------------------------------------\n");

				printf("Record can be replayed\n");

				//Create a row entry in mapping table
				if(n_flag){
					//Create a row entry in mapping table
					mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
					mapping_arr[mapping_count] = (int *)malloc(1*sizeof(int));
					//Fill the row with address and corresponding fd
					mapping_arr[mapping_count][0] = samplerecord_open->file_address;
					//mapping_arr[mapping_count] = rc;
					mapping_count++;
				}
				// printf("mapping_count %d\n", mapping_count);
				// for(i = 0; i < mapping_count; i++){
				// 	printf("%d\n", mapping_arr[i][0]);
				// }
				

				if(!n_flag){
					rc = open(samplerecord_open->pathname, samplerecord_open->open_flags, samplerecord_open->permission_mode);
					printf("The old file descriptor was %d\n", samplerecord_open->return_value);
					printf("The new file descriptor is %d\n", rc);
					if(s_flag == true && rc < 0){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_open){
							if(samplerecord_open->pathname)
								free(samplerecord_open->pathname);
							free(samplerecord_open);
						}
						goto strict_clean_label;
					}

					//Create a row entry in mapping table
					mapping_arr = (int **)realloc(mapping_arr, (mapping_count + 1)*sizeof(mapping_arr));
					mapping_arr[mapping_count] = (int *)malloc(2*sizeof(int));
					//Fill the row with address and corresponding fd
					mapping_arr[mapping_count][0] = samplerecord_open->file_address;
					mapping_arr[mapping_count][1] = rc;
					mapping_count++;
				}

				if(samplerecord_open){
					if(samplerecord_open->pathname)
						free(samplerecord_open->pathname);
				free(samplerecord_open);
				}
				
				break;

			case WRITE_TR:
				samplerecord_write = (struct trfs_write_record*)malloc(sizeof(struct trfs_write_record));

				memcpy((void *)&samplerecord_write->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_write->pathname = malloc(samplerecord_write->pathname_size + 2);
				samplerecord_write->pathname[0] = '.';
				memcpy((void *)samplerecord_write->pathname + 1, (void *)(buff + buffer_offset), samplerecord_write->pathname_size);
				buffer_offset = buffer_offset + samplerecord_write->pathname_size;
				samplerecord_write->pathname[samplerecord_write->pathname_size + 1] = '\0';

				memcpy((void *)&samplerecord_write->size, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_write->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_write->file_address, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_write->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				samplerecord_write->wr_buff = malloc(samplerecord_write->size);
				memcpy((void *)samplerecord_write->wr_buff, (void *)(buff + buffer_offset), samplerecord_write->size);
				buffer_offset = buffer_offset + samplerecord_write->size;

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_write->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("pathname_size is %d\n", samplerecord_write->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_write->pathname);
				printf("buffer size is %d\n", samplerecord_write->size);
				printf("return_value is %d\n", samplerecord_write->return_value);
				printf("file_address is %d\n", samplerecord_write->file_address);
				printf("Syscall for this record is WRITE(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == (int)samplerecord_write->file_address)
							break;
					}
					if(i == mapping_count)
						printf("Record cannot be replay...No OPEN(2) call before this call traced.\n");
					else
						printf("Record can be replayed\n");
				}
				

				if(!n_flag){
					//Execute the sys call as n flag is not set
					free(buff);
					buff = malloc(samplerecord_write->size);
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord_write->file_address){
							rc = mapping_arr[i][1];
							break;
						}
					}
					if(rc != -1){
						read_ret = write(rc, samplerecord_write->wr_buff, samplerecord_write->size);
						printf("The old return value was %d\n", samplerecord_write->return_value);
						printf("The new return value is %d\n", read_ret);
						if(s_flag == true && read_ret != samplerecord_write->return_value){
							printf("Aborting the replay program...Deviation occured\n");
							if(samplerecord_write){
								if(samplerecord_write->pathname)
									free(samplerecord_write->pathname);
								if(samplerecord_write->wr_buff)
									free(samplerecord_write->wr_buff);
								free(samplerecord_write);
							}
							goto strict_clean_label;
						}
					}

				}
				if(samplerecord_write){
					if(samplerecord_write->pathname)
						free(samplerecord_write->pathname);
					if(samplerecord_write->wr_buff)
						free(samplerecord_write->wr_buff);
					free(samplerecord_write);
				}
				
				break;

			case CLOSE_TR:
				samplerecord_close = (struct trfs_close_record*)malloc(sizeof(struct trfs_close_record));

				memcpy((void *)&samplerecord_close->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_close->pathname = malloc(samplerecord_close->pathname_size + 2);
				samplerecord_close->pathname[0] = '.';
				memcpy((void *)samplerecord_close->pathname + 1, (void *)(buff + buffer_offset), samplerecord_close->pathname_size);
				buffer_offset = buffer_offset + samplerecord_close->pathname_size;
				samplerecord_close->pathname[samplerecord_close->pathname_size + 1] = '\0';

				memcpy((void *)&samplerecord_close->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_close->file_address, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_close->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_close->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("pathname_size is %d\n", samplerecord_close->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_close->pathname);
				printf("return_value is %d\n", samplerecord_close->return_value);
				printf("file_address is %d\n", samplerecord_close->file_address);
				printf("Syscall for this record is CLOSE(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == (int)samplerecord_close->file_address)
							break;
					}
					if(i == mapping_count)
						printf("Record cannot be replay...No OPEN(2) call before this call traced.\n");
					else
						printf("Record can be replayed\n");
				}
				

	            if(!n_flag){
					//Execute the sys call as n flag is not set
					for(i = 0; i < mapping_count; i++){
						if(mapping_arr[i][0] == samplerecord_close->file_address){
							rc = mapping_arr[i][1];
							break;
						}
					}
					if(rc != -1){
						close(rc);
					}
				}

				if(samplerecord_close){
					if(samplerecord_close->pathname)
						free(samplerecord_close->pathname);
					free(samplerecord_close);
				}
				break;

			case LINK_TR:
				samplerecord_link = (struct trfs_link_record*)malloc(sizeof(struct trfs_link_record));

				memcpy((void *)&samplerecord_link->oldpathsize, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_link->oldpath = malloc(samplerecord_link->oldpathsize + 2);
				samplerecord_link->oldpath[0] = '.';
				memcpy((void *)samplerecord_link->oldpath + 1, (void *)(buff + buffer_offset), samplerecord_link->oldpathsize);
				buffer_offset = buffer_offset + samplerecord_link->oldpathsize;
				samplerecord_link->oldpath[samplerecord_link->oldpathsize + 1] = '\0';

				memcpy((void *)&samplerecord_link->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_link->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_link->newpathsize, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_link->newpath = malloc(samplerecord_link->newpathsize + 2);
				samplerecord_link->newpath[0] = '.';
				memcpy((void *)samplerecord_link->newpath + 1, (void *)(buff + buffer_offset), samplerecord_link->newpathsize);
				buffer_offset = buffer_offset + samplerecord_link->newpathsize;
				samplerecord_link->newpath[samplerecord_link->newpathsize + 1] = '\0';

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_link->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("old pathname_size is %d\n", samplerecord_link->oldpathsize + 1);
				printf("old pathname is %s\n", samplerecord_link->oldpath);
				printf("new pathname_size is %d\n", samplerecord_link->newpathsize + 1);
				printf("new pathname is %s\n", samplerecord_link->newpath);
				printf("return_value is %d\n", samplerecord_link->return_value);
				printf("Syscall for this record is LINK(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");
				}

				if(!n_flag){
					rc = link(samplerecord_link->oldpath, samplerecord_link->newpath);
					printf("The old return value was %d\n", samplerecord_link->return_value);
					printf("The new return value is %d\n", rc);
					if(s_flag == true && rc != samplerecord_link->return_value){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_link){
							if(samplerecord_link->oldpath)
								free(samplerecord_link->oldpath);
							if(samplerecord_link->newpath)
								free(samplerecord_link->newpath);
							free(samplerecord_link);
						}
						goto strict_clean_label;
					}
				}

				if(samplerecord_link){
					if(samplerecord_link->oldpath)
						free(samplerecord_link->oldpath);
					if(samplerecord_link->newpath)
						free(samplerecord_link->newpath);
					free(samplerecord_link);
				}
				break;

			case MKDIR_TR:
				samplerecord_mkdir = (struct trfs_mkdir_record *)malloc(sizeof(struct trfs_mkdir_record));

				memcpy((void *)&samplerecord_mkdir->permission_mode, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_mkdir->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_mkdir->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_mkdir->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_mkdir->pathname = malloc(samplerecord_mkdir->pathname_size + 2);
				samplerecord_mkdir->pathname[0] = '.';
				memcpy((void *)samplerecord_mkdir->pathname + 1, (void *)(buff + buffer_offset), samplerecord_mkdir->pathname_size);
				buffer_offset = buffer_offset + samplerecord_mkdir->pathname_size;
				samplerecord_mkdir->pathname[samplerecord_mkdir->pathname_size + 1] = '\0';

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_mkdir->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("permission_mode is %d\n", samplerecord_mkdir->permission_mode);
				printf("pathname_size is %d\n", samplerecord_mkdir->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_mkdir->pathname);
				printf("return_value is %d\n", samplerecord_mkdir->return_value);
				printf("Syscall for this record is MKDIR(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");	
				}

				if(!n_flag){
					rc = mkdir(samplerecord_mkdir->pathname, samplerecord_mkdir->permission_mode);
					printf("The old return value was %d\n", samplerecord_mkdir->return_value);
					printf("The new return value is %d\n", rc);
					if(s_flag == true && rc != samplerecord_mkdir->return_value){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_mkdir){
							if(samplerecord_mkdir->pathname)
								free(samplerecord_mkdir->pathname);
							free(samplerecord_mkdir);
						}
						goto strict_clean_label;
					}
				}
				if(samplerecord_mkdir){
					if(samplerecord_mkdir->pathname)
						free(samplerecord_mkdir->pathname);
					free(samplerecord_mkdir);
				}
				break;

			case UNLINK_TR:
				samplerecord_unlink = (struct trfs_unlink_record *)malloc(sizeof(struct trfs_unlink_record));

				memcpy((void *)&samplerecord_unlink->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_unlink->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_unlink->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_unlink->pathname = malloc(samplerecord_unlink->pathname_size + 2);
				samplerecord_unlink->pathname[0] = '.';
				memcpy((void *)samplerecord_unlink->pathname + 1, (void *)(buff + buffer_offset), samplerecord_unlink->pathname_size);
				buffer_offset = buffer_offset + samplerecord_unlink->pathname_size;
				samplerecord_unlink->pathname[samplerecord_unlink->pathname_size + 1] = '\0';

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_unlink->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("pathname_size is %d\n", samplerecord_unlink->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_unlink->pathname);
				printf("return_value is %d\n", samplerecord_unlink->return_value);
				printf("Syscall for this record is UNLINK(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");	
				}

				if(!n_flag){
					rc = unlink(samplerecord_unlink->pathname);
					printf("The old return value was %d\n", samplerecord_unlink->return_value);
					printf("The new return value is %d\n", rc);
					if(s_flag == true && rc != samplerecord_unlink->return_value){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_unlink){
							if(samplerecord_unlink->pathname)
								free(samplerecord_unlink->pathname);
							free(samplerecord_unlink);
						}
						goto strict_clean_label;
					}
				}
				if(samplerecord_unlink){
					if(samplerecord_unlink->pathname)
						free(samplerecord_unlink->pathname);
					free(samplerecord_unlink);
				}
				break;

			case RMDIR_TR:
				samplerecord_rmdir = (struct trfs_rmdir_record *)malloc(sizeof(struct trfs_rmdir_record));

				memcpy((void *)&samplerecord_rmdir->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_rmdir->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_rmdir->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_rmdir->pathname = malloc(samplerecord_rmdir->pathname_size + 2);
				samplerecord_rmdir->pathname[0] = '.';
				memcpy((void *)samplerecord_rmdir->pathname + 1, (void *)(buff + buffer_offset), samplerecord_rmdir->pathname_size);
				buffer_offset = buffer_offset + samplerecord_rmdir->pathname_size;
				samplerecord_rmdir->pathname[samplerecord_rmdir->pathname_size + 1] = '\0';

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_rmdir->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("pathname_size is %d\n", samplerecord_rmdir->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_rmdir->pathname);
				printf("return_value is %d\n", samplerecord_rmdir->return_value);
				printf("Syscall for this record is RMDIR(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");	
				}

				if(!n_flag){
					rc = rmdir(samplerecord_rmdir->pathname);
					printf("The old return value was %d\n", samplerecord_rmdir->return_value);
					printf("The new return value is %d\n", rc);
					if(s_flag == true && rc != samplerecord_rmdir->return_value){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_rmdir){
							if(samplerecord_rmdir->pathname)
								free(samplerecord_rmdir->pathname);
							free(samplerecord_rmdir);
						}
						goto strict_clean_label;
					}
				}
				if(samplerecord_rmdir){
					if(samplerecord_rmdir->pathname)
						free(samplerecord_rmdir->pathname);
					free(samplerecord_rmdir);
				}
				break;

			case SYMLINK_TR:
				samplerecord_symlink = (struct trfs_symlink_record*)malloc(sizeof(struct trfs_symlink_record));

				memcpy((void *)&samplerecord_symlink->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_symlink->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);
				

				memcpy((void *)&samplerecord_symlink->linkpath_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_symlink->linkpath = malloc(samplerecord_symlink->linkpath_size + 2);
				samplerecord_symlink->linkpath[0] = '.';
				memcpy((void *)samplerecord_symlink->linkpath + 1, (void *)(buff + buffer_offset), samplerecord_symlink->linkpath_size);
				buffer_offset = buffer_offset + samplerecord_symlink->linkpath_size;
				samplerecord_symlink->linkpath[samplerecord_symlink->linkpath_size + 1] = '\0';

				memcpy((void *)&samplerecord_symlink->targetpath_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_symlink->targetpath = malloc(samplerecord_symlink->targetpath_size + 1);
				//samplerecord_symlink->targetpath[0] = '.';
				memcpy((void *)samplerecord_symlink->targetpath, (void *)(buff + buffer_offset), samplerecord_symlink->targetpath_size);
				buffer_offset = buffer_offset + samplerecord_symlink->targetpath_size;
				samplerecord_symlink->targetpath[samplerecord_symlink->targetpath_size] = '\0';

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_symlink->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("targetpath_size is %d\n", samplerecord_symlink->targetpath_size + 1);
				printf("targetpath is %s\n", samplerecord_symlink->targetpath);
				printf("linkpath_size is %d\n", samplerecord_symlink->linkpath_size + 1);
				printf("linkpath is %s\n", samplerecord_symlink->linkpath);
				printf("return_value is %d\n", samplerecord_symlink->return_value);
				printf("Syscall for this record is SYMLINK(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");	
				}
				

				if(!n_flag){
					rc = symlink(samplerecord_symlink->targetpath, samplerecord_symlink->linkpath);
					printf("The old return value was %d\n", samplerecord_symlink->return_value);
					printf("The new return value is %d\n", rc);
					if(s_flag == true && rc != samplerecord_symlink->return_value){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_symlink){
							if(samplerecord_symlink->targetpath)
								free(samplerecord_symlink->targetpath);
							if(samplerecord_symlink->linkpath)
								free(samplerecord_symlink->linkpath);
							free(samplerecord_symlink);
						}
						goto strict_clean_label;
					}
				}

				if(samplerecord_symlink){
					if(samplerecord_symlink->targetpath)
						free(samplerecord_symlink->targetpath);
					if(samplerecord_symlink->linkpath)
						free(samplerecord_symlink->linkpath);
					free(samplerecord_symlink);
				}
				break;


			case RENAME_TR:
				samplerecord_rename = (struct trfs_rename_record*)malloc(sizeof(struct trfs_rename_record));

				memcpy((void *)&samplerecord_rename->oldpathsize, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_rename->oldpath = malloc(samplerecord_rename->oldpathsize + 2);
				samplerecord_rename->oldpath[0] = '.';
				memcpy((void *)samplerecord_rename->oldpath + 1, (void *)(buff + buffer_offset), samplerecord_rename->oldpathsize);
				buffer_offset = buffer_offset + samplerecord_rename->oldpathsize;
				samplerecord_rename->oldpath[samplerecord_rename->oldpathsize + 1] = '\0';

				memcpy((void *)&samplerecord_rename->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_rename->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_rename->newpathsize, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_rename->newpath = malloc(samplerecord_rename->newpathsize + 2);
				samplerecord_rename->newpath[0] = '.';
				memcpy((void *)samplerecord_rename->newpath + 1, (void *)(buff + buffer_offset), samplerecord_rename->newpathsize);
				buffer_offset = buffer_offset + samplerecord_rename->newpathsize;
				samplerecord_rename->newpath[samplerecord_rename->newpathsize + 1] = '\0';

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_rename->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("old pathname_size is %d\n", samplerecord_rename->oldpathsize + 1);
				printf("old pathname is %s\n", samplerecord_rename->oldpath);
				printf("new pathname_size is %d\n", samplerecord_rename->newpathsize + 1);
				printf("new pathname is %s\n", samplerecord_rename->newpath);
				printf("return_value is %d\n", samplerecord_rename->return_value);
				printf("Syscall for this record is RENAME(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");	
				}

				if(!n_flag){
					rc = rename(samplerecord_rename->oldpath, samplerecord_rename->newpath);
					printf("The old return value was %d\n", samplerecord_rename->return_value);
					printf("The new return value is %d\n", rc);
					if(s_flag == true && rc != samplerecord_rename->return_value){
						printf("Aborting the replay program...Deviation occured\n");
						if(samplerecord_rename){
							if(samplerecord_rename->oldpath)
								free(samplerecord_rename->oldpath);
							if(samplerecord_rename->newpath)
								free(samplerecord_rename->newpath);
							free(samplerecord_rename);
						}
						goto strict_clean_label;
					}
				}

				if(samplerecord_rename){
					if(samplerecord_rename->oldpath)
						free(samplerecord_rename->oldpath);
					if(samplerecord_rename->newpath)
						free(samplerecord_rename->newpath);
					free(samplerecord_rename);
				}
				break;

			case READLINK_TR:
				samplerecord_readlink = (struct trfs_readlink_record*)malloc(sizeof(struct trfs_readlink_record));

				memcpy((void *)&samplerecord_readlink->pathname_size, (void *)(buff + buffer_offset), sizeof(short));
				buffer_offset = buffer_offset + sizeof(short);

				samplerecord_readlink->pathname = malloc(samplerecord_readlink->pathname_size + 2);
				samplerecord_readlink->pathname[0] = '.';
				memcpy((void *)samplerecord_readlink->pathname + 1, (void *)(buff + buffer_offset), samplerecord_readlink->pathname_size);
				buffer_offset = buffer_offset + samplerecord_readlink->pathname_size;
				samplerecord_readlink->pathname[samplerecord_readlink->pathname_size + 1] = '\0';

				memcpy((void *)&samplerecord_readlink->size, (void *)(buff + buffer_offset), sizeof(unsigned long long));
				buffer_offset = buffer_offset + sizeof(unsigned long long);

				memcpy((void *)&samplerecord_readlink->return_value, (void *)(buff + buffer_offset), sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				memcpy((void *)&samplerecord_readlink->record_id,(void *)(buff + buffer_offset),  sizeof(int));
				buffer_offset = buffer_offset + sizeof(int);

				printf("----------------------------------------------------\n");
				printf("Record_id is %d\n", samplerecord_readlink->record_id);
				printf("Record_size is %d (in bytes)\n", record_size);
				printf("pathname_size is %d\n", samplerecord_readlink->pathname_size + 1);
				printf("pathname is %s\n", samplerecord_readlink->pathname);
				printf("buffer size is %d\n", samplerecord_readlink->size);
				printf("return_value is %d\n", samplerecord_readlink->return_value);
				printf("Syscall for this record is READLINK(2)\n");
				printf("----------------------------------------------------\n");

				if(n_flag){
					printf("Record can be replayed\n");	
				}

				if(!n_flag){
					//Execute the sys call as n flag is not set
					free(buff);
					buff = malloc(samplerecord_readlink->size);

					if(rc != -1){
						read_ret = readlink(samplerecord_readlink->pathname, buff, samplerecord_readlink->size);
						printf("The old return value was %d\n", samplerecord_readlink->return_value);
						printf("The new return value is %d\n", read_ret);
						if(s_flag == true && read_ret != samplerecord_readlink->return_value){
							printf("Aborting the replay program...Deviation occured\n");
							if(samplerecord_readlink){
								if(samplerecord_readlink->pathname)
									free(samplerecord_readlink->pathname);
								free(samplerecord_readlink);
							}
							goto strict_clean_label;
						}
					}

				}

				if(samplerecord_readlink){
					if(samplerecord_readlink->pathname)
						free(samplerecord_readlink->pathname);
					free(samplerecord_readlink);
				}
				break;

			default:
					printf("Invalid record_type\n");

		}
		
		if(buff)
			free(buff);

	}
	goto normal_clean_label;

strict_clean_label:
	if(buff != NULL)
		free(buff);
normal_clean_label:
	if(mapping_arr != NULL){
		//printf("Freeing table\n");
		for (i = 0; i < mapping_count; i++){
			if(mapping_arr[i]!= NULL){
				//printf("Freeing %d elemet\n", i);
         		free(mapping_arr[i]);
			}
		}
    	free(mapping_arr);
	}

	fclose(fileptr); // Close the file
	return 0;	
}
