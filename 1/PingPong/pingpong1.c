#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
int main()
{
    int fd[4];
    if (pipe(fd) < 0) 
    {
        perror("OMG");
    }

    if(pipe(fd+2) < 0)
    {
        perror("Failed.");
    }

    dup2(fd[0],33);
    dup2(fd[1],34);
    dup2(fd[2],53);
    dup2(fd[3],54);

    pid_t pid = fork();
    if(pid > 0)
    {
        wait(NULL);
    }
    else
    {
        char* argv[] = {NULL};
        char* envp[] = {NULL};
        if(execve("/home/adonist/Desktop/riddlesyro/riddle", argv, envp) == -1)
        {
            perror("Could not run riddle\n");
        }
    
    }
    
    
    return 0;
}