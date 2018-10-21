#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../src/window.c"
#include "../src/packet.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

int n=1;

void test_window()
{
	int length = 10;
	window_t* test = window_new(length);
	CU_ASSERT(test != NULL);
	CU_ASSERT(test->length == length && test->size_used == 0 && test->buffer != NULL);
	pkt_t* pktest = pkt_new();
	node_t* nodetest = node_new(pktest);
	CU_ASSERT(nodetest!=NULL);
	CU_ASSERT(nodetest->pkt == pktest && nodetest->seqnum == pkt_get_seqnum(pktest));
	CU_ASSERT(window_add(test, pktest) >= 0);
	CU_ASSERT(test->length == length && test->size_used == 1 && test->buffer != NULL);
	node_del(nodetest);
	CU_ASSERT(test->size_used == 0 && test->buffer != NULL);
	window_del(test);
}

int main (int argc, char* argv[])
{
	CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("test_window", 0, 0);
  CU_add_test(suite, "test2", test_window);
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
	CU_cleanup_registry();
}
