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

int main()
{
    char buff[8];
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    pid_t pid = fork();
    int fd = openat(AT_FDCWD,"secret_number",O_CREAT | O_RDWR | O_TRUNC, mode);
    if(pid > 0)
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
        sleep(3);
        read(fd,buff,8);
        write(1,buff,sizeof(buff));
        printf("\n");
        close(fd);
    }
    
    return 0;
}