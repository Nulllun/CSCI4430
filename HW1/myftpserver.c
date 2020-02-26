# include "myftp.c"

void list_reply(int sd) {
    char list_dir[500];
    int payload_len;
    listdir(list_dir);
    payload_len = strlen(list_dir) + 1;
    send_header(sd, 0xa2, payload_len);
    send_payload(sd, list_dir, payload_len);
}

void get_reply(int sd, int payload_len) {
    int file_size;
    struct stat st;  
    char *filename = (char *)recv_payload(sd, payload_len);
    char final_path[6 + strlen(filename)];
    strcpy(final_path, "data/");
    strcat(final_path, filename);
    printf("Filename: %s\n", final_path);
    if (stat(final_path, &st) == 0) {
        // the file exist
        file_size = st.st_size;
        send_header(sd, 0xb2, 0);
        send_header(sd, 0xff, file_size);

        FILE *fptr = fopen(final_path, "rb");
        void *buff = (void *)malloc(file_size);
        fread(buff, file_size, 1, fptr);
        send_payload(sd, buff, file_size);

        free(buff);
        fclose(fptr);
    }
    else {
        send_header(sd, 0xb3, 0);
        printf("File error: %s (Errno:%d)\n", strerror(errno), errno);
    }

    free(filename);
}

void put_reply() {

}

void *pthread_prog(void *sDescriptor) {
    int sd = *(int *)sDescriptor;
    struct message_s *header = recv_header(sd);
    // printf("Received Header:\n");
    // printf("protocol: %s", header->protocol);
    // printf("type: 0x%x\n", header->type);
    // printf("length: %u\n", header->length);
    if(header->type == 0xa1){
        list_reply(sd);
    }
    else if(header->type == 0xb1){
        get_reply(sd, header->length - HEADER_LEN);
    }
    else if(header->type == 0xc1){
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    long val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("bind error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    if (listen(sd, 10) < 0) {
        printf("listen error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    while(1) {
        int client_sd;
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) <0) {
            printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
            // exit(0);
        } else {
            printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
        }
        pthread_t worker;
        pthread_create(&worker, NULL, pthread_prog, &client_sd);
        pthread_detach(worker);
    }

    return 0;
}
