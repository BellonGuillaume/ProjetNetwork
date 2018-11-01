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
#include <string.h>
#include "../src/commonlib.c"

int n=100;
void test_perfect()
{
	printf("Test envoi de %d kb sur une ligne parfaite.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./receiver -f out.txt localhost 6565 >&-");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
	CU_ASSERT(err==0);
}

void test_err()
{
	printf("Test envoi de %d kb avec 25/100 erreurs de corruption.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./test/linksim/link_sim -e 25 -p 6565 -P 1341 & ./receiver -f out.txt :: 1341 > /dev/null 2>&1");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
	CU_ASSERT(err==0);
}

void test_loss()
{
	printf("Test envoi de %d kb avec 25/100 des paquets perdus.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./test/linksim/link_sim -l 25 -p 6565 -P 1341 & ./receiver -f out.txt :: 1341 > /dev/null 2>&1");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
	CU_ASSERT(err==0);
}

void test_delay()
{
	printf("Test envoi de %d kb avec un délai de 100ms et un jitter de 200ms.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./test/linksim/link_sim -d 100 -j 200 -p 6565 -P 1341 & ./receiver -f out.txt :: 1341 > /dev/null 2>&1");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
	CU_ASSERT(err==0);
}

void test_err_1mb()
{
	printf("Test envoi de %d kb avec 1/100 erreurs de corruption.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./test/linksim/link_sim -e 1 -p 6565 -P 1341 & ./receiver -f out.txt :: 1341 > /dev/null 2>&1");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
	CU_ASSERT(err==0);
}

void test_loss_1mb()
{
	printf("Test envoi de %d kb avec 1/100 des paquets perdus.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./test/linksim/link_sim -l 1 -p 6565 -P 1341 & ./receiver -f out.txt :: 1341 > /dev/null 2>&1");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
	CU_ASSERT(err==0);
}

void test_tot()
{
	printf("Test envoi de %d kb avec 1/100 erreurs de corruption, 1/100 de paquets perdus, 100ms de délai et 200ms de jitter.\n",n/100);
	pid_t pid=fork();
	if(pid==0)
	{
		system("rm out.txt > /dev/null 2>&1");
		system("./test/linksim/link_sim -e 1 -l 1 -d 100 -j 200 -p 6565 -P 1341 & ./receiver -f out.txt :: 1341 > /dev/null 2>&1");
		exit(0);
	}
	sleep(1);
	system("./sender -f test.txt localhost 6565 > /dev/null 2>&1");
	close(pid);
	int err=system("diff test.txt out.txt");
	system("rm out.txt > /dev/null 2>&1");
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

void test_commonlib()
{
	char* argv1[] = {"./sender", "-f", "file1", "123456789", "6565"};
	int argc1=5;
	char* argv2[] = {"./sender", "-f", "123456789", "6565"};
	int argc2=4;
	char* argv3[] = {"./sender", "file1", "123456789", "6565"};
	int argc3=4;
	char* argv4[] = {"./sender", "123456789", "6565"};
	int argc4=3;
	char* argv5[] = {"./sender", "-f", "file1", "6565"};
	int argc5=4;
	char* argv6[] = {"./sender", "6565"};
	int argc6=2;
	int optionf=-1;
	char filename[255]="";
	char first_address[16]="";
	int port=-1;
	int err=-1;
	int oldstdout=dup(1);
	freopen("/dev/null", "w", stderr);
	CU_ASSERT(ValidateArgs(argc1,argv1,&optionf,filename,first_address,&port)==0);
	CU_ASSERT(optionf==1);
	CU_ASSERT(strcmp(filename,"file1")==0);
	CU_ASSERT(strcmp(first_address,"123456789")==0);
	CU_ASSERT(port=6565);
	ValidateArgs(argc2,argv2,&optionf,filename,first_address,&port);
	CU_ASSERT(ValidateArgs(argc2,argv2,&optionf,filename,first_address,&port)!=0);
	CU_ASSERT(ValidateArgs(argc3,argv3,&optionf,filename,first_address,&port)!=0);
	CU_ASSERT(ValidateArgs(argc4,argv4,&optionf,filename,first_address,&port)==0);
	CU_ASSERT(optionf==0);
	CU_ASSERT(strcmp(first_address,"123456789")==0);
	CU_ASSERT(port==6565);
	CU_ASSERT(ValidateArgs(argc5,argv5,&optionf,filename,first_address,&port)!=0);
	CU_ASSERT(ValidateArgs(argc6,argv6,&optionf,filename,first_address,&port)!=0);
  fclose(stderr);
	stdout=fdopen(oldstdout,"w");

}

int main (int argc, char* argv[])
{
	system("cd test/linksim > /dev/null 2>&1");
	system("make > /dev/null 2>&1");
	system("make rebuild > /dev/null 2>&1");
	system("cd .. > /dev/null 2>&1");
	system("cd .. > /dev/null 2>&1");
	system("fuser -k 1341/udp > /dev/null 2>&1");
	system("fuser -k 6565/udp > /dev/null 2>&1");
	system("rm test.txt > /dev/null 2>&1");
	system("clear");
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
  CU_pSuite suite = CU_add_suite("Tests du programme avec 1kb", 0, 0);
  CU_add_test(suite, "test1", test_perfect);
	CU_add_test(suite, "test2", test_err);
	CU_add_test(suite, "test3", test_loss);
	CU_add_test(suite, "test3", test_delay);
	CU_pSuite suite2 = CU_add_suite("Tests du programme avec 0.1mb", 0, 0);
  CU_add_test(suite2, "test1", test_perfect);
	CU_add_test(suite2, "test2", test_err_1mb);
	CU_add_test(suite2, "test3", test_loss_1mb);
  CU_basic_set_mode(CU_BRM_VERBOSE);

	//Test 10kb
  CU_basic_run_suite(suite);
	//sleep(5);
	//system("clear");

	n=n*100;
	system("fuser -k 1341/udp");
	system("fuser -k 6565/udp");
	system("rm test.txt > /dev/null 2>&1");
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

	//Test 0.25mb
	CU_basic_run_suite(suite2);
	//sleep(5);
	//system("clear");

	system("fuser -k 1341/udp");
	system("fuser -k 6565/udp");
	CU_pSuite suite3 = CU_add_suite("Test avec mauvaise connection du programme", 0, 0);
	CU_add_test(suite3, "test1", test_tot);

	//Test total
	CU_basic_run_suite(suite3);
	//sleep(5);
	//system("clear");

	system("rm test.txt > /dev/null 2>&1");

	CU_pSuite suite_window = CU_add_suite("Tests fonctions de window.c", 0, 0);
	CU_add_test(suite_window, "test1", test_window);

	//Test window
	CU_basic_run_suite(suite_window);
	//sleep(5);
	//system("clear");

	CU_pSuite suite_commonlib = CU_add_suite("Tests fonctions de commonlib.c", 0, 0);
	CU_add_test(suite_commonlib, "test1", test_commonlib);

	//Test commonlib
	CU_basic_run_suite(suite_commonlib);

	CU_cleanup_registry();
}
