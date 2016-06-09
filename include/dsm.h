#ifndef __DSM
#define __DSM
#include <stdint.h>
#include <errorcode.h>
#define CACHE_LINE_BIT  6



struct dsm_item{
	union{
	uint32_t  block_address;
	struct {
		uint32_t block_offset:CACHE_LINE_BIT;
		uint32_t block_index:32-CACHE_LINE_BIT;
	};
	};
	uint64_t version_number;
};

struct dsm_memory{
	char node_name[256];
	void * base;
	int nr_of_items;
	struct dsm_item * items_array;
};



struct dsm_item * alloc_dsm_item_array(int nr_of_items);
void dealloc_dsm_item_array(struct dsm_item *array);
struct dsm_memory * alloc_dsm_memory(char *node_name,int nr_of_items);

void dealloc_dsm_memory(struct dsm_memory* dsm);
int read_dsm_memory_raw(struct dsm_memory * dsm,int start_block_index,int nr_of_blocks,char * buffer);
int write_dsm_memory_raw(struct dsm_memory *dsm,int start_block_index,int nr_of_blocks,char * buffer);
int update_dsm_memory_version_with_max_number(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks);
int update_dsm_memory_version_with_specific_value(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks,uint64_t value);
uint64_t maxmum_version_number(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks);
int match_version_number(struct dsm_memory*dsm,int start_block_index,int nr_of_blocks,uint64_t target_version);
#endif

