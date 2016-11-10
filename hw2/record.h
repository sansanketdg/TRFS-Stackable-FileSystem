/* these are the default bitmap values in decimal of different sys-calls */
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

/* set this flag to 1 if want to support incrementale IOCTL */
#define EXTRA_CREDIT 0

/* these are used to define either setting/ getting the bitmap value in IOCTL call*/
#define TRFS_GET_FLAG 0
#define TRFS_SET_FLAG 10

/* these values are the decimal values for 'all' and 'none' options provided in trctl command */
#define BITMAP_ALL 2047
#define BITMAP_NONE 0

/* trfs record structure for READ(2) sys call used to store to store in file*/
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

/* trfs record structure for WRITE(2) sys call used to store to store in file*/
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

/* trfs record structure for OPEN(2) sys call used to store to store in file*/
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

/* trfs record structure for CLOSE(2) sys call used to store to store in file*/
struct trfs_close_record {

	unsigned short record_size;
	int record_type;

	short pathname_size;
	char *pathname;

	int return_value;
	unsigned long long file_address;
 
	int record_id;
};

/* trfs record structure for LINK(2) sys call used to store to store in file*/
struct trfs_link_record
{
   unsigned short record_size;
   int record_type;
   short oldpathsize;
   char *oldpath;

   int return_value;
   int record_id;

   short newpathsize;
   char *newpath;
   

};


/* trfs record structure for MKDIR(2) sys call used to store to store in file*/
struct trfs_mkdir_record
{
   unsigned short record_size;
   int record_type;
   int permission_mode;
   int return_value;
   int record_id;
   short pathname_size;
   char *pathname;
   

};

/* trfs record structure for UNLINK(2) sys call used to store to store in file*/
struct trfs_unlink_record
{
   unsigned short record_size;
   int record_type;
   int return_value;
   int record_id;
   short pathname_size;
   char *pathname;
   

};

/* trfs record structure for RMDIR(2) sys call used to store to store in file*/
struct trfs_rmdir_record
{
   unsigned short record_size;
   int record_type;
   int return_value;
   int record_id;
   short pathname_size;
   char *pathname;
   

};

/* trfs record structure for SYMLINK(2) sys call used to store to store in file*/
struct trfs_symlink_record
{

	unsigned short record_size;
    int record_type;
    
    int return_value;
    int record_id;

    short linkpath_size;
    char *linkpath;

    short targetpath_size;
    char *targetpath;
};

/* trfs record structure for RENAME(2) sys call used to store to store in file*/
struct trfs_rename_record
{

	unsigned short record_size;
	int record_type;

	short oldpathsize;
	char *oldpath;

	int return_value;
	int record_id;

	short newpathsize;
    char *newpath;
};

/* trfs record structure for READLINK(2) sys call used to store to store in file*/
struct trfs_readlink_record {

	unsigned short record_size;
	int record_type;
	
	short pathname_size;
	char *pathname;

	unsigned long long size;

	int return_value;
	int record_id;
};

