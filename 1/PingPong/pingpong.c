#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>

int main() {
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd = open("/home/adonist/Desktop/riddle-20201014/challenge6", O_RDWR , mode);
    dup2(fd, 34);
    int fd1 = open("/home/adonist/Desktop/riddle-20201014/player1", O_RDWR , mode);
    dup2(fd1, 33);
    int fd2 = open("/home/adonist/Desktop/riddle-20201014/player2", O_RDWR , mode);
    dup2(fd2, 53);
    int fd3 = open("/home/adonist/Desktop/riddle-20201014/score", O_RDWR , mode);
    dup2(fd3, 54);
    char* argv[] = {NULL};
    char* envp[] = {NULL};
    if(execve("/home/adonist/Desktop/riddle-20201014/riddle", argv, envp) == -1)
    {
        perror("Could not run riddle\n");
    }
    return 0;
}
