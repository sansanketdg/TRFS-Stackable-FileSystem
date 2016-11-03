#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char *argv[])
{
  int fd;
 
 if(argc>3)
{
	printf("Invalid no of arguments\n");
	return 0;
}	

	int status=(int)strtol(argv[1],NULL,16);
	//char pathname[strlen(argv[2])];
	//pathname = argv[2];
	printf("status is:%d\n",status);
	printf("pathname is:%s\n",argv[2]);
	
    fd = open(argv[2], O_RDONLY);
	ioctl(fd,1,&status);
   //if (ioctl(fd, 1, &status) == -1)
     // printf("TIOCMGET failed: %s\n",
       //      strerror(errno));
  /* else {
      if (status & TIOCM_DTR)
         puts("TIOCM_DTR is not set");
      else
         puts("TIOCM_DTR is set");
   }*/
   close(fd);
return 0;
}
