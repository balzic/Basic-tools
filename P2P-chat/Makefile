CC = gcc
CFLAGS =-Wall -Wextra -pedantic -std=gnu11 -g
LD_FLAGS = -lpthread

all : peer name_server

rebuild : clean all

csapp.o : csapp.c csapp.h
	$(CC) $(CFLAGS) -c $< -o $@

buffer.o : buffer.c buffer.h csapp.h
	$(CC) $(CFLAGS) -c $< -o $@

peer : peer.c peer.h csapp.o buffer.o
	$(CC) $(CFLAGS) $< *.o -o $@ $(LD_FLAGS)

name_server : name_server.c name_server.h csapp.o
	$(CC) $(CFLAGS) $< *.o -o $@ $(LD_FLAGS)

clean :
	rm -rf *.o peer name_server

../src.zip :
	make clean
	cd .. && zip src.zip -r src
