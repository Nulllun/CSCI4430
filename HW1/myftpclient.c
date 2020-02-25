# include "myftp.c"

int sd;
char *server_ip_addr;
char *port;
char *mode;
char *filename;

void list_request() {
    send_header(sd, 0xa1, 0);
    struct message_s *header = recv_header(sd);
    int payload_len = header->length - 10;
    void *buff = recv_payload(sd, payload_len);
    printf("%s\n", (char *)buff);
    free(header);
    free(buff);
}

void get_request() {
    send_header(sd, 0xb1, 0);
}

void put_request() {
    send_header(sd, 0xc1, 0);
}

int main(int argc, char **argv) {
    // setup connection
    sd = socket(AF_INET, SOCK_STREAM, 0);
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

    // three mode
    mode = argv[1];
    if(argc >= 3) {
        filename = argv[2];
    }
    printf("Mode: %s\n", mode);
    printf("Filename: %s\n", filename);

    if(strcmp(mode, "list") == 0){
        list_request();
    }
    else if(strcmp(mode, "put") == 0){
        put_request();
    }
    else if(strcmp(mode, "get") == 0){
        get_request();
    }
    
    close(sd);
    return 0;
}
