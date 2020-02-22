# include "myftp.h"

void listdir(){
    DIR *dir;
    struct dirent *entry;
    char ** d_array;
    int i = 0;

    if ((dir = opendir("data")) == NULL){
        perror("opendir() error");
    }
    else {
        while ((entry = readdir(dir)) != NULL) {
            printf("%s\n", entry->d_name);
        }
        closedir(dir);
    }
}

int main(void) {
    listdir();
    return 0;
}