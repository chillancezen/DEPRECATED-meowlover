
#include <dsm.h>
#include <string.h>
#include <stdlib.h>

struct dsm_item * alloc_dsm_item_array(int nr_of_items)
{
	int index;
	struct dsm_item *array;
	array=malloc(sizeof(struct dsm_item)*nr_of_items);
	
	if(array){
		memset(array,0x0,sizeof(struct dsm_item)*nr_of_items);
		for(index=0;index<nr_of_items;index++){
			array[index].block_index=index;
		}
	}
	return array;
}

void dealloc_dsm_item_array(struct dsm_item *array)
{
	free(array);
}

struct dsm_memory * alloc_dsm_memory(char *node_name,int nr_of_items)
{
	struct dsm_memory * dsm=NULL;
	dsm=malloc(sizeof(struct dsm_memory));
	if(dsm){
		memset(dsm,0x0,sizeof(struct dsm_memory));
		strncpy(dsm->node_name,node_name,sizeof(dsm->node_name));
		dsm->nr_of_items=nr_of_items;
		dsm->items_array=alloc_dsm_item_array(nr_of_items);
		dsm->base=malloc(nr_of_items*(1<<CACHE_LINE_BIT));
		if(!dsm->items_array||!dsm->base)
			goto fails;
	}
	return dsm;
	fails:
		dealloc_dsm_memory(dsm);
		return NULL;
}

void dealloc_dsm_memory(struct dsm_memory* dsm)
{
	if(dsm->items_array)
		dealloc_dsm_item_array(dsm->items_array);
	if(dsm->base)
		free(dsm->base);
	free(dsm);
}



int read_dsm_memory_raw(struct dsm_memory * dsm,int start_block_index,int nr_of_blocks,char * buffer)
{
	int offset,length;
	if((start_block_index+nr_of_blocks)>dsm->nr_of_items)
		return -1;
	offset=start_block_index*(1<<CACHE_LINE_BIT);
	length=nr_of_blocks*(1<<CACHE_LINE_BIT);
	memcpy(buffer,offset+(char*)dsm->base,length);
	return 0;
}
int write_dsm_memory_raw(struct dsm_memory *dsm,int start_block_index ,int nr_of_blocks,char * buffer)
{
	int offset,length;
	if((start_block_index+nr_of_blocks)>dsm->nr_of_items)
		return -1;
	offset=start_block_index*(1<<CACHE_LINE_BIT);
	length=nr_of_blocks*(1<<CACHE_LINE_BIT);
	memcpy(offset+(char*)dsm->base,buffer,length);
	return 0;
}
int update_dsm_memory_version_with_max_number(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks)
	/*we update all the version numbers with the maximum based on the
	assumption that all the accessed blocks share the same attributes*/
{
	uint64_t max_number=0;
	int idx;
	if((start_block_index+nr_of_blocks)>dsm->nr_of_items)
		return -1;
	for(idx=start_block_index;idx<start_block_index+nr_of_blocks;idx++){
		if(max_number<dsm->items_array[idx].version_number)
			max_number=dsm->items_array[idx].version_number;
	}
	for(idx=start_block_index;idx<start_block_index+nr_of_blocks;idx++)
		dsm->items_array[idx].version_number=max_number+1;
	return 0;
}
int update_dsm_memory_version_with_specific_value(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks,uint64_t value)
{
	int idx;
	if((start_block_index+nr_of_blocks)>dsm->nr_of_items)
		return -1;
	for(idx=start_block_index;idx<start_block_index+nr_of_blocks;idx++)
		dsm->items_array[idx].version_number=value;
	return 0;
}
uint64_t maxmum_version_number(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks)
{
	uint64_t max_number=0;
	int idx;
	if((start_block_index+nr_of_blocks)>dsm->nr_of_items)
		return -1;
	for(idx=start_block_index;idx<start_block_index+nr_of_blocks;idx++){
		if(max_number<dsm->items_array[idx].version_number)
			max_number=dsm->items_array[idx].version_number;
	}
	return max_number;
}
int match_version_number(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks,uint64_t target_version)
{
	int idx;
	if((start_block_index+nr_of_blocks)>dsm->nr_of_items)
		return FALSE;
	for(idx=start_block_index;idx<start_block_index+nr_of_blocks;idx++)
		if(dsm->items_array[idx].version_number!=target_version)
			return FALSE;

	return TRUE;
}
uint64_t get_equal_version_number(struct dsm_memory*dsm,int start_block_idnex,int nr_of_blocks)
{
	uint64_t version_number=maxmum_version_number(dsm,start_block_idnex,nr_of_blocks);
	int rc=match_version_number(dsm,start_block_idnex,nr_of_blocks,version_number);
	if(rc==FALSE)
		version_number=0;
	return version_number;
}