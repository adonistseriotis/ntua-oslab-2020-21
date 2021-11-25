#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/mman.h>

int main() {
    pid_t pid = fork();
    if(pid>0)
    {
        char* argv[] = {NULL};
        char* envp[] = {NULL};
        if(execve("/home/adonist/Desktop/riddle-20201014/riddle", argv, envp) == -1)
        {
            perror("Could not run riddle\n");
        }
    }
    else
    {
        sleep(2);
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int fd = open("/home/adonist/Desktop/riddle-20201014/.hello_there", O_RDWR, mode);
        mmap(NULL,32768, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        ftruncate(fd,32768);
    }
    
    return 0;
}

