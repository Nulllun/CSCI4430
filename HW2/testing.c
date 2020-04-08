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

int calculate_num_of_stripe(int file_size, int k, int block_size)
{
    int return_ans = ceil((double)file_size / (block_size * k));
    return return_ans;
}

int main(int argc, char **argv)
{
    char *filename = "test.txt";
    printf("reading %s\n", argv[1]);
    FILE *fp;
    char buff[255];
    printf("start reading n,k and block_size\n");
    fp = fopen(argv[1], "r");
    fgets(buff, 255, (FILE *)fp);
    int n = atoi(buff);
    printf("read n = %d\n", n);
    fgets(buff, 255, (FILE *)fp);
    int k = atoi(buff);
    printf("read k = %d\n", k);
    fgets(buff, 255, (FILE *)fp);
    int block_size = atoi(buff);
    printf("read block_size = %d\n", block_size);
    char delim[] = ":";
    char list_of_IP[255][255];
    int list_of_port[255];
    int count = 0;
    printf("end reading n,k and block_size\n");
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

    //end reading config file

    unsigned char *encode_matrix = malloc(sizeof(unsigned char) * (n * k));
    gf_gen_rs_matrix(encode_matrix, n, k);
    printf("after gf_gen_rs_matrix, encode_matrix :\n");
    int count2;
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < k; count2++)
        {
            printf("%d ", encode_matrix[count * k + count2]);
        }
        printf("\n");
    }

    unsigned char *table = malloc(sizeof(unsigned char) * (32 * k * (n - k)));
    ec_init_tables(k, n - k, &encode_matrix[k * k], table);

    printf("after ec_init_tables, table :\n");
    for (count = 0; count < 32 * k * (n - k); count++)
    {
        printf("%d ", table[count]);
    }
    printf("\n");

    struct stat st;
    stat(filename, &st);
    int file_size = st.st_size;
    FILE *fptr = fopen(filename, "rb");
    void *file_buff = (void *)malloc(file_size);
    fread(file_buff, file_size, 1, fptr);
    printf("file_buff: %s\n", file_buff + 0 * (k * block_size));

    unsigned char **blocks_data = malloc(sizeof(unsigned char **) * n);
    int num_of_stripe = calculate_num_of_stripe(file_size, k, block_size);
    for (count = 0; count < n; count++)
    {
        blocks_data[count] = malloc(num_of_stripe * block_size);
    }

    blocks_data[0] = "1111";
    blocks_data[1] = "2222";
    blocks_data[2] = "3333";
    // blocks_data[3] = "";
    // blocks_data[4] = "";

    printf("before ec_encode_data, block_data: \n");
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < num_of_stripe * block_size; count2++)
        {
            printf("%d ", blocks_data[count][count2]);
        }
        printf("\n");
    }
    ec_encode_data(block_size * num_of_stripe, k, n - k, table, blocks_data, &blocks_data[k]);
    printf("after ec_encode_data, block_data: \n");
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < num_of_stripe * block_size; count2++)
        {
            printf("%d ", blocks_data[count][count2]);
        }
        printf("\n");
    }
    printf("\n");
    // done encoding

    printf("after encoding, encode_matrix :\n");
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < k; count2++)
        {
            printf("%d ", encode_matrix[count * k + count2]);
        }
        printf("\n");
    }

    printf("after encoding, table :\n");
    for (count = 0; count < 32 * k * (n - k); count++)
    {
        printf("%d ", table[count]);
    }
    printf("\n");

    unsigned char *errors_matrix = malloc(sizeof(unsigned char) * (k * k));
    unsigned char *invert_matrix = malloc(sizeof(unsigned char) * (k * k));
    int work_nodes[5] = {0, 1, 4};

    for (count = 0; count < k; count++)
    {
        int r = work_nodes[count];
        for (count2 = 0; count2 < k; count2++)
        {
            errors_matrix[k * count + count2] = encode_matrix[k * r + count2];
        }
        printf("\n");
    }

    printf("after creating errors_matrix, errors_matrix :\n");
    for (count = 0; count < k; count++)
    {
        for (count2 = 0; count2 < k; count2++)
        {
            printf("%d ", errors_matrix[count * k + count2]);
        }
        printf("\n");
    }
    
    if (gf_invert_matrix(errors_matrix, invert_matrix, k)) return -1;
    printf("after creating gf_invert_matrix, gf_invert_matrix :\n");
    for (count = 0; count < k; count++)
    {
        for (count2 = 0; count2 < k; count2++)
        {
            printf("%d ", invert_matrix[count * k + count2]);
        }
        printf("\n");
    }
    // unsigned char *encode_matrix = malloc(sizeof(unsigned char) * (n * k));
    // gf_gen_rs_matrix(encode_matrix, n, k);
    int i,j;
    unsigned char non_working_nodes[2] = {2, 3};
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

    printf("after I don't know, decode_matrix :\n");
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < k; count2++)
        {
            printf("%d ", decode_matrix[count * k + count2]);
        }
        printf("\n");
    }


    unsigned char *table_decoding = malloc(sizeof(unsigned char) * (32 * k * (n - k)));
    ec_init_tables(k, n - k, decode_matrix, table_decoding);





    printf("after ec_init_tables, table_decoding :\n");
    for (count = 0; count < 32 * k * (n - k); count++)
    {
        printf("%d ", table_decoding[count]);
    }
    printf("\n");
    unsigned char **blocks_data_decoding = malloc(sizeof(unsigned char **) * n);
    for (count = 0; count < n; count++)
    {
        blocks_data_decoding[count] = malloc(num_of_stripe * block_size);
    }

    for (count = 0; count < 3; count++)
    {
        memcpy(blocks_data_decoding[count], blocks_data[work_nodes[count]], num_of_stripe * block_size);
    }
    printf("before ec_encode_data, block_data: \n");
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < num_of_stripe * block_size; count2++)
        {
            printf("%d ", blocks_data_decoding[count][count2]);
        }
        printf("\n");
    }




    ec_encode_data(block_size * num_of_stripe, k, n - k, table_decoding, blocks_data_decoding, &blocks_data_decoding[k]);
    
    
    
    
    
    
    printf("after ec_encode_data, block_data: \n");
    for (count = 0; count < n; count++)
    {
        for (count2 = 0; count2 < num_of_stripe * block_size; count2++)
        {
            printf("%d ", blocks_data_decoding[count][count2]);
        }
        printf("\n");
    }
    printf("\n");
}