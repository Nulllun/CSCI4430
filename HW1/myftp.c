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
            printf("%lu\n", strlen(entry->d_name));
        }
        closedir(dir);
    }
}

void list_request() {
    struct message_s *msg = (struct message_s *)malloc(sizeof(msg));
    memcpy(msg->protocol, "myftp", 5);
    msg->type = 0xA1;
    msg->length = sizeof(msg);

}

int main(void) {
    list_request();
    return 0;
}