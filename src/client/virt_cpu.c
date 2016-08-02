#include <virt_cpu.h>

struct virt_cpu* create_virt_cpu()
{
	struct virt_cpu* cpu=NULL;
	cpu=(struct virt_cpu*)malloc(sizeof(struct virt_cpu));
	if(cpu){
		memset(cpu,0x0,sizeof(struct virt_cpu));
	}
	return cpu;
}
void destroy_virt_cpu(struct virt_cpu*cpu)
{
	if(!cpu)
		return;
	if(cpu->cache)
		local_memory_dealloc(cpu->cache);
	free(cpu);
		
}

void virt_cpu_load(struct virt_cpu*cpu)
{/*
INPUT operand:
	REG0:address
	REG1:size 
OUTPUT:
	REG4:non-zero indicates success*/
	void *start_address=(void*)cpu->reg0;
	void *end_address;
	int size=(int)cpu->reg1;
	int start_block_index;
	int nr_of_blocks;
	int rc;
	uint64_t version_number;
	int is_modified;
	
	if(((size+(char*)start_address)>(cpu->cache->real_memory_size+(char*)cpu->cache->local_memory_base))||((char*)start_address<(char*)cpu->cache->local_memory_base)){
		CLEAR_REG4(cpu);
		return;
	}

	end_address=(void*)(size+(char*)start_address);
	start_block_index=((int)((char*)start_address-(char*)cpu->cache->local_memory_base))>>CACHE_LINE_BIT;
	size+=((int)((char*)start_address-(char*)cpu->cache->local_memory_base))%CACHE_LINE_SIZE;
	nr_of_blocks=size/CACHE_LINE_SIZE+((size%CACHE_LINE_SIZE)?1:0);

	version_number=get_equal_version_number(cpu->cache->dsm,start_block_index,nr_of_blocks);
	rc=client_endpoint_read_virtual_bus_matched(cpu->cache->cep,
		start_block_index,
		nr_of_blocks,
		start_block_index*CACHE_LINE_SIZE+(char*)cpu->cache->local_memory_base,
		&version_number,
		&is_modified);
	if(rc){
		CLEAR_REG4(cpu);
		return;
	}
	
	if(is_modified){
		update_dsm_memory_version_with_specific_value(cpu->cache->dsm,start_block_index,nr_of_blocks,version_number);
	}
	SET_REG4(cpu,!0);
}
void virt_cpu_store(struct virt_cpu *cpu)
{
	void *start_address=(void*)cpu->reg0;
	void *end_address;
	int size=(int)cpu->reg1;
	int start_block_index;
	int nr_of_blocks;
	int rc;
	uint64_t version_number;
	if(((size+(char*)start_address)>(cpu->cache->real_memory_size+(char*)cpu->cache->local_memory_base))||((char*)start_address<(char*)cpu->cache->local_memory_base)){
		CLEAR_REG4(cpu);
		return;
	}
	end_address=(void*)(size+(char*)start_address);
	start_block_index=((int)((char*)start_address-(char*)cpu->cache->local_memory_base))>>CACHE_LINE_BIT;
	size+=((int)((char*)start_address-(char*)cpu->cache->local_memory_base))%CACHE_LINE_SIZE;
	nr_of_blocks=size/CACHE_LINE_SIZE+((size%CACHE_LINE_SIZE)?1:0);

	version_number=get_equal_version_number(cpu->cache->dsm,start_block_index,nr_of_blocks);
	rc=client_endpoint_write_virtual_bus_generic(cpu->cache->cep,
		start_block_index,
		nr_of_blocks,
		start_block_index*CACHE_LINE_SIZE+(char*)cpu->cache->local_memory_base,
		&version_number);
	if(rc){
		CLEAR_REG4(cpu);
		return;
	}
	
	update_dsm_memory_version_with_specific_value(cpu->cache->dsm,start_block_index,nr_of_blocks,version_number);
	SET_REG4(cpu,!0);
}
void virt_cpu_cmpxchg(struct virt_cpu*cpu)
{
	void *start_address=(void*)cpu->reg0;
	void *end_address;
	int size=(int)cpu->reg1;
	int start_block_index;
	int nr_of_blocks;
	int rc;
	uint64_t version_number;
	int is_matched;
	if(((size+(char*)start_address)>(cpu->cache->real_memory_size+(char*)cpu->cache->local_memory_base))||((char*)start_address<(char*)cpu->cache->local_memory_base)){
		CLEAR_REG4(cpu);
		return;
	}
	end_address=(void*)(size+(char*)start_address);
	start_block_index=((int)((char*)start_address-(char*)cpu->cache->local_memory_base))>>CACHE_LINE_BIT;
	size+=((int)((char*)start_address-(char*)cpu->cache->local_memory_base))%CACHE_LINE_SIZE;
	nr_of_blocks=size/CACHE_LINE_SIZE+((size%CACHE_LINE_SIZE)?1:0);

	version_number=get_equal_version_number(cpu->cache->dsm,start_block_index,nr_of_blocks);
	rc=client_endpoint_write_virtual_bus_matched(cpu->cache->cep,
		start_block_index,
		nr_of_blocks,
		start_block_index*CACHE_LINE_SIZE+(char*)cpu->cache->local_memory_base,
		&is_matched,
		&version_number);
	if(rc){
		CLEAR_REG4(cpu);
		return;
	}
	SET_REG4(cpu,!0);
	
	if(is_matched){
		SET_REG5(cpu,!0);
		update_dsm_memory_version_with_specific_value(cpu->cache->dsm,start_block_index,nr_of_blocks,version_number);
	}else{
		CLEAR_REG5(cpu);
	}
	
}