HEADER=./include
CCFLAG+= -g3 -Wall
LDFLAG+= -lpthread
CC=gcc
LD=ld
SRC=$(wildcard src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

%.o:%.c
	$(CC) -I$(HEADER)  $(CCFLAG) -c -o $@ $<
all:$(OBJ)
test:$(OBJ)
	$(CC) -o test $(LDFLAG)  $(OBJ)
clean:
	-rm -f ./test
	find -name "*.o" -exec rm -f {} \;
