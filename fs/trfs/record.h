#define MK_DIR 1
#define OPEN_TR 2
#define READ_TR 4
#define WRITE_TR 8
#define CLOSE_TR 16
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
	unsigned long long size;

	int return_value;
	unsigned long long file_address;

	char *wr_buff;
};

struct trfs_read_record {

	unsigned short record_size;
	unsigned char record_type;
	
	short pathname_size;
	char *pathname;

	unsigned long long size;

	int return_value;
	unsigned long long file_address;
	int record_id;
}; 

struct trfs_write_record {

	unsigned short record_size;
	
	unsigned char record_type;
	
	short pathname_size;
	char *pathname;

	unsigned long long size;

	int return_value;
	unsigned long long file_address;
	int record_id;

	char *wr_buff;
};

struct trfs_open_record {

	unsigned short record_size;
	unsigned char record_type;

	int open_flags;
	int permission_mode;

	short pathname_size;
	char *pathname;

	int return_value;
	unsigned long long file_address;

	int record_id;
};

struct trfs_close_record {

	unsigned short record_size;
	unsigned char record_type;

	short pathname_size;
	char *pathname;

	int return_value;
	unsigned long long file_address;

	int record_id;
};
