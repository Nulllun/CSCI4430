CC = gcc
LIB = -lpthread
LIB2 = -lpthread -lsocket -lnsl

all: 
	@if [ "`uname -s`" = "Linux" ] ; then \
		make myftpclientlinux myftpserverlinux ; \
	else \
		make myftpclientunix myftpserverunix ; \
	fi

myftpserverlinux: 
	$(CC) -o myftpserver myftpserver.c $(LIB)

myftpclientlinux: 
	$(CC) -o myftpclient myftpclient.c $(LIB)

myftpserverunix: 
	$(CC) -o myftpserver myftpserver.c $(LIB2)

myftpclientunix: 
	$(CC) -o myftpclient myftpclient.c $(LIB2)

clean:
	rm myftpserver myftpclient 