INCLUDE=-I../include
LIB=-L../


all:
	mkdir -p bin
	gcc -o bin/virt_computer -lmeeeow $(INCLUDE) $(LIB) virt_computer.c
	gcc -o bin/client_demo -lmeeeow $(INCLUDE) $(LIB) client_demo.c
	gcc -o bin/spinlock  -lmeeeow $(INCLUDE) $(LIB) spinlock.c
	gcc -o bin/semaphore -lmeeeow $(INCLUDE) $(LIB) semaphore.c
	gcc -o bin/lockless-queue -lmeeeow $(INCLUDE) $(LIB) lockless-queue.c 
clean:
	-rm -rf  ./bin/*
