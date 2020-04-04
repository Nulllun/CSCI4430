CC = gcc
LIB = -lpthread -lisal

all: 
	$(CC) -o myftpserver myftpserver.c $(LIB) 
	$(CC) -o myftpclient myftpclient.c $(LIB)

clean:
	rm myftpserver myftpclient 