HEADER=./include
CCFLAG+= -g3 -Wall -fpic
LDFLAG+= -lpthread
CC=gcc
LD=ld
SRC=$(wildcard src/*.c)
SRC+=$(wildcard client/*.c)

OBJ=$(patsubst %.c,%.o,$(SRC))


%.o:%.c
	$(CC) -I$(HEADER)   $(CCFLAG) -c -o $@ $<
libmeeeow.so:$(OBJ)
	gcc -shared -o libmeeeow.so  $(OBJ)
install:libmeeeow.so
	cp ./libmeeeow.so /lib64
uninstall:
	-rm -f /lib64/libmeeeow.so
clean:
	-rm -f ./test
	-rm -f libmeeeow.so
	find -name "*.o" -exec rm -f {} \;
