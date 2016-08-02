#ifndef __VIRT_CACHE
#define __VIRT_CACHE

/*here memory is full-mapping cache*/
#include <virtbus_client.h>
#include <dsm.h>


struct local_memory{
	struct client_endpoint* cep;/*explcitly alloc and release*/
	struct dsm_memory *dsm;/*explcitly alloc and release*/
	int real_memory_size;
	void *local_memory_base;/*equal to local dsm memory base,but we  before and after we access these memory,we need synchronize with remote DSM using exported virtbus_client API*/
};

struct local_memory * local_memoory_alloc(char* remote_address,char*bus_name,int nr_of_blocks);
void local_memory_dealloc(struct local_memory*mem);
#endif

