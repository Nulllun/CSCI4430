#include "myftp.h"


int BLOCK_SIZE = 4096;
int n = 5;
int k = 3;
int stripe_num;
char *filename = "test.txt";
//char *filename = "testAudio.mp3";
//char *filename = "testZip.zip";

typedef struct stripe {
    int sid;
    void **data_block;
    void **parity_block;
} Stripe;


int main(){

    // read the file first
    int file_size;
    struct stat st;
    void *buff;
    if (stat(filename, &st) == 0)
    {
        // the file exist
        file_size = st.st_size;
        FILE *fptr = fopen(filename, "rb");
        buff = (void *)malloc(file_size);
        fread(buff, file_size, 1, fptr);
        free(buff);
        fclose(fptr);
    }
    printf("Read success, the file size is %d bytes\n",file_size);

    // calculate stripe num
    stripe_num = ceil(file_size / (BLOCK_SIZE * k));
    if(file_size % (BLOCK_SIZE * k) != 0){
        stripe_num = stripe_num + 1;
    }
    printf("Number of stripes is %d\n", stripe_num);

    // allocate space for matrix
    uint8_t *encode_matrix = malloc(sizeof(uint8_t) * (n * k));
    uint8_t *errors_matrix = malloc(sizeof(uint8_t) * (k * k));
    uint8_t *invert_matrix = malloc(sizeof(uint8_t) * (k * k));

    gf_gen_rs_matrix(encode_matrix, n, k);

    return 0;
}