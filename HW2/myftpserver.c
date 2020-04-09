#include "myftp.c"

int n;
int k;
int ID;
int BLOCK_SIZE;
int PORT;

void print_global_vars(){
    printf("n: %d\n", n);
    printf("k: %d\n", k);
    printf("ID: %d\n", ID);
    printf("BLOCK_SIZE: %d\n", BLOCK_SIZE);
    printf("PORT: %d\n", PORT);
}

void init_config(char *config_file_name)
{
    // printf("reading %s\n", config_file_name);
    FILE *fp;
    char buff[255];
    fp = fopen(config_file_name, "r");
    fgets(buff, 255, (FILE *)fp);
    n = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    k = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    ID = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    BLOCK_SIZE = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    PORT = atoi(buff);
    // printf("This is the read var:\n");
    // printf("n : %d\n", n);
    // printf("k : %d\n", k);
    // printf("ID : %d\n", ID);
    // printf("BLOCK_SIZE : %d\n", BLOCK_SIZE);
    // printf("PORT : %d\n", PORT);
    fclose(fp);
    // printf("finish reading var\n");
}

void list_reply(int sd)
{
    char list_dir[500];
    int payload_len;
    listdir(list_dir);
    payload_len = strlen(list_dir) + 1;
    send_header(sd, 0xa2, payload_len);
    send_payload(sd, list_dir, payload_len);
}

void get_reply(int sd, int payload_len)
{
    int file_size;
    int fragment_size;
    struct stat st;
    char *filename = (char *)recv_payload(sd, payload_len);
    char final_path[6 + strlen(filename)];
    char meta_path[15 + strlen(filename)];
    strcpy(final_path, "data/");
    strcat(final_path, filename);
    strcpy(meta_path, "data/metadata/");
    strcat(meta_path, filename);
    printf("Requested filename: %s\n", filename);
    if (stat(meta_path, &st) == 0)
    {   
        printf("meta path: %s\n", meta_path);
        // the file exist
        FILE *fptr = fopen(meta_path, "rb");
        fscanf(fptr, "%d", &file_size);
        printf("File size: %d\n", file_size);
        send_header(sd, 0xb2, file_size);
        fclose(fptr);

        fragment_size = ceil((double)file_size / (BLOCK_SIZE * k)) * BLOCK_SIZE;
        send_header(sd, 0xb2, fragment_size);
        // printf("Fragment size: %d\n", fragment_size);
        fptr = fopen(final_path, "rb");
        unsigned char *buff = malloc(fragment_size);
        
        fread((void *)buff, fragment_size, 1, fptr);
        // for(int i = 0; i< 0; i++){
        //     printf("%d ", *(buff + i));
        // }
        // printf("\n");
        // printf("%s\n",buff);
        send_payload(sd, (void *)buff, fragment_size);
        free(buff);
    }
    else
    {
        send_header(sd, 0xb3, 0);
    }

    free(filename);
}

void put_reply(int sd, int payload_len)
{
    char *filename = (char *)recv_payload(sd, payload_len);
    char final_path[6 + strlen(filename)];
    char meta_path[15 + strlen(filename)];
    strcpy(final_path, "data/");
    strcat(final_path, filename);
    strcpy(meta_path, "data/metadata/");
    strcat(meta_path, filename);

    // send put reply header
    send_header(sd, 0xc2, 0);

    // receive file transfer header and get file size
    struct message_s *header = recv_header(sd);
    int file_size = header->length - HEADER_LEN;
    free(header);

    // get the fragment size
    header = recv_header(sd);
    int fragment_size = header->length - HEADER_LEN;

    //get payload and write it to file system
    unsigned char *buff = recv_payload(sd, fragment_size);
    // printf("%s\n",(char *)buff);
    // printf("%s\n",final_path);
    FILE *fptr = fopen(final_path, "wb");
    if (fwrite(buff, fragment_size, 1, fptr) == 0)
    {
        printf("Download fail.\n");
    }
    else
    {
        printf("Download success.\n");
    }
    // printf("block: ");
    // for (int j = 0; j < fragment_size; j++){
    //     printf("%d ", (unsigned char*)buff[j]);
    // }
    // printf("\n");
    fclose(fptr);
    // write metadata file
    fptr = fopen(meta_path, "wb");
    // printf("%s\n", meta_path);
    if (fprintf(fptr, "%d", file_size) == 0)
    {
        printf("Fail to write metadata file.\n");
    }else {
        printf("Successfully write metadata file.\n");
    }
    
    free(header);
    free(buff);
    fclose(fptr);
}

void *pthread_prog(void *sDescriptor)
{
    int sd = *(int *)sDescriptor;
    struct message_s *header = recv_header(sd);
    // printf("Received Header:\n");
    // printf("protocol: %s", header->protocol);
    // printf("type: 0x%x\n", header->type);
    // printf("length: %u\n", header->length);
    if (header->type == 0xa1)
    {
        printf("List request received.\n");
        list_reply(sd);
    }
    else if (header->type == 0xb1)
    {
        printf("Get request received.\n");
        get_reply(sd, header->length - HEADER_LEN);
    }
    else if (header->type == 0xc1)
    {
        printf("Put request received.\n");
        put_reply(sd, header->length - HEADER_LEN);
    }
    close(sd);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    init_config(argv[1]);
    // create a metadata directory
    struct stat st;
    if (stat("data/", &st) == -1) {
        mkdir("data/", 0777);
    }
    if (stat("data/metadata/", &st) == -1) {
        mkdir("data/metadata/", 0777);
    }
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    long val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("bind error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    if (listen(sd, 10) < 0)
    {
        printf("listen error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    printf("Listen on %u:%d\n",server_addr.sin_addr.s_addr, PORT);
    while (1)
    {
        int client_sd;
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) < 0)
        {
            printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
            // exit(0);
        }
        else
        {
            printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
        }
        pthread_t worker;
        pthread_create(&worker, NULL, pthread_prog, &client_sd);
        pthread_detach(worker);
    }

    return 0;
}
