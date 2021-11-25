#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>

int main() {
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd = open("/home/adonist/Desktop/riddle-20201014/challenge5", O_CREAT | O_RDWR | O_TRUNC, mode);
    dup2(fd, 99);
    char* argv[] = {NULL};
    char* envp[] = {NULL};
    if(execve("/home/adonist/Desktop/riddle-20201014/riddle", argv, envp) == -1)
    {
        perror("Could not run riddle\n");
    }
    return 0;
}
