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

// combine all filenames to one string
void listdir(char *file_list){
    DIR *dir;
    struct dirent *entry;
    int i = 0;
    if ((dir = opendir("data/metadata/")) == NULL){
        perror("opendir() error");
    }
    else {
        while ((entry = readdir(dir)) != NULL) {
            memcpy(file_list + i, entry->d_name, strlen(entry->d_name));
            i = i + strlen(entry->d_name);
            memcpy(file_list + i, "\n", 1);
            i = i + 1;
        }
        memcpy(file_list + i - 1, "\0", 1);
        closedir(dir);
    }
}

// send header
void send_header(int sd, unsigned char type, int payload_len) {
    struct message_s *header = (struct message_s *)malloc(sizeof(struct message_s));
    memcpy(header->protocol, "myftp", 5);
    header->type = type;
    header->length = htonl(sizeof(*header) + payload_len);
    if (sendn(sd, (void *)header, HEADER_LEN) == 1) {
        fprintf(stderr, "send error, exit\n");
        exit(0);
    }
    free(header);
}

// receive header and return
struct message_s *recv_header(int sd) {
    struct message_s *header = (struct message_s *)malloc(sizeof(struct message_s));
    if (recvn(sd, (void *)header, HEADER_LEN) == 1) {
        fprintf(stderr, "error receiving, exit!\n");
        exit(0);
    }
    header->length = ntohl(header->length);
    return header;
}

// send payload as void buffer
void send_payload(int sd, void *buff, int payload_len) {
    if(sendn(sd, (void *)buff, payload_len) == 1) {
        fprintf(stderr, "send error, exit\n");
        exit(0);
    }
}

// receive payload as void buffer
void *recv_payload(int sd, int payload_len) {
    void *buff = (void *)malloc(payload_len);
    if (recvn(sd, (void *)buff, payload_len) == 1) {
        fprintf(stderr, "error receiving, exit!\n");
        exit(0);
    }
    return buff;
}
