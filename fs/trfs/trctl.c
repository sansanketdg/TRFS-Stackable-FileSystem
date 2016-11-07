#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
int main(int argc,char *argv[])
{
  int fd;
  int i;
  int bitmap;
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
    ioctl(fd,10,&bitmap);
    printf("The present bitmap value is 0x%x\n",bitmap);
  
    close(fd);

  }
  if(argc==3)
{

    if(strcmp(argv[1],"all")==0)
    {
        bitmap=0x3ff;

    }  
    else if(strcmp(argv[1],"none")==0)
    {
        bitmap=0x000;
      
    }  
    else if(!(argv[1][0] == '0' && argv[1][1] == 'x')){
      printf("Invalid argument\n");
      return 0;
    }

    else
  	   bitmap=(int)strtol(argv[1],NULL,16);

    if(bitmap>1027)
    printf("Invalid Bitmap value\n");	

	  printf("Mount Pathname is:%s\n",argv[2]);
	
    fd = open(argv[2], O_RDONLY);
    if(fd<0)
    {
        printf("\n Invalid Path");
    }
	    ioctl(fd,1,&bitmap);
  
    close(fd);
}
return 0;
}
