# include "myftp.c"

void *pthread_prog(void *sDescriptor) {
    int sd = *(int *)sDescriptor;
    struct message_s *header = (struct message_s *)malloc(sizeof(struct message_s));
    recv_header(sd, header);
    printf("protocol: %s", header->protocol);
    printf("type: 0x%x\n", header->type);
    printf("length: %u\n", header->length);
    if(header->type == 0xa1){
        printf("It is a list request\n");
    }
    else if(header->type == 0xb1){
        printf("It is a list request\n");
    }
    else if(header->type == 0xc1){
        printf("It is a list request\n");
    }
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
    if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) <0) {
        printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    } else {
        printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
    }
    pthread_t worker;
    pthread_create(&worker, NULL, pthread_prog, &client_sd);
    char buff[100];
    while (1) {
        // memset(buff, 0, 100);
        // scanf("%s", buff);
        // int *msgLen = (int *)calloc(sizeof(int), 1);
        // *msgLen = strlen(buff);
        // if (sendMsg(client_sd, (char *)msgLen, sizeof(int)) == 1) {
        //     fprintf(stderr, "send error, exit\n");
        //     exit(0);
        // }
        // if (sendMsg(client_sd, buff, *msgLen) == 1) {
        //     fprintf(stderr, "send error, exit\n");
        //     exit(0);
        // }
    }
    close(sd);
    return 0;
}