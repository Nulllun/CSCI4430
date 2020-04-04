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

#define HEADER_LEN 10
#define PORT 12345
#define IPADDR "127.0.0.1"

struct message_s {
    unsigned char protocol[5];
    unsigned char type;
    unsigned int length;
} __attribute__ ((packed));
