CC = gcc
CFLAGS = -I../shared -Wall -O2 -Wno-deprecated-declarations
LDFLAGS = -lpthread -lssl -lcrypto

all: server
server: server.c handler.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o server
clean: 
	rm -f server *.o

