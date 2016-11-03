#define MKDIR 2
#define OPEN 1

/* trfs record structure used to store to store in file*/
struct trfs_record {
	int record_id;
	unsigned short record_size;
	unsigned char record_type;
	int open_flags;
	int permission_mode;
	short pathname_size;
	char *pathname;
	int return_value;
	
};