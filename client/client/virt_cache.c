#include <virt_cache.h>
struct local_memory * local_memoory_alloc(char* remote_address,char*bus_name,int nr_of_blocks)
{
	struct local_memory *ret=NULL;
	int rc;
	ret=(struct local_memory*)malloc(sizeof(struct local_memory));
	if(ret){
		memset(ret,0x0,sizeof(struct local_memory));
		/*1.setup virtbus client*/
		ret->cep=client_endpoint_alloc_and_init(remote_address);
		if(!ret->cep)
			goto error;
		rc=client_endpoint_connect_to_virtual_bus(ret->cep,bus_name,nr_of_blocks);
		if(rc){
			client_endpoint_uninit_and_dealloc(ret->cep);
			goto error;
		}
		/*2.setup local dsm structure*/
		
		ret->dsm=alloc_dsm_memory(bus_name,ret->cep->nr_of_blocks);

		if(!ret->dsm){
			client_endpoint_uninit_and_dealloc(ret->cep);
			goto error;
		}
		ret->real_memory_size=ret->cep->nr_of_blocks*CACHE_LINE_SIZE;
		ret->local_memory_base=ret->dsm->base;
		
		/*printf("[x] real number of blocks:%d %d\n",ret->cep->nr_of_blocks,ret->real_memory_size);*/
		
	}
	
	return ret;
	error:
		if(ret)
			free(ret);
		return NULL;
}

void local_memory_dealloc(struct local_memory*mem)
{
	if(!mem)
		return ;
	if(mem->dsm)
		dealloc_dsm_memory(mem->dsm);
	if(mem->cep)
		client_endpoint_uninit_and_dealloc(mem->cep);
	free(mem);
}
