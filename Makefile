# make for Semaphores

CC=gcc
CFLAGS=-Wall -lpthread -ggdb

ex: server client 
	$(CC) $(CFLAGS) server.c -o server
	$(CC) $(CFLAGS) client.c -o client


clean:
	rm -f *.o *.bak ex
