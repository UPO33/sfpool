CC          = gcc
DFLAGS		= -g -ggdb
CFLAGS   	= -Wall -std=c99 -O2 -fpic
LDFLAGS		= -Wall
OBJ_FILES	= bin/sfpool.o
LIB_FILES	=
INCLUDE_PATH=

all: main bin/libsfpool.so

main:
	mkdir -p bin

bin/libsfpool.so : $(OBJ_FILES)
	$(CC) $(LDFLAGS) -shared $(LIBRARY_PATH) $(LIB_FILES) $(OBJ_FILES) -o bin/libsfpool.so

bin/%.o : %.c
	$(CC) $(CFLAGS) $(DFLAGS) -c $(INCLUDE_PATH) $< -o $@

clean : 
	rm -rf bin
	mkdir -p bin

install:
	cp -rf sfpool.h /usr/include
	cp -rf bin/libsfpool.so /usr/lib/
