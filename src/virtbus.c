#include <virtbus.h>
#include <malloc.h>
#include <string.h>
struct virtual_bus  *alloc_virtual_bus(char * bus_name,int nr_of_blocks)
{
	struct virtual_bus * bus;
	bus=malloc(sizeof(struct virtual_bus));
	if(bus){
		memset(bus,0x0,sizeof(struct virtual_bus));
		bus->dsm=alloc_dsm_memory(bus_name,nr_of_blocks);
		if(!bus->dsm)
			goto fails;
		bus->ref_counnt=0;
	}
	return bus;
	fails:
		dealloc_virtual_bus(bus);
		return NULL;
}

void dealloc_virtual_bus(struct virtual_bus *bus)
{
	if(bus->dsm)
		dealloc_dsm_memory(bus->dsm);
	if(bus->ref_counnt)
		printf("bug:reference count not equal 0\n");
	
	free(bus);
}
void bus_lock(struct virtual_bus * bus)
{

}
int bus_try_lock(struct virtual_bus *bus)
{

	return TRUE;
}
void bus_unlock(struct virtual_bus *bus)
{
	
}
int issue_bus_read_raw(struct virtual_bus *bus,int start_block_index,int nr_of_blocks,char *buffer)
{/*only copy the dsm into the buffer,and do not update version number*/
	int rc;
	rc=read_dsm_memory_raw(bus->dsm,start_block_index,nr_of_blocks,buffer);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	return VIRTUAL_BUS_OK;
}
int issue_bus_read_matched(struct virtual_bus * bus ,int start_block_index,int nr_of_blocks,char *buffer,uint64_t *target_version)
{/*in this read operaytion,if the cache blocks match the target_version,then we just tell the caller that the data not modified, otherwise 
we copy them into buffer regularly ,and update their version number with maximum version*/
	int rc;
	uint64_t max_number;
	
	if(match_version_number(bus->dsm,start_block_index,nr_of_blocks,*target_version))
		return -VIRTUAL_BUS_MATCHED_VERSION;
	rc=read_dsm_memory_raw(bus->dsm,start_block_index,nr_of_blocks,buffer);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	max_number=maxmum_version_number(bus->dsm,start_block_index,nr_of_blocks);
	rc=update_dsm_memory_version_with_specific_value(bus->dsm,start_block_index,nr_of_blocks,max_number);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	*target_version=max_number;
	return VIRTUAL_BUS_OK;
}

int issue_bus_write_raw(struct virtual_bus *bus,int start_block_index,int nr_of_blocks,char *buffer)
{
	int rc=0;
	rc=write_dsm_memory_raw(bus->dsm,start_block_index,nr_of_blocks,buffer);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	return VIRTUAL_BUS_OK;
}

int issue_bus_write_matched(struct virtual_bus * bus,int start_block_index,int nr_of_blocks,char *buffer,uint64_t * target_version)
{/*if version is matched ,write them and update version normally ,otherwise ,return errorcode*/
	int rc;
	if(!match_version_number(bus->dsm, start_block_index, nr_of_blocks,*target_version))
		return -VIRTUAL_BUS_VERSION_NOT_MATCH;
	rc=write_dsm_memory_raw(bus->dsm,start_block_index,nr_of_blocks,buffer);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	rc=update_dsm_memory_version_with_specific_value(bus->dsm,start_block_index,nr_of_blocks,*target_version+1);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	*target_version=*target_version+1;
	return VIRTUAL_BUS_OK;
}
int issue_bus_write_generic(struct virtual_bus * bus,int start_block_index,int nr_of_blocks,char *buffer,uint64_t * target_version)
{/*anyway,we just update the version number with the maximum numebr plus one*/
	int rc;
	rc=write_dsm_memory_raw(bus->dsm,start_block_index,nr_of_blocks,buffer);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	rc=update_dsm_memory_version_with_max_number(bus->dsm,start_block_index,nr_of_blocks);
	if(rc==-1)
		return -VIRTUAL_BUS_ERROR_INVALID_ARG;
	*target_version=maxmum_version_number(bus->dsm,start_block_index,nr_of_blocks);
	
	return VIRTUAL_BUS_OK;
}