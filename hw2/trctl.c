#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record.h"

#define EXTRA_CREDIT 0

int main(int argc,char *argv[])
{
  int fd;
  int i,j,k=0;
  int retVal;
  unsigned int actual_bitmap = 0;
  unsigned int bitmap_plus = 0;
  unsigned int bitmap_minus = 0;
  char *option = NULL;
  option = malloc(sizeof(char)*25);
  char call[25];
  
  #if EXTRA_CREDIT

    fd = open(argv[argc-1], O_RDONLY);
    if(fd<0)
    {
        printf("\n Invalid Path");
    }

    if(argc==2)
    {
      fd = open(argv[1], O_RDONLY);
      if(fd<0)
      {
        printf("\n Invalid Path");
      }
      retVal = ioctl(fd,TRFS_GET_FLAG,&actual_bitmap);
      printf("The present bitmap value is 0x%x\n", actual_bitmap);
  
      close(fd);

    }
    
    for(i=1;i<argc-1;i++)
    {
      option=argv[i];
      if(option[0]=='+'||option[0]=='-')
      {
            k = 0;
            for(j=1;j<strlen(option);j++)
                call[k++]=option[j];
            call[k]='\0';
            if(strcmp(call,"read")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | READ_TR;
              else
                bitmap_minus = bitmap_minus | READ_TR;
            }
            else if(strcmp(call,"write")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | WRITE_TR;
              else
                bitmap_minus = bitmap_minus | WRITE_TR;
            }
            else if(strcmp(call,"open")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | OPEN_TR;
              else
                bitmap_minus = bitmap_minus | OPEN_TR;
            }
            else if(strcmp(call,"close")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | CLOSE_TR;
              else
                bitmap_minus = bitmap_minus | CLOSE_TR;
            }
            else if(strcmp(call,"mkdir")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | MKDIR_TR;
              else
                bitmap_minus = bitmap_minus | MKDIR_TR;
            }
            else if(strcmp(call,"link")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | LINK_TR;
              else
                bitmap_minus = bitmap_minus | LINK_TR;
            }
            else if(strcmp(call,"rmdir")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | RMDIR_TR;
              else
                bitmap_minus = bitmap_minus | RMDIR_TR;
            }
            else if(strcmp(call,"unlink")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | UNLINK_TR;
              else
                bitmap_minus = bitmap_minus | UNLINK_TR;
            }
            else if(strcmp(call,"symlink")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | SYMLINK_TR;
              else
                bitmap_minus = bitmap_minus | SYMLINK_TR;
            }
            else if(strcmp(call,"readlink")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | READLINK_TR;
              else
                bitmap_minus = bitmap_minus | READLINK_TR;
            }
            else if(strcmp(call,"rename")==0) 
            {
              if(option[0] == '+')
                bitmap_plus = bitmap_plus | RENAME_TR;
              else
                bitmap_minus = bitmap_minus | RENAME_TR;
            }
        }
      }
    retVal = ioctl(fd, TRFS_GET_FLAG, &actual_bitmap);

    bitmap_minus = ~bitmap_minus;

    //for negative bitmap, negate it and AND it with actual bitmap
    actual_bitmap = actual_bitmap & bitmap_minus;
    actual_bitmap = actual_bitmap | bitmap_plus;
    
    retVal = ioctl(fd,TRFS_SET_FLAG,&actual_bitmap);
    printf("The present bitmap value is 0x%x\n",actual_bitmap);
    close(fd);

  #else
  
  //Default IOCTL flow
    if(argc>3)
    {
	    printf("Invalid no of arguments\n");
	    return 0;
    }	
    if(argc==2)
    {
      fd = open(argv[1], O_RDONLY);
      if(fd<0)
      {
        printf("\n Invalid Path");
      }
      retVal = ioctl(fd,TRFS_GET_FLAG,&actual_bitmap);
      printf("The present bitmap value is 0x%x\n", actual_bitmap);
  
      close(fd);

    }
    if(argc==3)
    {

      if(strcmp(argv[1],"all") == 0)
      {
        actual_bitmap = 0x7ff;
      }  
      else if(strcmp(argv[1],"none") == 0)
      {
        actual_bitmap = 0x000;
      }  
      else
  	    actual_bitmap = (int)strtol(argv[1], NULL, 16);

      if(actual_bitmap > 2047)
      printf("Invalid Bitmap value\n");	

	    printf("Mount Pathname is:%s\n",argv[2]);
	
      fd = open(argv[2], O_RDONLY);
      if(fd < 0)
      {
        printf("\n Invalid Path");
      }
	    retVal = ioctl(fd, TRFS_SET_FLAG, &actual_bitmap);

      printf("Bitmap is now set to 0x%x successfully\n", actual_bitmap);
  
      close(fd);
    }
  #endif
  return 0;
}
