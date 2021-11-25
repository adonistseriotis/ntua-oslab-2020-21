#ifndef _IRC_SERVER_H
#define _IRC_SERVER_H

struct
{
    char username[20];
    int sockfd;
    int speaks_with;
} user[10];

#define TCP_PORT    35001
#define BACKLOG     10
#define MAX_USERS   10

#define HELLO "Hello, please input username!\n"
#define USER_INERR "Please input a username with maximum 100 characters.\n"
#define ONLINE_USER "Please select user to speak to (Write 0-N):\n"
#define NO_ONLINE_USERS "No users available\n"


#endif