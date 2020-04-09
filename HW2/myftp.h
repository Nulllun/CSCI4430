#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <isa-l.h>


#define HEADER_LEN 10

struct message_s {
    unsigned char protocol[5];
    unsigned char type;
    unsigned int length;
} __attribute__ ((packed));
