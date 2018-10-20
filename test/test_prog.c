#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

int n=1;
void test()
{
	int fd=open("test.txt",O_CREAT|O_WRONLY,S_IRWXU|S_IRWXO);
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			buf[0]=i+'0';
			write(fd,buf,1);
		}
	}
	pid_t pid=fork();
	if(pid==0)
	{
		system("../receiver -f out.txt localhost 1341");
		exit(0);
	}
	sleep(1);
	system("../sender -f test.txt localhost 1341");
	close(pid);
	close(fd);
	fd= open("out.txt",O_RDONLY);
	int err=0;
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				//printf("%c - %c\n",buf[0],i+'0');
				err=1;
			}
		}
	}
	close(fd);
	//fprintf(stderr,"%d\n",err);
	CU_ASSERT(err==0);
}

int main (int argc, char* argv[])
{
	if(argc>1)
	{
		n=atoi(argv[1]);
	}
	printf("Test envoi de %d kb.\n",n/100);
	CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("test", 0, 0);
  CU_add_test(suite, "test", test);
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
	CU_cleanup_registry();
}
