#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <string.h>

int main() {
    pid_t pid=fork();
    
    if(pid>0)
    {
        sleep(3);
        char* argv[] = {NULL};
        char* envp[] = {NULL};
        if(execve("/home/adonist/Desktop/riddle-20201014/riddle", argv, envp) == -1)
        {
            perror("Could not run riddle\n");
        }
    }
    else
    {
        char * message;
        struct sockaddr_in server, client;
        int sock_fd = socket(AF_INET,SOCK_STREAM,0);
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port=htons(49842);

        if(bind(sock_fd, (struct stockaddr *)&server, sizeof(server)) < 0)
        {
            perror("Failed to bind!");
        }

        listen(sock_fd, 3);

        int c = sizeof(struct sockaddr_in);
        int new_socket = accept(sock_fd, (struct sockaddr *)&client, (socklen_t*)&c);

            //Reply to the client
        message = "203062";
        write(new_socket , message , strlen(message));

        if (new_socket<0)
        {
            perror("accept failed");
            return 1;
        }
    }
    
    return 0;
}