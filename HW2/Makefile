CC = gcc
LIB = -lpthread

all: 
	$(CC) -o myftpserver myftpserver.c $(LIB) 
	$(CC) -o myftpclient myftpclient.c $(LIB)
	cp myftpserver testSpace/1
	cp serverconfig.txt testSpace/1
	
	cp myftpserver testSpace/2
	cp serverconfig.txt testSpace/2

	cp myftpserver testSpace/3
	cp serverconfig.txt testSpace/3

	cp myftpserver testSpace/4
	cp serverconfig.txt testSpace/4

	cp myftpserver testSpace/5
	cp serverconfig.txt testSpace/5



clean:
	rm myftpserver myftpclient 