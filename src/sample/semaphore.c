#include <virt_cpu.h>
#include <assert.h>
#include <string.h>

struct semaphore{
	int signal;
};
void semaphore_release(struct virt_cpu*cpu,struct semaphore * sema,int signal)
{
	
	while(TRUE){
		LOAD(cpu,sema,sizeof(struct semaphore));
		sema->signal+=signal;
		CMPXCHG(cpu,sema,sizeof(struct semaphore));
		if(CPU_REG5(cpu)){
			break;
		}else{
			sema->signal-=signal;
		}
	}

}
void semaphore_wait_one(struct virt_cpu*cpu,struct semaphore*sema)
{
	while(TRUE){
		LOAD(cpu,sema,sizeof(struct semaphore));
		if(sema->signal<=0)
			continue;
		sema->signal--;
		CMPXCHG(cpu,sema,sizeof(struct semaphore));
		if(CPU_REG5(cpu)){
			break;
		}else{
			sema->signal++;
		}

	}
}
int main(int argc,char**argv)
{
	struct virt_cpu*cpu=create_virt_cpu();
	struct semaphore* sema;
	assert(cpu);
	int count=0;
	cpu->cache=local_memoory_alloc("192.168.139.148","/root/semaphore_bus",64);
	assert(cpu->cache);
	assert(argc>=2);
	sema=(struct semaphore*)cpu->cache->local_memory_base;
	if(argc>2){
		LOAD(cpu,sema,sizeof(struct semaphore));
		sema->signal=0;
		STORE(cpu,sema,sizeof(struct semaphore));
	}
	if(!strcmp(argv[1],"prod")){
		while(TRUE){
			semaphore_release(cpu,sema,300);
			count+=300;
			printf("release total:%d\n",count);
			getchar();
		}
	}else{
		while(TRUE){
			semaphore_wait_one(cpu,sema);
			count++;
			printf("wait total:%d\n",count);
			//sleep(1);
		}
	}
	return 0;
}
