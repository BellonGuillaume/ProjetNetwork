#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/window.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

int n=1000;
void test_perfect()
{
	printf("Test envoi de %d kb sur une ligne parfaite.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./receiver -f out.txt localhost 6565 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_err()
{
	printf("Test envoi de %d kb avec 25/100 erreurs de corruption.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./test/linksim/link_sim -e 25 -p 6565 -P 1341  >&- & ./receiver -f out.txt :: 1341 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_loss()
{
	printf("Test envoi de %d kb avec 25/100 des paquets perdus.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./test/linksim/link_sim -l 25 -p 6565 -P 1341  >&- & ./receiver -f out.txt :: 1341 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_delay()
{
	printf("Test envoi de %d kb avec un délai de 100ms et un jitter de 200ms.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./test/linksim/link_sim -d 100 -j 200 -p 6565 -P 1341  >&- & ./receiver -f out.txt :: 1341 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_err_1mb()
{
	printf("Test envoi de %d kb avec 1/100 erreurs de corruption.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./test/linksim/link_sim -e 1 -p 6565 -P 1341  >&- & ./receiver -f out.txt :: 1341 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_loss_1mb()
{
	printf("Test envoi de %d kb avec 1/100 des paquets perdus.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./test/linksim/link_sim -l 1 -p 6565 -P 1341 >&- & ./receiver -f out.txt :: 1341 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_tot()
{
	printf("Test envoi de %d kb avec 1/100 erreurs de corruption, 1/100 de paquets perdus, 100ms de délai et 200ms de jitter.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt");
		system("./test/linksim/link_sim -e 1 -l 1 -d 100 -j 200 -p 6565 -P 1341  >&- & ./receiver -f out.txt :: 1341 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565");
	close(pid);
	int fd= open("out.txt",O_RDONLY);
	int err=0;
	if(fd<0)
	{
		err=1;
		fprintf(stderr,"No output file created\n");
		CU_ASSERT(err==0);
	}
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			read(fd,buf,1);
			if(buf[0]!=(i+'0'))
			{
				err=1;
			}
		}
	}
	close(fd);
	system("rm out.txt");
	CU_ASSERT(err==0);
}

void test_window()
{
	int length = 10;
	window_t* test = window_new(length);
	CU_ASSERT(test != NULL);
	if(test==NULL)
	return;
	CU_ASSERT(test->length == length && test->size_used == 0 && test->buffer != NULL);
	pkt_t* pktest = pkt_new();
	node_t* nodetest = node_new(pktest);
	CU_ASSERT(nodetest!=NULL);
	if(nodetest==NULL)
	return;
	CU_ASSERT(nodetest->pkt == pktest && nodetest->seqnum == pkt_get_seqnum(pktest));
	CU_ASSERT(window_add(test, pktest) >= 0);
	CU_ASSERT(test->length == length && test->size_used == 1 && test->buffer != NULL);
	window_remove(test,nodetest->seqnum);
	CU_ASSERT(test->size_used == 0 && test->buffer != NULL);
	window_del(test);
	test=NULL;
}

int main (int argc, char* argv[])
{
	system("fuser -k 1341/udp");
	system("fuser -k 6565/udp");
	system("rm test.txt");
	int fd=open("test.txt",O_CREAT|O_TRUNC|O_WRONLY,S_IRWXU|S_IRWXO);
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			buf[0]=i+'0';
			write(fd,buf,1);
		}
	}
	close(fd);
	CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("Tests du programme avec 10kb", 0, 0);
  CU_add_test(suite, "test1", test_perfect);
	CU_add_test(suite, "test2", test_err);
	CU_add_test(suite, "test3", test_loss);
	CU_add_test(suite, "test3", test_delay);
	CU_pSuite suite2 = CU_add_suite("Tests du programme avec 1mb", 0, 0);
  CU_add_test(suite2, "test1", test_perfect);
	CU_add_test(suite2, "test2", test_err_1mb);
	CU_add_test(suite2, "test3", test_loss_1mb);
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_suite(suite);

	n=n*25;
	system("fuser -k 1341/udp");
	system("fuser -k 6565/udp");
	system("rm test.txt");
	fd=open("test.txt",O_CREAT|O_TRUNC|O_WRONLY,S_IRWXU|S_IRWXO);
	for(int j=0;j<n;j++)
	{
		for(int i=0;i<10;i++)
		{
			char buf[1];
			buf[0]=i+'0';
			write(fd,buf,1);
		}
	}
	close(fd);
	CU_basic_run_suite(suite2);

	system("fuser -k 1341/udp");
	system("fuser -k 6565/udp");
	CU_pSuite suite3 = CU_add_suite("Test avec mauvaise connection du programme", 0, 0);
	CU_add_test(suite3, "test1", test_tot);
	CU_basic_run_suite(suite3);

	system("rm test.txt");

	CU_pSuite suite_window = CU_add_suite("Tests fonctions de window.c", 0, 0);
	CU_add_test(suite_window, "test1", test_window);
	CU_basic_run_suite(suite_window);
	CU_cleanup_registry();
}
