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
    char buff[70];
    char ans[10];
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
        read(fd,buff,70);
        int i,j=0;
        //write(1,buff,strlen(buff)+1);
        while(!((buff[i] >= 0x30 && buff[i] <= 0x39) || (buff[i]>=0x41 && buff[i]<=0x46)))
        {
            i++;
        }
        
        while((buff[i] >= 0x30 && buff[i] <= 0x39) || (buff[i]>=0x41 && buff[i]<=0x46))
        {
            ans[j++]=buff[i++];
        }
        write(1,ans,sizeof(ans));
        close(fd);
    }
    
    return 0;
}