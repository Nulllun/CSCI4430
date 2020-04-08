#include "myftp.c"

int sds[255];
char *mode;
char *FILENAME;
int SERVER_COUNT;
int AVAIL_SERVER_COUNT;
int n;
int k;
int BLOCK_SIZE;
char LIST_OF_IP[255][255];
int LIST_OF_PORT[255];

void init_config(char *config_file_name)
{
    printf("reading %s\n", config_file_name);
    FILE *fp;
    char buff[255];
    fp = fopen(config_file_name, "r");
    fgets(buff, 255, (FILE *)fp);
    n = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    k = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    BLOCK_SIZE = atoi(buff);

    char delim[] = ":";
    //char LIST_OF_IP[255][255];
    //int LIST_OF_PORT[255];
    SERVER_COUNT = 0;
    while (fgets(buff, sizeof(buff), (FILE *)fp) != 0)
    {
        char *ptr = strtok(buff, delim);
        memcpy(LIST_OF_IP[SERVER_COUNT], ptr, strlen(ptr) + 1);
        ptr = strtok(NULL, delim);
        LIST_OF_PORT[SERVER_COUNT] = atoi(ptr);
        SERVER_COUNT++;
    }
    printf("This is the read var:\n");
    printf("n : %d\n", n);
    printf("k : %d\n", k);
    printf("block_size : %d\n", BLOCK_SIZE);
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        printf("list %d\n", i);
        printf("IP : %s\n", LIST_OF_IP[i]);
        printf("port : %d\n\n", LIST_OF_PORT[i]);
    }
    fclose(fp);

    printf("finish reading var\n");
}

void init_sd()
{
    AVAIL_SERVER_COUNT = 0;
    for(int i = 0; i < SERVER_COUNT ; i++){
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(LIST_OF_IP[i]);
        server_addr.sin_port = htons(LIST_OF_PORT[i]);
        if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
            printf("connection error to %s:%d : %s (Errno:%d)\n", LIST_OF_IP[i], LIST_OF_PORT[i], strerror(errno), errno);            
        }else{
            printf("Connected to %s:%d\n", LIST_OF_IP[i], LIST_OF_PORT[i]);
            sds[AVAIL_SERVER_COUNT] = sd;
            AVAIL_SERVER_COUNT = AVAIL_SERVER_COUNT + 1;
        }
    }
    printf("Available server count: %d\n", AVAIL_SERVER_COUNT);
}

void list_request(int sd)
{
    send_header(sd, 0xa1, 0);
    struct message_s *header = recv_header(sd);
    int payload_len = header->length - HEADER_LEN;
    void *buff = recv_payload(sd, payload_len);
    printf("%s\n", (char *)buff);
    free(header);
    free(buff);
}

void get_request(int sd)
{
    send_header(sd, 0xb1, strlen(FILENAME) + 1);
    send_payload(sd, (void *)FILENAME, strlen(FILENAME) + 1);
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
    FILE *fptr = fopen(FILENAME, "wb");
    header = recv_header(sd);
    int file_size = header->length - HEADER_LEN;
    printf("File size: %d\n", file_size);
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

void put_request(int sd)
{
    int file_size;
    struct stat st;
    if (stat(FILENAME, &st) == 0)
    {
        // the file exist
        file_size = st.st_size;

        // tell server filename
        send_header(sd, 0xc1, strlen(FILENAME) + 1);
        send_payload(sd, (void *)FILENAME, strlen(FILENAME) + 1);
        struct message_s *header = recv_header(sd);
        unsigned char type = header->type;

        // tell server file size
        send_header(sd, 0xff, file_size);

        // read file to void buffer
        FILE *fptr = fopen(FILENAME, "rb");
        void *buff = (void *)malloc(file_size);
        fread(buff, file_size, 1, fptr);

        printf("Sending %s to server.\n", FILENAME);

        // send the file to server
        send_payload(sd, buff, file_size);
        printf("Upload success.\n");

        free(header);
        free(buff);
        fclose(fptr);
    }
    else
    {
        printf("%s does not exist in local.\n", FILENAME);
    }
}

int main(int argc, char **argv)
{
    init_config(argv[1]);

    init_sd();

    // three mode
    mode = argv[2];
    if (argc >= 4)
    {
        FILENAME = argv[3];
    }

    if (strcmp(mode, "list") == 0)
    {
        if(AVAIL_SERVER_COUNT < 1){
            printf("No server is available. LIST command failed. Exit program now.\n");
        }
        list_request(sds[0]);
    }
    else if (strcmp(mode, "put") == 0)
    {
        if(AVAIL_SERVER_COUNT != n){
            printf("Not all servers are available. PUT command failed. Exit program now.\n");
        }
        put_request(sds[0]);
    }
    else if (strcmp(mode, "get") == 0)
    {
        if(AVAIL_SERVER_COUNT < k){
            printf("Not enough number of servers available (k). GET command failed. Exit program now.\n");
        }
        get_request(sds[0]);
    }

    return 0;
}
