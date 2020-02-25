# include "myftp.c"

void list_reply(int sd) {
    char list_dir[500];
    int payload_len;
    struct message_s *header = (struct message_s *)malloc(sizeof(struct message_s));
    listdir(list_dir);
    payload_len = strlen(list_dir) + 1;
    send_header(sd, 0xa2, payload_len);
    send_payload(sd, list_dir, payload_len);
}

void get_reply() {

}

void put_reply() {

}

void *pthread_prog(void *sDescriptor) {
    int sd = *(int *)sDescriptor;
    struct message_s *header = recv_header(sd);
    printf("protocol: %s", header->protocol);
    printf("type: 0x%x\n", header->type);
    printf("length: %u\n", header->length);
    if(header->type == 0xa1){
        list_reply(sd);
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
    
    close(sd);
    return 0;
}