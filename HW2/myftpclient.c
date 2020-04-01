#include "myftp.c"

int sd;
char *server_ip_addr;
char *port;
char *mode;
char *filename;

void list_request()
{
    send_header(sd, 0xa1, 0);
    struct message_s *header = recv_header(sd);
    int payload_len = header->length - HEADER_LEN;
    void *buff = recv_payload(sd, payload_len);
    printf("%s\n", (char *)buff);
    free(header);
    free(buff);
}

void get_request()
{
    send_header(sd, 0xb1, strlen(filename) + 1);
    send_payload(sd, (void *)filename, strlen(filename) + 1);
    struct message_s *header = recv_header(sd);
    unsigned char type = header->type;
    if (type == 0xb3)
    {
        // the file does not exist
        printf("The requested file does not exist.\n");
        return;
    }
    if (type == 0xb2)
    {
        printf("The download process will begin soon.\n");
    }

    // start to receive file
    FILE *fptr = fopen(filename, "wb");
    header = recv_header(sd);

    int file_size = header->length - HEADER_LEN;
    void *buff = recv_payload(sd, file_size);
    if (fwrite(buff, file_size, 1, fptr) == 0)
    {
        printf("Download fail.\n");
    }
    else
    {
        printf("Download success.\n");
    }
    free(header);
    free(buff);
    fclose(fptr);
}

void put_request()
{
    int file_size;
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        // the file exist
        file_size = st.st_size;
        send_header(sd, 0xc1, strlen(filename) + 1);
        send_payload(sd, (void *)filename, strlen(filename) + 1);
        struct message_s *header = recv_header(sd);
        unsigned char type = header->type;
        send_header(sd, 0xff, file_size);
        FILE *fptr = fopen(filename, "rb");
        void *buff = (void *)malloc(file_size);
        fread(buff, file_size, 1, fptr);
        printf("Sending %s to server.\n", filename);
        send_payload(sd, buff, file_size);
        printf("Upload success.\n");

        free(header);
        free(buff);
        fclose(fptr);
    }
    else
    {
        printf("%s does not exist in local.\n", filename);
    }
}

int main(int argc, char **argv)
{
    // setup connection
    char *ip_addr = argv[1];  //Adam: these variables ip_addr and port need to connect the variables list_of_IP and list_of_port below
    int port = atoi(argv[2]);

    printf("reading %s\n", argv[1]);
    FILE *fp;
    char buff[255];
    fp = fopen(argv[1], "r");
    fgets(buff, 255, (FILE *)fp);
    int n = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    int k = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    int block_size = atoi(buff);

    char delim[] = ":";
    char list_of_IP[255][255];
    int list_of_port[255];
    int count = 0;
    while (fgets(buff, sizeof(buff), (FILE *)fp) != 0)
    {
        char *ptr = strtok(buff, delim);
        memcpy(list_of_IP[count], ptr, strlen(ptr) + 1);
        ptr = strtok(NULL, delim);
        list_of_port[count] = atoi(ptr);
        count++;
    }
    printf("This is the read var:\n");
    printf("n : %d\n", n);
    printf("k : %d\n", k);
    printf("block_size : %d\n", block_size);
    int print_count = 0;
    for (print_count = 0; print_count < count; print_count++)
    {
        printf("list %d\n", print_count);
        printf("IP : %s\n", list_of_IP[print_count]);
        printf("port : %d\n\n", list_of_port[print_count]);
    }
    fclose(fp);

    printf("finish reading var\n");

    sd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    pthread_t worker;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_port = htons(port);
    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("connection error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    // three mode
    mode = argv[3];
    if (argc >= 5)
    {
        filename = argv[4];
    }
    // printf("Mode: %s\n", mode);
    // printf("Filename: %s\n", filename);
    if (strcmp(mode, "list") == 0)
    {
        list_request();
    }
    else if (strcmp(mode, "put") == 0)
    {
        put_request();
    }
    else if (strcmp(mode, "get") == 0)
    {
        get_request();
    }

    close(sd);
    return 0;
}
