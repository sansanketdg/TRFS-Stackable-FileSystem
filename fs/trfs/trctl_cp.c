#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char *argv[])
{
  int fd;
  int i,j,k=0;
  int bitmap;
  char option[25];
  char call[25];
  if(argc<2)
  {
     printf("Invalid no of arguments\n");
     return 0;
  }  
  #ifdef EXTRA_CREDIT
  {
    fd = open(argv[argc-1], O_RDONLY);
    if(fd<0)
    {
        printf("\n Invalid Path");
    }

    for(i=1;i<argc-1;i++)
    {
      option=argv[i];
      if(option[0]=='+'||option[0]=='-')
      {
        if(option[0]=='+')
        {
            for(j=1;j<strlen(option);j++)
                call[k++]=option[j];
            call[k]='\0'
            if(strcmp(call,"read")==0) 
            {
              bitmap=bitmap|READ_TR;
            }
            if(strcmp(call,"write")==0) 
            {
              bitmap=bitmap|WRITE_TR;
            }
            if(strcmp(call,"open")==0) 
            {
              bitmap=bitmap|OPEN_TR;
            }
            if(strcmp(call,"close")==0) 
            {
              bitmap=bitmap|CLOSE_TR;
            }
            if(strcmp(call,"mkdir")==0) 
            {
              bitmap=bitmap|MKDIR_TR;
            }
            if(strcmp(call,"link")==0) 
            {
              bitmap=bitmap|LINK_TR;
            }
            if(strcmp(call,"rmdir")==0) 
            {
              bitmap=bitmap|RMDIR_TR;
            }
            if(strcmp(call,"unlink")==0) 
            {
              bitmap=bitmap|UNLINK_TR;
            }




        }
      }

    }
    ioctl(fd,1,&bitmap);
    printf("The present bitmap value is%x\n",bitmap);
  
    close(fd);

  }
  else


{

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
        printf("\n Invalid Path")
    }
    ioctl(fd,10,&bitmap);
    printf("The present bitmap value is%x\n",bitmap);
  
    close(fd);

  }
  if(argc==3)
{

    if(strcmp(argv[1],"all")==0)
    {
        bitmap=0xff;

    }  
    else if(strcmp(argv[1],"none")==0)
    {
        bitmap=0x00;
      
    }  
    else
  	   bitmap=(int)strtol(argv[1],NULL,16);

    if(bitmap>255)
    printf("Invalid Bitmap value\n");	

	  printf("Mount Pathname is:%s\n",argv[2]);
	
    fd = open(argv[2], O_RDONLY);
    if(fd<0)
    {
        printf("\n Invalid Path")
    }
	    ioctl(fd,1,&bitmap);
  
    close(fd);
}
}
return 0;
}
