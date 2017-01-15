#define MKDIR_TR 1
#define OPEN_TR 2
#define READ_TR 4
#define WRITE_TR 8
#define CLOSE_TR 16
#define LINK_TR 32
#define UNLINK_TR 64
#define RMDIR_TR 128
#define SYMLINK_TR 256
#define RENAME_TR 512
#define READLINK_TR 1024

#define TRFS_GET_FLAG 0
#define TRFS_SET_FLAG 10

#define BITMAP_ALL 2047
#define BITMAP_NONE 0

/* trfs record structure used to store to store in file*/
struct trfs_record {

	unsigned short record_size;
	int record_id;
	int record_type;
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
	int record_type;

	short pathname_size;
	char *pathname;

	unsigned long long size;

	int return_value;
	unsigned long long file_address;
	int record_id;
};

struct trfs_write_record {

	unsigned short record_size;

	int record_type;

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
	int record_type;

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
	int record_type;

	short pathname_size;
	char *pathname;

	int return_value;
	unsigned long long file_address;

	int record_id;
};

struct trfs_link_record {
	unsigned short record_size;
	int record_type;
	short oldpathsize;
	char *oldpath;

	int return_value;
	int record_id;

	short newpathsize;
	char *newpath;

};

struct trfs_mkdir_record {
	unsigned short record_size;
	int record_type;
	int permission_mode;
	int return_value;
	int record_id;
	short pathname_size;
	char *pathname;

};

struct trfs_unlink_record {
	unsigned short record_size;
	int record_type;
	int return_value;
	int record_id;
	short pathname_size;
	char *pathname;

};

struct trfs_rmdir_record {
	unsigned short record_size;
	int record_type;
	int return_value;
	int record_id;
	short pathname_size;
	char *pathname;

};

struct trfs_symlink_record {

	unsigned short record_size;
	int record_type;

	int return_value;
	int record_id;

	short linkpath_size;
	char *linkpath;

	short targetpath_size;
	char *targetpath;
};

struct trfs_rename_record {

	unsigned short record_size;
	int record_type;

	short oldpathsize;
	char *oldpath;

	int return_value;
	int record_id;

	short newpathsize;
	char *newpath;
};

struct trfs_readlink_record {

	unsigned short record_size;
	int record_type;

	short pathname_size;
	char *pathname;

	unsigned long long size;

	int return_value;
	int record_id;
};
