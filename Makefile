HEADER=./include
CCFLAG+= -g3 -Wall
CC=gcc

SRC=$(wildcard src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

%.o:%.c
	$(CC) -I$(HEADER) $(CCFLAG) -c -o $@ $<
all:$(OBJ)
test:$(OBJ)
	$(CC) -o test $(OBJ)
clean:
	-rm -f ./test
	find -name "*.o" -exec rm -f {} \;
