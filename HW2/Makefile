CC = gcc
LIB = -lm -lpthread -lisal

all: 
	$(CC) -o myftpserver myftpserver.c $(LIB) 
	$(CC) -o myftpclient myftpclient.c $(LIB)
	
send:
	scp myftpserver 10.0.3.2:~/testSpace/0
	scp myftpserver 10.0.3.2:~/testSpace/1
	scp myftpserver 10.0.3.2:~/testSpace/2
	scp myftpserver 10.0.3.2:~/testSpace/3
	scp myftpserver 10.0.3.2:~/testSpace/4

clean:
	rm myftpserver myftpclient 