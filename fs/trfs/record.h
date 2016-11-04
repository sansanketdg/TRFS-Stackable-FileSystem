#define MK_DIR 2
#define OPEN_TR 1
#define READ_TR 3
#define WRITE_TR 4
/* trfs record structure used to store to store in file*/
struct trfs_record {
	
	unsigned short record_size;
	int record_id;
	unsigned char record_type;
	int open_flags;
	int permission_mode;
	short pathname_size;
	char *pathname;

	//unsigned long long offset;
	unsigned long long  size;
	int return_value;
	unsigned long long file_address;
	
};
