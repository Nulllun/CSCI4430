# include "myftp.h"

// for completely sending data. Copied from tutorial
int sendn(int sd, void *buff, int buff_len) {
    int n_left = buff_len;
    int n;
    while(n_left > 0){
        if((n = send(sd, buff + (buff_len - n_left), n_left, 0)) < 0){
            if(errno == EINTR){
                n = 0;
            }
            else {
                return -1;
            }
        }
        else if(n == 0){
            return 0;
        }
        n_left = n_left - n;
    }
    return buff_len;
}

// for completely receiving data. Copied from tutorial
int recvn(int sd, void *buff, int buff_len) {
    int n_left = buff_len;
    int n;
    while(n_left > 0){
        if((n = recv(sd, buff + (buff_len - n_left), n_left, 0)) < 0){
            if(errno == EINTR){
                n = 0;
            }
            else {
                return -1;
            }
        }
        else if(n == 0){
            return 0;
        }
        n_left = n_left - n;
    }
    return buff_len;
}

void listdir(char **file_list){
    DIR *dir;
    struct dirent *entry;
    int i = 0;

    if ((dir = opendir("data")) == NULL){
        perror("opendir() error");
    }
    else {
        while ((entry = readdir(dir)) != NULL) {
            printf("%lu\n", strlen(entry->d_name));
        }
        closedir(dir);
    }
}

void send_header(int sd, unsigned char type, unsigned int payload_len) {
    struct message_s *header = (struct message_s *)malloc(sizeof(struct message_s));
    memcpy(header->protocol, "myftp", 5);
    header->type = type;
    header->length = sizeof(*header) + payload_len;
    printf("protocol: %s", header->protocol);
    printf("type: 0x%x\n", header->type);
    printf("length: %u\n", header->length);
    
    if (sendn(sd, (void *)header, 10) == 1) {
        fprintf(stderr, "send error, exit\n");
        exit(0);
    }
    free(header);
}

void recv_header(int sd, struct message_s *header) {
    if (recvn(sd, (void *)header, 10) == 1) {
        fprintf(stderr, "error receiving, exit!\n");
        exit(0);
    }
}

void send_payload() {

}

void send_file() {

}

void recv_payload() {

}

void recv_file() {

}
