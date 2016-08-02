#include <virt_cpu.h>
#include <assert.h>
#include <string.h>


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#if 1
struct lockless_queue{/*all the filed will be modulo 32*/
	union{
		uint32_t  array_length;
		uint8_t common_cache_block[CACHE_LINE_SIZE];
	};
	
	union{
		struct{
			uint32_t prod_head;
			uint32_t prod_tail;
		};
		uint8_t prod_cache_block[CACHE_LINE_SIZE];
	};/*align to cache lines*/

	union{
		struct{
			uint32_t cons_head;
			uint32_t cons_tail;
		};
		uint8_t cons_cache_block[CACHE_LINE_SIZE];
	};
	uint64_t queue_array[0];
}__attribute__((packed));/*cache aligned to optmize performance*/
#else
struct lockless_queue{/*all the filed will be modulo 32*/
	union{
		uint32_t  array_length;
		uint8_t common_cache_block[CACHE_LINE_SIZE];
	};
	
	union{
		struct{
			uint32_t prod_head;
		};
		uint8_t prod_cache_block[CACHE_LINE_SIZE];
	};

	union{
		struct{
			uint32_t prod_tail;
		};
		uint8_t prod_cache_block1[CACHE_LINE_SIZE];
	};

	union{
		struct{
			uint32_t cons_head;
		};
		uint8_t cons_cache_block[CACHE_LINE_SIZE];
	};
	union{
		struct{
			uint32_t cons_tail;
		};
		uint8_t cons_cache_block1[CACHE_LINE_SIZE];
	};
	uint64_t queue_array[0];
}__attribute__((packed));/*cache aligned to optmize performance*/

#endif


void lockless_queue_init(struct virt_cpu*cpu,struct lockless_queue*queue,int queue_array_length)
{
	LOAD(cpu,&queue->array_length,sizeof(queue->array_length));
	queue->array_length=queue_array_length;
	STORE(cpu,&queue->array_length,sizeof(queue->array_length));

	LOAD(cpu,&queue->prod_head,CACHE_LINE_SIZE);
	queue->prod_head=0;
	queue->prod_tail=0;
	STORE(cpu,&queue->prod_head,CACHE_LINE_SIZE);


	LOAD(cpu,&queue->cons_head,CACHE_LINE_SIZE);
	queue->cons_head=0;
	queue->cons_tail=0;
	STORE(cpu,&queue->cons_head,CACHE_LINE_SIZE);
}

int enqueue_bulk(struct virt_cpu*cpu,struct lockless_queue*queue,uint64_t value[],int number)
{/*return value will be the realistic number that is put into the queue*/

	/*local regs */
	uint32_t local_prod_head;
	uint32_t local_prod_next;
	uint32_t local_cons_tail;
	int ret=0;
	int idx;
	int avail_room;
	int debug_last_head=-1;
	LOAD(cpu,&queue->array_length,sizeof(queue->array_length));
	start:
		
	LOAD(cpu,&queue->cons_tail,sizeof(queue->cons_tail));
	local_cons_tail=queue->cons_tail;

	LOAD(cpu,&queue->prod_head,sizeof(queue->prod_head));
	local_prod_head=queue->prod_head;

	if(debug_last_head!=-1){
		assert(debug_last_head==local_prod_head);
	}
	debug_last_head=local_prod_head;
	
	/*1.calculate available room*/
	idx=0;
	for(;idx<number;idx++){
		if(((local_prod_head+idx+1)%queue->array_length)==local_cons_tail)
			break;
	}
	avail_room=idx;
	if(!avail_room)
		return 0;

	local_prod_next=(local_prod_head+avail_room)%queue->array_length;
	/*2.try to update remote prod_head with local_prod_next*/
	queue->prod_head=local_prod_next;
	CMPXCHG(cpu,&queue->prod_head,sizeof(queue->prod_head));
	if(!CPU_REG5(cpu))
		goto start;/*already updated by other nodes,so restart this procedure*/

	/*3.copy queue entries*/
	/*LOAD(cpu,&queue->queue_array[local_prod_head],sizeof(uint64_t)*avail_room);*/
	#if 1
	for(idx=0;idx<avail_room;idx++){
		LOAD(cpu,&queue->queue_array[(idx+local_prod_head)%queue->array_length],sizeof(uint64_t));
		queue->queue_array[(idx+local_prod_head)%queue->array_length]=value[idx];
		STORE(cpu,&queue->queue_array[(idx+local_prod_head)%queue->array_length],sizeof(uint64_t));
	}
	#endif
	/*STORE(cpu,&queue->queue_array[local_prod_head],sizeof(uint64_t)*avail_room);*/

	end:
	LOAD(cpu,&queue->prod_tail,sizeof(queue->prod_tail));
	if(local_prod_head!=queue->prod_tail)
		goto end;
	queue->prod_tail=local_prod_next;
	CMPXCHG(cpu,&queue->prod_tail,sizeof(queue->prod_tail));/*use STORE instead of CMPXHG ,because some node my update version number when they prefetch pro_head*/
	if(!CPU_REG5(cpu))
		goto end;
	ret=avail_room;
	return ret;
	
}
int dequeue_bulk(struct virt_cpu*cpu,struct lockless_queue*queue,uint64_t value[],int number)
{
	int32_t local_cons_head;
	int32_t local_cons_next;
	int32_t local_prod_tail;
	int rc;
	int idx=0;
	int avail_quantum;
	LOAD(cpu,&queue->array_length,sizeof(queue->array_length));
	
	start:
		LOAD(cpu,&queue->prod_tail,sizeof(queue->prod_tail));
		local_prod_tail=queue->prod_tail;

		LOAD(cpu,&queue->cons_head,sizeof(queue->cons_head));
		local_cons_head=queue->cons_head;

		/*calculate available element quantum*/
		for(idx=0;idx<number;idx++){
			if((local_cons_head+idx)%queue->array_length==local_prod_tail)
				break;
		}
		avail_quantum=idx;
		if(!avail_quantum)
			return 0;
		local_cons_next=(local_cons_head+avail_quantum)%queue->array_length;
		/*updtae remote cons_head with local_cons_next*/
		queue->cons_head=local_cons_next;
		CMPXCHG(cpu,&queue->cons_head,sizeof(queue->cons_head));
		if(!CPU_REG5(cpu)){
			
			goto start;
		}
		/*copy queue elements into local buffer*/
		#if 1
		for(idx=0;idx<avail_quantum;idx++){
			LOAD(cpu,&queue->queue_array[(local_cons_head+idx)%queue->array_length],sizeof(uint64_t));
			value[idx]=queue->queue_array[(local_cons_head+idx)%queue->array_length];
		}
		#endif
	end:
		LOAD(cpu,&queue->cons_tail,sizeof(queue->cons_tail));
		if(queue->cons_tail!=local_cons_head){
			goto end;
		}
		queue->cons_tail=local_cons_next;
		CMPXCHG(cpu,&queue->cons_tail,sizeof(queue->cons_tail));
		if(!CPU_REG5(cpu))
			goto end;
		
	return avail_quantum;
		
}
int main(int argc,char**argv)
{
	struct virt_cpu*cpu=create_virt_cpu();
	struct lockless_queue*queue;
	printf("size :%d\n",sizeof(struct lockless_queue));
	assert(cpu);
	int count=0;
	int zero_count=0;
	uint64_t number[500];
	int rc;
	cpu->cache=local_memoory_alloc("192.168.139.134","/root/lockless_queue_bus",1024);
	assert(cpu->cache);
	queue=cpu->cache->local_memory_base;
	assert(argc>=2);
	if(argc>2){
		puts("[x] queue init");
		lockless_queue_init(cpu,queue,13);
	}
	
	if(!strcmp(argv[1],"prod")){
		#if 0
		LOAD(cpu,&queue->array_length,4);
		queue->array_length=44;
		getchar();
		CMPXCHG(cpu,&queue->array_length,4);
		printf("cpu reg5:%d\n",CPU_REG5(cpu));
		getchar();
		LOAD(cpu,&queue->array_length,4);
		printf("value:%d\n",queue->array_length);
		
		return 0;
		#endif
		while(TRUE){
			rc=enqueue_bulk(cpu,queue,number,5);
			if(!rc)
				zero_count++;
			count+=rc;
			printf("enqueue:%d(%d).....%d\n",count,rc,zero_count);
			getchar();
		}
	}else{
		#if 0
		LOAD(cpu,&queue->array_length,4);
		queue->array_length=55;
		getchar();
		STORE(cpu,&queue->array_length,4)

		return 0;
		#endif
		while(TRUE){
			rc=dequeue_bulk(cpu,queue,number,1);
			count+=rc;
			if(rc)
			printf("dequeue:%d\n",count);
		}
	}
	return 0;
}
