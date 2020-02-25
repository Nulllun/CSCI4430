CC = gcc
CFLAG = -lpthread

all: myftpserver myftpclient

myftpserver: myftpserver.c
	$(CC) -o $@ $< $(CFLAG)

myftpclient: myftpclient.c
	$(CC) -o $@ $< $(CFLAG)

clean:
	rm myftpserver myftpclient 
