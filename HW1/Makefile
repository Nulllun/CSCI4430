CC = gcc
CFLAG = -lpthread

all: server client

server: myftpserver.c
	$(CC) -o $@ $< $(CFLAG)

client: myftpclient.c
	$(CC) -o $@ $< $(CFLAG)

clean:
	rm myftpserver myftpclient 
