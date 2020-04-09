// #include "myftp.c"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <isa-l.h>

int n;
int k;
int BLOCK_SIZE;
int FILE_SIZE;
int STRIPE_NUM;

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

void save_file(unsigned char **reformed_blocks, char *filename){
    FILE *out_fptr = fopen("test_out.txt", "wb");
    int remain_bytes = FILE_SIZE;
    printf("Remain bytes: %d\n", remain_bytes);
    for(int i = 0 ; i < k ; i++){
        int write_bytes = remain_bytes > STRIPE_NUM * BLOCK_SIZE ? STRIPE_NUM * BLOCK_SIZE : remain_bytes;
        int written_bytes = fwrite(reformed_blocks[i], write_bytes, 1, out_fptr);
        remain_bytes = remain_bytes - write_bytes;
        printf("Write bytes: %d\n", write_bytes);
        printf("Remain bytes: %d\n", remain_bytes);
    }
    fclose(out_fptr);
}

unsigned char **buffer_to_blocks_data(void *file_buff){
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

    return blocks_data;
}

unsigned char **generate_reform_block(unsigned char **blocks_data){
    int count, count2;
    unsigned char *encode_matrix = malloc(sizeof(unsigned char) * (n * k));
    gf_gen_rs_matrix(encode_matrix, n, k);

    unsigned char *table = malloc(sizeof(unsigned char) * (32 * k * (n - k)));
    ec_init_tables(k, n - k, &encode_matrix[k * k], table);
    unsigned char *errors_matrix = malloc(sizeof(unsigned char) * (k * k));
    unsigned char *invert_matrix = malloc(sizeof(unsigned char) * (k * k));
    int work_nodes[3] = {0, 1, 4};
    int non_working_nodes[2] = {2, 3};


    for (count = 0; count < k; count++)
    {
        int r = work_nodes[count];
        for (count2 = 0; count2 < k; count2++)
        {
            errors_matrix[k * count + count2] = encode_matrix[k * r + count2];
        }
    }

    for(int i = 0; i < k)

    if (gf_invert_matrix(errors_matrix, invert_matrix, k)) return -1;

    int i,j;
    unsigned char *decode_matrix = malloc(sizeof(unsigned char) * (n * k));
    for (i = 0; i < n - k; i++) {
		if (non_working_nodes[i] < k)	// A src err
			for (j = 0; j < k; j++)
				decode_matrix[k * i + j] =
				    invert_matrix[k * non_working_nodes[i] + j];
	}

	// For non-src (parity) erasures need to multiply encode matrix * invert
    int p;
    unsigned char s = malloc(sizeof(unsigned char) * n * k);
	for (p = 0; p < n - k; p++) {
		if (non_working_nodes[p] >= k) {	// A parity err
			for (i = 0; i < k; i++) {
				s = 0;
				for (j = 0; j < k; j++)
					s ^= gf_mul(invert_matrix[j * k + i],
						    encode_matrix[k * non_working_nodes[p] + j]);
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

    for (count = 0; count < 3; count++)
    {
        memcpy(blocks_data_decoding[count], blocks_data[work_nodes[count]], STRIPE_NUM * BLOCK_SIZE);
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
        tmp = arr_index(i, work_nodes, 3);
        if(tmp >= 0){
            reformed_blocks[i] = blocks_data_decoding[tmp];
        }else{
            tmp = arr_index(i, non_working_nodes, 2);
            reformed_blocks[i] = blocks_data_decoding[tmp + 3];
        }
    }

    return reformed_blocks;
}

int main(int argc, char **argv)
{
    char *filename = argv[2];
    FILE *fp;
    char buff[255];
    fp = fopen(argv[1], "r");
    fgets(buff, 255, (FILE *)fp);
    n = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    k = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    BLOCK_SIZE = atoi(buff);

    char delim[] = ":";
    char list_of_IP[255][255];
    int list_of_port[255];
    int count = 0;
    int count2 = 0;
    while (fgets(buff, sizeof(buff), (FILE *)fp) != 0)
    {
        char *ptr = strtok(buff, delim);
        memcpy(list_of_IP[count], ptr, strlen(ptr) + 1);
        ptr = strtok(NULL, delim);
        list_of_port[count] = atoi(ptr);
        count++;
    }
    
    int print_count = 0;

    fclose(fp);

    //end reading config file

    struct stat st;
    stat(filename, &st);
    FILE_SIZE = st.st_size;
    STRIPE_NUM = ceil((double)FILE_SIZE / (BLOCK_SIZE * k));
    print_global_vars();
    FILE *fptr = fopen(filename, "rb");
    void *file_buff = (void *)malloc(FILE_SIZE);
    fread(file_buff, FILE_SIZE, 1, fptr);
    printf("file_buff: %s\n", file_buff);

    unsigned char **blocks_data = buffer_to_blocks_data(file_buff);
    
    printf("Encoded block_data: \n");
    for (count = 0; count < n; count++)
    {
        printf("blocks_data[%d]: ", count);
        for (count2 = 0; count2 < STRIPE_NUM * BLOCK_SIZE; count2++)
        {
            printf("%d ", blocks_data[count][count2]);
        }
        printf("\n");
    }
    printf("\n\n");

    unsigned char **reformed_blocks = generate_reform_block(blocks_data);
    printf("Reform block: \n");
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
    save_file(reformed_blocks, "test_out.txt");
}

