#include "myftp.c"

int sds[255];
int max_sd = 0;
char *mode;
char *FILENAME;
int FILE_SIZE;
int SERVER_COUNT;
int AVAIL_SERVER_COUNT;
int FAIL_SERVER_COUNT;
int WK_SERVER_LIST[255];
int NWK_SERVER_LIST[255];
int n;
int k;
int BLOCK_SIZE;
int STRIPE_NUM;
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
    // printf("reading %s\n", config_file_name);
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
    // printf("This is the read var:\n");
    // printf("n : %d\n", n);
    // printf("k : %d\n", k);
    // printf("block_size : %d\n", BLOCK_SIZE);
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        // printf("list %d\n", i);
        // printf("IP : %s\n", LIST_OF_IP[i]);
        // printf("port : %d\n\n", LIST_OF_PORT[i]);
    }
    fclose(fp);

    // printf("finish reading var\n");
}

void init_sd()
{
    AVAIL_SERVER_COUNT = 0;
    FAIL_SERVER_COUNT = 0;
    for(int i = 0; i < SERVER_COUNT ; i++){
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(LIST_OF_IP[i]);
        server_addr.sin_port = htons(LIST_OF_PORT[i]);
        if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
            //printf("connection error to %s:%d : %s (Errno:%d)\n", LIST_OF_IP[i], LIST_OF_PORT[i], strerror(errno), errno);            
            NWK_SERVER_LIST[FAIL_SERVER_COUNT] = i;
            FAIL_SERVER_COUNT = FAIL_SERVER_COUNT + 1;
        }else{
            //printf("Connected to %s:%d\n", LIST_OF_IP[i], LIST_OF_PORT[i]);
            sds[AVAIL_SERVER_COUNT] = sd;
            max_sd = sd > max_sd ? sd : max_sd;
            WK_SERVER_LIST[AVAIL_SERVER_COUNT] = i;
            AVAIL_SERVER_COUNT = AVAIL_SERVER_COUNT + 1;
        }
    }
    printf("Available server: ");
    for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
        printf("%d ", WK_SERVER_LIST[i]);
    }
    printf("\n");
    printf("Unavailable server: ");
    for(int i = 0; i < FAIL_SERVER_COUNT ; i++){
        printf("%d ", NWK_SERVER_LIST[i]);
    }
    printf("\n");
}
void print_global_vars(){
    printf("n: %d\n", n);
    printf("k: %d\n", k);
    printf("BLOCK_SIZE: %d\n", BLOCK_SIZE);
    printf("STRIPE_NUM: %d\n", STRIPE_NUM);
    printf("FILE_SIZE: %d\n", FILE_SIZE);
}

int arr_index(int x, int *arr, int size){
    for(int i = 0 ; i < size ; i++){
        if(arr[i] == x){
            return i;
        }
    }
    return -1;
}

void save_file(unsigned char **reformed_blocks)
{
    FILE *out_fptr = fopen(FILENAME, "wb");
    int remain_bytes = FILE_SIZE;
    // printf("Remain bytes: %d\n", remain_bytes);
    for(int i = 0 ; i < k ; i++){
        int write_bytes = remain_bytes > STRIPE_NUM * BLOCK_SIZE ? STRIPE_NUM * BLOCK_SIZE : remain_bytes;
        int written_bytes = fwrite(reformed_blocks[i], write_bytes, 1, out_fptr);
        remain_bytes = remain_bytes - write_bytes;
        // printf("Write bytes: %d\n", write_bytes);
        // printf("Remain bytes: %d\n", remain_bytes);
    }
    fclose(out_fptr);
}

unsigned char **buffer_to_blocks_data(void *file_buff)
{
    int count, count2;
    unsigned char *encode_matrix = malloc(sizeof(unsigned char) * (n * k));
    gf_gen_rs_matrix(encode_matrix, n, k);
    unsigned char *table = malloc(sizeof(unsigned char) * (32 * k * (n - k)));
    ec_init_tables(k, n - k, &encode_matrix[k * k], table);

    unsigned char **blocks_data = malloc(sizeof(unsigned char **) * n);
    for (count = 0; count < n; count++)
    {
        blocks_data[count] = malloc(STRIPE_NUM * BLOCK_SIZE);
    }
    int remain_bytes = FILE_SIZE;
    for (int i = 0; i < k ; i++){
        int write_bytes = remain_bytes > STRIPE_NUM * BLOCK_SIZE ? STRIPE_NUM * BLOCK_SIZE : remain_bytes;
        memcpy(blocks_data[i], file_buff + STRIPE_NUM * BLOCK_SIZE * i, write_bytes);
        remain_bytes = remain_bytes - write_bytes;
    }
    
    ec_encode_data(BLOCK_SIZE * STRIPE_NUM, k, n - k, table, blocks_data, &blocks_data[k]);
    
    printf("Encoded block_data: \n");
    for (count = 0; count < n; count++)
    {
        printf("block[%d]: ", count);
        for (count2 = 0; count2 < STRIPE_NUM * BLOCK_SIZE; count2++)
        {
            printf("%d ", blocks_data[count][count2]);
        }
        printf("\n");
    }
    printf("\n");
    
    return blocks_data;
}

unsigned char **generate_reform_block(unsigned char **blocks_data)
{
    printf("Before decode block_data: \n");
    for (int i = 0; i < AVAIL_SERVER_COUNT; i++)
    {
        printf("block[%d]: ", i);
        for (int j = 0; j < STRIPE_NUM * BLOCK_SIZE; j++)
        {
            printf("%d ", blocks_data[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    int count, count2;
    unsigned char *encode_matrix = malloc(sizeof(unsigned char) * (n * k));
    gf_gen_rs_matrix(encode_matrix, n, k);

    unsigned char *errors_matrix = malloc(sizeof(unsigned char) * (k * k));
    unsigned char *invert_matrix = malloc(sizeof(unsigned char) * (k * k));
    
    for (count = 0; count < AVAIL_SERVER_COUNT; count++)
    {
        int r = WK_SERVER_LIST[count];
        for (count2 = 0; count2 < k; count2++)
        {
            errors_matrix[k * count + count2] = encode_matrix[k * r + count2];
        }
    }
    printf("Error matrix: \n");
    for(int i = 0; i < k ; i++){
        for(int j = 0; j < k ; j++){
            printf("%d ", errors_matrix[i * k + j]);
        }
        printf("\n");
    }

    if (gf_invert_matrix(errors_matrix, invert_matrix, k)) return -1;

    int i,j;
    unsigned char *decode_matrix = malloc(sizeof(unsigned char) * (n * k));
    for (i = 0; i < FAIL_SERVER_COUNT; i++) {
		if (NWK_SERVER_LIST[i] < k)	// A src err
			for (j = 0; j < k; j++)
				decode_matrix[k * i + j] =
				    invert_matrix[k * NWK_SERVER_LIST[i] + j];
	}

	// For non-src (parity) erasures need to multiply encode matrix * invert
    int p;
    unsigned char s = malloc(sizeof(unsigned char) * n * k);
	for (p = 0; p < FAIL_SERVER_COUNT; p++) {
		if (NWK_SERVER_LIST[p] >= k) {	// A parity err
			for (i = 0; i < k; i++) {
				s = 0;
				for (j = 0; j < k; j++)
					s ^= gf_mul(invert_matrix[j * k + i],
						    encode_matrix[k * NWK_SERVER_LIST[p] + j]);
				decode_matrix[k * p + i] = s;
			}
		}
	}

    unsigned char *table_decoding = malloc(sizeof(unsigned char) * (32 * k * (n - k)));
    ec_init_tables(k, n - k, decode_matrix, table_decoding);

    unsigned char **blocks_data_decoding = malloc(sizeof(unsigned char **) * n);
    for (count = 0; count < n; count++)
    {
        blocks_data_decoding[count] = malloc(STRIPE_NUM * BLOCK_SIZE);
    }

    for (count = 0; count < AVAIL_SERVER_COUNT; count++)
    {
        memcpy(blocks_data_decoding[count], blocks_data[WK_SERVER_LIST[count]], STRIPE_NUM * BLOCK_SIZE);
    }

    ec_encode_data(BLOCK_SIZE * STRIPE_NUM, k, n - k, table_decoding, blocks_data_decoding, &blocks_data_decoding[k]);
    
    printf("Decoded block_data: \n");
    for (count = 0; count < n; count++)
    {
        printf("block[%d]: ", count);
        for (count2 = 0; count2 < STRIPE_NUM * BLOCK_SIZE; count2++)
        {
            printf("%d ", blocks_data_decoding[count][count2]);
        }
        printf("\n");
    }
    printf("\n");

    
    unsigned char **reformed_blocks = malloc(sizeof(unsigned char **) * k);
    for(int i = 0 ; i < k ; i++){
        int tmp;
        tmp = arr_index(i, WK_SERVER_LIST, AVAIL_SERVER_COUNT);
        if(tmp >= 0){
            reformed_blocks[i] = blocks_data_decoding[tmp];
        }else{
            tmp = arr_index(i, NWK_SERVER_LIST, FAIL_SERVER_COUNT);
            reformed_blocks[i] = blocks_data_decoding[tmp + k];
        }
    }

    printf("Reformed block_data: \n");
    for (count = 0; count < k; count++)
    {
        printf("block[%d]: ", count);
        for (count2 = 0; count2 < STRIPE_NUM * BLOCK_SIZE; count2++)
        {
            printf("%d ", reformed_blocks[count][count2]);
        }
        printf("\n");
    }
    printf("\n");

    return reformed_blocks;
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

void get_request()
{
    fd_set fds;
    unsigned char **blocks_data = malloc(sizeof(unsigned char **) * n);
    int flag = 0;
    int check[AVAIL_SERVER_COUNT];
    for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
        check[i] = 0;
    }
    int success = 0;
    int fragment_size = 0;
    while(1){
        reset_all_sd(&fds);
        for (int i = 0 ; i < AVAIL_SERVER_COUNT ; i++){
            if(check[i] == 1){
                FD_CLR(sds[i], &fds);
                success = success + 1;
            }
        }
        if(success == AVAIL_SERVER_COUNT){
            break;
        }
        if(select(max_sd+1, NULL, &fds, NULL, NULL) > 0){
            for(int i = 0 ; i < AVAIL_SERVER_COUNT ; i++){
                if(FD_ISSET(sds[i], &fds)){
                    send_header(sds[i], 0xb1, strlen(FILENAME) + 1);
                    send_payload(sds[i], (void *)FILENAME, strlen(FILENAME) + 1);
                    struct message_s *header = recv_header(sds[i]);
                    unsigned char type = header->type;
                    FILE_SIZE = header->length - HEADER_LEN;
                    STRIPE_NUM = ceil((double)FILE_SIZE / (BLOCK_SIZE * k));
                    if (type == 0xb3)
                    {
                        // the file does not exist
                        printf("The requested file does not exist.\n");
                        return;
                    }
                    if (type == 0xb2)
                    {   
                        header = recv_header(sds[i]);
                        if(flag == 0){
                            printf("The download process will begin soon.\n");
                            fragment_size = header->length - HEADER_LEN;
                            for(int z = 0; z < n ; z++){
                                blocks_data[z] = malloc(fragment_size);
                            }
                            flag = 1;
                        }
                        unsigned char *block = recv_payload(sds[i], fragment_size);
                        // printf("Fragment size: %d\n", fragment_size);
                        // printf("%s", block);
                        memcpy(blocks_data[i], block, fragment_size);
                        //blocks_data[i] = block;
                    }

                    free(header);
                    check[i] = 1;
                }
            }
        }
    }

    printf("Filename: %s\n", FILENAME);
    printf("File size: %d bytes\n", FILE_SIZE);

    // for(int i = 0; i < n ; i++){
    //     printf("block[%d]: %s\n", i, blocks_data[i]);
    // }
    // printf("\n");
    unsigned char **reformed_block = generate_reform_block(blocks_data);
    save_file(reformed_block);
    free(reformed_block);
    free(blocks_data);
}

void put_request()
{
    struct stat st;
    int check[AVAIL_SERVER_COUNT];
    for(int i = 0; i < AVAIL_SERVER_COUNT ; i++){
        check[i] = 0;
    }
    if (stat(FILENAME, &st) == 0)
    {
        // the file exist
        FILE_SIZE = st.st_size;
        STRIPE_NUM = ceil((double)FILE_SIZE / (BLOCK_SIZE * k));
        fd_set fds;
        // read file to void buffer
        FILE *fptr = fopen(FILENAME, "rb");
        unsigned char *buff = malloc(FILE_SIZE);
        fread(buff, FILE_SIZE, 1, fptr);
        printf("File size: %d\n", FILE_SIZE);
        printf("File buff: %s\n", buff);
        printf("STRIPE_NUM: %d | BLOCK_SIZE: %d\n", STRIPE_NUM, BLOCK_SIZE);
        unsigned char **blocks_data = buffer_to_blocks_data(buff);
    
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
                        send_header(sds[i], 0xff, FILE_SIZE);

                        // tell server fragment size
                        send_header(sds[i], 0xff, STRIPE_NUM * BLOCK_SIZE);

                        printf("Sending %s to server.(%d)\n", FILENAME, sds[i]);

                        // send the file to server
                        send_payload(sds[i], blocks_data[i], STRIPE_NUM * BLOCK_SIZE);
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
        put_request();
    }
    else if (strcmp(mode, "get") == 0)
    {
        if(AVAIL_SERVER_COUNT < k){
            printf("Not enough number of servers available (k). GET command failed. Exit program now.\n");
            exit(0);
        }
        get_request();
    }

    return 0;
}
