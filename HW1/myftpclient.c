# include <myftp.h>
# include <myftp.c>

int recvMsg(int sd, char *buff, int len) {
    int recvLen = 0;
    while (recvLen != len) {
        int rLen = recv(sd, buff + recvLen, len - recvLen, 0);
        if (rLen <= 0) {
            fprintf(stderr, "error recving msg\n");
            return 1;
        }
        recvLen += rLen;
    }
    return 0;
}

int sendMsg(int sd, char *buff, int len) {
    int recvLen = 0;
    while (recvLen != len) {
        int rLen = send(sd, buff + recvLen, len - recvLen, 0);
        if (rLen <= 0) {
            fprintf(stderr, "error sending msg\n");
            return 1;
        }
        recvLen += rLen;
    }
    return 0;
}

/*
 * Worker thread performing receiving and outputing messages
 */
void *pthread_prog(void *sDescriptor) {
    int sd = *(int *)sDescriptor;
    int *len = (int *)calloc(sizeof(int), 1);
    while (1) {
        if (recvMsg(sd, (char *)len, sizeof(int)) == 1) {
            fprintf(stderr, "error receiving, exit!\n");
            exit(0);
        }
        char *buff = (char *)calloc(sizeof(char), *len + 1);
        if (recvMsg(sd, buff, *len) == 1) {
            fprintf(stderr, "error receiving, exit!\n");
            exit(0);
        }
        printf("recv'd msg: %s\n", buff);
        free(buff);
    }
    free(len);
}

int main(int argc, char **argv) {
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    pthread_t worker;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IPADDR);
    server_addr.sin_port = htons(PORT);
    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("connection error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    pthread_create(&worker, NULL, pthread_prog, &sd);
    char buff[100];
    while (1) {
        memset(buff, 0, 100);
        scanf("%s", buff);
        int *msgLen = (int *)calloc(sizeof(int), 1);
        *msgLen = strlen(buff);
        // printf("%s\n",buff);
        if (sendMsg(sd, (char *)msgLen, sizeof(int)) == 1) {
            fprintf(stderr, "send error, exit\n");
            exit(0);
        }
        if (sendMsg(sd, buff, *msgLen) == 1) {
            fprintf(stderr, "send error, exit\n");
            exit(0);
        }
    }
    return 0;
}
