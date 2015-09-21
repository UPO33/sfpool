CC          = gcc
DFLAGS		= -g -ggdb
CFLAGS   	= -Wall -std=c99 -O2 
LDFLAGS		= -Wall
OBJ_FILES	= bin/test.o bin/sfpool.o
LIB_FILES	=
INCLUDE_PATH=

.PHONY:
	mkdir -p bin

bin/test : $(OBJ_FILES)
	$(CC) $(LDFLAGS) $(LIBRARY_PATH) $(LIB_FILES) $(OBJ_FILES) -o bin/test

bin/%.o : %.c
	$(CC) $(CFLAGS) $(DFLAGS) -c $(INCLUDE_PATH) $< -o $@

clean : 
	rm -rf bin
	mkdir -p bin
