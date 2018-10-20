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
#define N 10

void test()
{
	int fd=open("test.txt",O_CREAT|O_WRONLY,S_IRWXU|S_IRWXO);
	for(int j=0;j<N;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[sizeof(int)];
			sprintf(buf,"%d",i);
			write(fd,buf,sizeof(int));
		}
	}
	pid_t pid=fork();
	if(pid==0)
	{
		system("../receiver -f out.txt localhost 6565");
		exit(0);
	}
	else
	{
		sleep(1);
		system("../sender -f test.txt localhost 6565");
	}

	close(fd);
	fd= open("out.txt",O_RDONLY);
	int err=0;
	for(int j=0;j<N;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[sizeof(int)];
			char buf2[sizeof(int)];
			sprintf(buf2,"%d",i);
			read(fd,buf,sizeof(int));
			//printf("%s --- %d\n",buf,i);
			if(strcmp(buf,buf2)!=0)
			{
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
	CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("test", 0, 0);
  CU_add_test(suite, "test", test);
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
	CU_cleanup_registry();
}
