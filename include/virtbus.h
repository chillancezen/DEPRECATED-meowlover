#ifndef __VIRTBUS
#define __VIRTBUS
#include <dsm.h>
#include <pthread.h>

/*when I firstly desgin the virtual bus here ,I use a single thraed which needs no locking*/

struct virtual_bus
{
	struct dsm_memory *dsm;

	int ref_counnt;
	/*locking is preserved in case I need to implement bus logic in multi-thraed context*/
	int bus_is_locked:1;
};
struct virtual_bus  *alloc_virtual_bus(char * bus_name,int nr_of_blocks);
void dealloc_virtual_bus(struct virtual_bus *bus);
void bus_lock(struct virtual_bus * bus);
int bus_try_lock(struct virtual_bus *bus);
void bus_unlock(struct virtual_bus *bus);
int issue_bus_read_raw(struct virtual_bus *bus,int start_block_index,int nr_of_blocks,char *buffer);
int issue_bus_read_matched(struct virtual_bus * bus ,int start_block_index,int nr_of_blocks,char *buffer,uint64_t *target_version);
int issue_bus_write_raw(struct virtual_bus *bus,int start_block_index,int nr_of_blocks,char *buffer);
//int issue_bus_write_matched(struct virtual_bus * bus,int start_block_index,int nr_of_blocks,char *buffer,uint64_t target_version);
//int issue_bus_write_generic(struct virtual_bus * bus,int start_block_index,int nr_of_blocks,char *buffer);
int issue_bus_write_matched(struct virtual_bus * bus,int start_block_index,int nr_of_blocks,char *buffer,uint64_t * target_version);
int issue_bus_write_generic(struct virtual_bus * bus,int start_block_index,int nr_of_blocks,char *buffer,uint64_t * target_version);


#endif

