CC = gcc
CFLAGS = -I../shared -Wall -O2 -Wno-deprecated-declarations
LDFLAGS = -lssl -lcrypto

all: client
client: client.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o client
clean:  
	rm -f client *.o

