#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>

int main(int argc, char *argv[]) {
        char *file = argv[1];
        char *buff = argv[2];
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int fd = openat(AT_FDCWD, file, O_RDWR, mode);
        lseek(fd,111,SEEK_SET);
        write(fd, buff, 1);
        close(fd);
}