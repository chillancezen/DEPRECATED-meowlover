#include <virt_cpu.h>
#include <assert.h>
#include <stdint.h>

struct spinlock{
	uint32_t ref_count;
};
void spin_lock(struct virt_cpu*cpu,struct spinlock*lock)
{/*not recursively designed,doing so may cause dead-lock,
and during the current circumstance,if the node who has already got the lock is disconnected to server before releasing lock,
other nodes which are attempting to acquire lock may trap into dead-lock,this deficiency is very easy to testify and turns out to be true,so this would be very dangerous,one solution is :
we deliver the determination of whether to continue polling on the lock reference count to function caller ,which means that the possible
function my be spin_try_lock_demo()*/
	
	while(TRUE){
		LOAD(cpu,lock,sizeof(struct spinlock));
		if(lock->ref_count)
			continue;
		/*try to increase the reference count*/
		lock->ref_count++;
		CMPXCHG(cpu,lock,sizeof(struct spinlock));
		if(CPU_REG5(cpu)){/*lock already acquired */
			break;
		}else{
			lock->ref_count--;/*restore to the original*/
		}
	}
}
void spin_unlock(struct virt_cpu*cpu,struct spinlock*lock)
{
	lock->ref_count--;
	STORE(cpu,lock,sizeof(struct spinlock));
	
}
int main(int argc,char**argv)
{
	struct virt_cpu *cpu;
	cpu=create_virt_cpu();
	assert(cpu);
	int count=0;
	cpu->cache=local_memoory_alloc("192.168.139.148","/root/spinlock_bus",64);
	assert(cpu->cache);

	struct spinlock *lock=cpu->cache->local_memory_base;
	if(argc!=1){/*an indicator to say:initialize the spin-lock*/
		LOAD(cpu,lock,sizeof(struct spinlock));
		lock->ref_count=0;
		STORE(cpu,lock,sizeof(struct spinlock));
	}
	
	while(TRUE){
		spin_lock(cpu,lock);
		count++;
		printf("acquires:%d\n",count);
		getchar();
		spin_unlock(cpu,lock);
	}
	return 0;
}

