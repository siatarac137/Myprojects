CC=gcc
CFLAGS=-I.
DEPS = hash.h  hsl_server.h  lib.h  linklist.h  message.h  network.h  pal_time.h  thread.h

%.o: %.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)
  
server: main.o hash.o  hsl_server.o  linklist.o  message.o  network.o thread.o
	$(CC) -g -o server main.o hash.o  hsl_server.o  linklist.o  message.o  network.o  thread.o

client: client.o
	$(CC) -g -o client client.o

client1: client1.o
	$(CC) -g -o client1 client1.o

clean:
	rm -rvf *.o server client client1

