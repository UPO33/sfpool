CC          = gcc
DFLAGS		= -g -ggdb
CFLAGS   	= -Wall -std=c99 -O2 
LDFLAGS		= -Wall
OBJ_FILES	= bin/main.o bin/sfpool.o
LIB_FILES	=
INCLUDE_PATH=

.PHONY:
	mkdir -p bin

bin/sfpool : $(OBJ_FILES)
	$(CC) $(LDFLAGS) $(LIBRARY_PATH) $(LIB_FILES) $(OBJ_FILES) -o bin/sfpool

bin/%.o : %.c
	$(CC) $(CFLAGS) $(DFLAGS) -c $(INCLUDE_PATH) $< -o $@

clean : 
	rm -rf bin
	mkdir -p bin
