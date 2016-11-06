#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc, char **argv)
{
        //struct argument arg;
	//int flagentered;
	int rc;
        char c;
        int flag=0;
	int index=0;
        int count=0;
        int k=0;
	char *buff;
	buff = malloc(52*sizeof(char));
	rc = open("/mnt/trfs/hw1/test_hw2.txt", O_CREAT | O_RDWR, 0644);
  //rc1 = open("/mnt/trfs/demo.sh", O_CREAT | O_RDONLY, 0644);
	k = read(rc, buff, 12);
	k = write(rc, buff, 12);

  //rc = link("/mnt/trfs/hw1/test_hw2.txt", "/mnt/trfs/hw1/test_hw2_link.txt");
  //k = read(rc1, buff, 52);
  //rc = mkdir("/mnt/trfs/hw1/test_hw2_mkdir", 0777);
	printf("rc=%d\n",rc);

	free(buff);
  close(rc);
  //close(rc1);
/*        while ((c = getopt (argc, argv, "uaitd")) != -1)
    		switch (c)
      		{
      			case 'u':
        			 flag=flag|0x01 ;
       				 break;
      			case 'a':
        			flag=flag|0x02 ;
                         	break;
        		case 'i':
        			flag=flag|0x04;
        			break;
                        case 't':
                        	flag=flag|0x10;
                        	break;
                        case 'd':
                        	flag=flag|0x20;
                        	break;
                        default:
                        	abort ();
     		 }
        
         arg.outfile=argv[optind];  	  
     	 for (index = optind+1; index < argc; index++)
	{
       		arg.inputfile[k++]=argv[index];
	        count=count+1;	
	}

        arg.data=(int*)malloc(sizeof(int));
        arg.noofinfile=count;
        arg.flag= flag;
	if(flag==0)
	printf("\nWrong arguments entered!Enter atleast 1 flag");
	
  	rc = syscall(__NR_xmergesort,(void*)&arg);

	if (rc == 0){

                printf("syscall returned %d\n", rc);
		if(flag&0x20)
		printf("\nData at user space is:%d",*(arg.data));
		}
	else
	{
		printf("syscall returned %d (errno=%d)\n", rc, errno);
                perror("Message from Syscall");
	}


//	free(arg->data);
	//free(arg);*/
	exit(rc);
}
