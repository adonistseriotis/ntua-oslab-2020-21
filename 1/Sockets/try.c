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

int main() {
    char message[25];
    char pid[5];
    char answer[5];
    struct sockaddr_in server, client;
    int sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd == -1)
    {
        perror("OMG");
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port=htons( 49842 );

    if(bind(sock_fd, (struct stockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Failed to bind");
    }

    listen(sock_fd, 3);

    int c = sizeof(struct sockaddr_in);
    int new_socket = accept(sock_fd, (struct sockaddr *)&client, (socklen_t*)&c);

    if (new_socket<0)
    {
        perror("accept failed");
        return 1;
    }

    read(4,message,25);
    for( int i=0; i<6; i++)
    {
        pid[i]=message[i+12];
    }
    int ans = atoi(pid) + 1;
    snprintf(answer,7,"%d",ans);
    write(4,answer,strlen(answer));
    return 0;
}