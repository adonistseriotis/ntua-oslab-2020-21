#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>

int main() {
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char buff[4];
    for(int i=0; i<10; i++)
    {
        snprintf(buff,5,"bf0%d",i);
        int fd = open(buff, O_CREAT | O_RDWR | O_TRUNC, mode);
        lseek(fd,1073741824,SEEK_SET);
        write(fd,"DATA",4);
        close(fd);
    }
    char* argv[] = {NULL};
    char* envp[] = {NULL};
    if(execve("/home/adonist/Desktop/riddle-20201014/riddle", argv, envp) == -1)
    {
        perror("Could not run riddle\n");
    }
    return 0;
}