CC = gcc
LIB = -lpthread
LIB2 = -lpthread -lsocket -lnsl

all: 
	@if [ "`uname -s`" = "Linux" ] ; then \
		make linux ; \
	else \
		make unix ; \
	fi

linux: 
	$(CC) -o myftpserver myftpserver.c $(LIB) 
	$(CC) -o myftpclient myftpclient.c $(LIB)

unix: 
	$(CC) -o myftpserver myftpserver.c $(LIB2) 
	$(CC) -o myftpclient myftpclient.c $(LIB2)

clean:
	rm myftpserver myftpclient 