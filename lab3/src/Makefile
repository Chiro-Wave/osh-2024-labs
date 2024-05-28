CC = gcc
CFLAGS = -Wall -g

all: server server_select

server: server.o thread.o
	$(CC) $(CFLAGS) -o server server.o thread.o

server_select: server_select.o
	$(CC) $(CFLAGS) -o server_select server_select.o

server.o: server.c server.h thread.h
	$(CC) $(CFLAGS) -c server.c

thread.o: thread.c server.h thread.h
	$(CC) $(CFLAGS) -c thread.c

server_select.o: server_select.c server.h
	$(CC) $(CFLAGS) -c server_select.c

clean:
	rm -f *.o server server_select
