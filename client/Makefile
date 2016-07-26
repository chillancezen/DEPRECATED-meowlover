OTHER_OBJS=$(wildcard ../src/*.o)

all:
	gcc -o client_demo -I../include -I. client.c client_demo.c `echo $(OTHER_OBJS)|sed "s/..\/src\/test.o//g"`
clean:
	-rm -rf  ./client_dmeo
