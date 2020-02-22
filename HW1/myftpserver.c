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
        fprintf(stderr, "error sending msg, error code: %d\n", rLen);
        return 1;
        }
        recvLen += rLen;
    }
    return 0;
}

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
    int client_sd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("bind error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    if (listen(sd, 3) < 0) {
        printf("listen error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, &addr_len)) <0) {
        printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    } else {
        printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
    }
    pthread_t worker;
    pthread_create(&worker, NULL, pthread_prog, &client_sd);
    char buff[100];
    while (1) {
        memset(buff, 0, 100);
        scanf("%s", buff);
        int *msgLen = (int *)calloc(sizeof(int), 1);
        *msgLen = strlen(buff);
        if (sendMsg(client_sd, (char *)msgLen, sizeof(int)) == 1) {
            fprintf(stderr, "send error, exit\n");
            exit(0);
        }
        if (sendMsg(client_sd, buff, *msgLen) == 1) {
            fprintf(stderr, "send error, exit\n");
            exit(0);
        }
    }
    close(sd);
    return 0;
}