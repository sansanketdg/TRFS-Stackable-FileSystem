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
      
	int rc;
        char c;
        int flag=0;
	int index=0;
        int count=0;
        int k=0;
	char *buff;
	buff = malloc(52*sizeof(char));
	rc = open("/mnt/trfs/hw1/test_hw2.txt", O_CREAT | O_RDWR | O_APPEND, 0644);
  
	k = read(rc, buff, 12);
  k = write(rc, buff, 12);

  rc = mkdir("/mnt/trfs/hw1/test_hw2_dir_1/", 0644);
  printf("rc=%d\n",rc);
  rc = rmdir("/mnt/trfs/hw1/test_hw2_dir_1/");
  printf("rc=%d\n",rc);

  rc = rename("/mnt/trfs/hw1/test_hw2.txt", "/mnt/trfs/hw1/test_hw2_renamed.txt");
  printf("rc=%d\n",rc);
  rc = rename("/mnt/trfs/hw1/test_hw2_renamed.txt", "/mnt/trfs/hw1/test_hw2.txt");
  printf("rc=%d\n",rc);  

  rc = link("/mnt/trfs/hw1/test_hw2.txt", "/mnt/trfs/hw1/test_hw2_hardlink.txt");
  printf("rc=%d\n",rc);

  rc = unlink("/mnt/trfs/hw1/test_hw2_hardlink.txt");
  printf("rc=%d\n",rc);

  rc = symlink("/mnt/trfs/hw1/test_hw2.txt", "/mnt/trfs/hw1/test_hw2_symlink11.txt");
  printf("rc=%d\n",rc);

  rc = readlink("/mnt/trfs/hw1/test_hw2_symlink11.txt", buff, 52);
  printf("rc=%d\n",rc);

  rc = unlink("/mnt/trfs/hw1/test_hw2_symlink11.txt");
  printf("rc=%d\n",rc);
	
	free(buff);
  close(rc);

	exit(rc);
}
