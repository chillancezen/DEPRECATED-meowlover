#ifndef __VIRT_CPU
#define __VIRT_CPU
#include <virt_cache.h>
#include <assert.h>
struct virt_cpu{
	struct local_memory * cache;
	uint64_t reg0;
	uint64_t reg1;
	uint64_t reg2;
	uint64_t reg3;
	uint64_t reg4;/*common ops indicator*/
	uint64_t reg5;/*sub indicator*/
	uint64_t reg6;
	uint64_t reg7;
};
struct virt_cpu* create_virt_cpu();
void destroy_virt_cpu(struct virt_cpu*cpu);
void virt_cpu_load(struct virt_cpu*cpu);
void virt_cpu_store(struct virt_cpu *cpu);

#define CLEAR_REG0(cpu) {\
	(cpu)->reg0=0;\
}
#define CLEAR_REG1(cpu) {\
	(cpu)->reg1=0;\
}
#define CLEAR_REG2(cpu) {\
	(cpu)->reg2=0;\
}
#define CLEAR_REG3(cpu) {\
	(cpu)->reg3=0;\
}
#define CLEAR_REG4(cpu) {\
	(cpu)->reg4=0;\
}
#define CLEAR_REG5(cpu) {\
	(cpu)->reg5=0;\
}
#define CLEAR_REG6(cpu) {\
	(cpu)->reg6=0;\
}
#define CLEAR_REG7(cpu) {\
	(cpu)->reg7=0;\
}


#define SET_REG0(cpu,val) {\
	(cpu)->reg0=(uint64_t)(val);\
}
#define SET_REG1(cpu,val) {\
	(cpu)->reg1=(uint64_t)(val);\
}
#define SET_REG2(cpu,val) {\
	(cpu)->reg2=(uint64_t)(val);\
}
#define SET_REG3(cpu,val) {\
	(cpu)->reg3=(uint64_t)(val);\
}
#define SET_REG4(cpu,val) {\
	(cpu)->reg4=(uint64_t)(val);\
}
#define SET_REG5(cpu,val) {\
	(cpu)->reg5=(uint64_t)(val);\
}
#define SET_REG6(cpu,val) {\
	(cpu)->reg6=(uint64_t)(val);\
}
#define SET_REG7(cpu,val) {\
	(cpu)->reg7=(uint64_t)(val);\
}


#define CPU_REG0(cpu)  ((cpu)->reg0)
#define CPU_REG1(cpu)  ((cpu)->reg1)
#define CPU_REG2(cpu)  ((cpu)->reg2)
#define CPU_REG3(cpu)  ((cpu)->reg3)
#define CPU_REG4(cpu)  ((cpu)->reg4)
#define CPU_REG5(cpu)  ((cpu)->reg5)
#define CPU_REG6(cpu)  ((cpu)->reg6)
#define CPU_REG7(cpu)  ((cpu)->reg7)









#define LOAD(cpu,addr,size) {\
	SET_REG0((cpu),(addr));\
	SET_REG1((cpu),(size));\
	virt_cpu_load(cpu);\
	assert(CPU_REG4(cpu));\
}

#define STORE(cpu,addr,size) {\
	SET_REG0((cpu),(addr));\
	SET_REG1((cpu),(size));\
	CLEAR_REG2((cpu));\
	virt_cpu_store(cpu);\
	assert(CPU_REG4(cpu));\
}
#define CMPXCHG(cpu,addr,size) {\
	SET_REG0((cpu),(addr));\
	SET_REG1((cpu),(size));\
	SET_REG2((cpu),!0);\
	virt_cpu_cmpxchg(cpu);\
	assert(CPU_REG4(cpu));\
}

#endif
