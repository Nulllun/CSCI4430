#include "myftp.c"

int sds[255];
int max_sd = 0;
char *mode;
char *FILENAME;
int SERVER_COUNT;
int AVAIL_SERVER_COUNT;
int n;
int k;
int BLOCK_SIZE;
char LIST_OF_IP[255][255];
int LIST_OF_PORT[255];

void reset_all_sd(fd_set *fds){
    FD_ZERO(fds);
    for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
        FD_SET(sds[i], fds);
    }
}

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
            max_sd = sd > max_sd ? sd : max_sd;
            AVAIL_SERVER_COUNT = AVAIL_SERVER_COUNT + 1;
        }
    }
    printf("Available server count: %d\n", AVAIL_SERVER_COUNT);
}

void list_request()
{
    fd_set fds;
    while(1){
        reset_all_sd(&fds);
        if(select(max_sd, NULL, &fds, NULL, NULL) > 0){
            break;
        }
    }

    for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
        if(FD_ISSET(sds[i], &fds)){
            send_header(sds[i], 0xa1, 0);
            struct message_s *header = recv_header(sds[i]);
            int payload_len = header->length - HEADER_LEN;
            void *buff = recv_payload(sds[i], payload_len);
            printf("%s\n", (char *)buff);
            free(header);
            free(buff);
            break;
        }
    }
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
    int check[AVAIL_SERVER_COUNT];
    for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
        check[i] = 0;
    }
    if (stat(FILENAME, &st) == 0)
    {
        // the file exist
        file_size = st.st_size;
        fd_set fds;
        // read file to void buffer
        FILE *fptr = fopen(FILENAME, "rb");
        void *buff = (void *)malloc(file_size);
        fread(buff, file_size, 1, fptr);
        while(1){
            reset_all_sd(&fds);
            int flag = 0;
            for (int i = 0 ; i < AVAIL_SERVER_COUNT ; i++){
                if(check[i] == 1){
                    FD_CLR(sds[i], &fds);
                    flag = flag + 1;
                }
            }
            if(flag == AVAIL_SERVER_COUNT){
                return;
            }
            if(select(max_sd+1, NULL, &fds, NULL, NULL) > 0){
                for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
                    if(FD_ISSET(sds[i], &fds)){

                        // tell server filename
                        send_header(sds[i], 0xc1, strlen(FILENAME) + 1);
                        send_payload(sds[i], (void *)FILENAME, strlen(FILENAME) + 1);
                        struct message_s *header = recv_header(sds[i]);

                        // tell server file size
                        send_header(sds[i], 0xff, file_size);

                        printf("Sending %s to server.(%d)\n", FILENAME, sds[i]);

                        // send the file to server
                        send_payload(sds[i], buff, file_size);
                        printf("Upload success.(%d)\n", sds[i]);

                        free(header);
                        
                        check[i] = 1;
                    }
                }       
            }
        }
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
            exit(0);
        }
        list_request();
    }
    else if (strcmp(mode, "put") == 0)
    {
        if(AVAIL_SERVER_COUNT != n){
            printf("Not all servers are available. PUT command failed. Exit program now.\n");
            exit(0);
        }
        put_request(sds[0]);
    }
    else if (strcmp(mode, "get") == 0)
    {
        if(AVAIL_SERVER_COUNT < k){
            printf("Not enough number of servers available (k). GET command failed. Exit program now.\n");
            exit(0);
        }
        get_request(sds[0]);
    }

    return 0;
}
