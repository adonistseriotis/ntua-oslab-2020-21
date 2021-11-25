/* 
 * Simple IRC server that supports 10 clients
 * 
 * George Syrogiannis <sirogiannisgiw@gmail.com>
 * Adonis Tseriotis <adonis.tseriotis@gmail.com>
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include"cryptodev.h"
//#include <crypto/cryptodev.h>

#define SERVER

#include "irc_server.h"


int getUserFromfd(int sockfd)
{
    int i;
    for(i=0; i<MAX_USERS; i++)
    {
        if(user[i].sockfd == sockfd)
        {
            return i;
        }
    }
    return -1;
}

int get_online_users(int connections, int me, int client_fd, int cfd)
{
    char temp[64],buff[DATA_SIZE];
    int i,n;

    memset(&temp,0,sizeof(temp));
    memset(&buff,0,sizeof(buff));
    /* If no other users are available */
    if(connections == 1)
    {
        strncpy(buff, NO_ONLINE_USERS, sizeof(buff));
        buff[sizeof(NO_ONLINE_USERS)] = '\0';   //Insert null char correctly

        encrypt(buff, cfd); //Encrypt

        if((insist_write(client_fd, data.encrypted, sizeof(data.encrypted))) != sizeof(data.encrypted))
        {
            perror("write wtf message");
            exit(1);
        }
        return -1;
    }

    for(i=-1; i<connections; i++)
    {
        memset(&buff,0,sizeof(buff));
        /* Print initial message */
        if(i == -1)
        {
            strncpy(temp, ONLINE_USER, sizeof(temp));
            temp[sizeof(temp)-1] = '\0'; 

            strncpy(buff,temp, sizeof(buff));
            encrypt(buff,cfd);
        }

        /* Print all other users */
        if(i != me && i != -1 && user[i].active == 1)
        {
            clear_buff(temp,0);
            strcpy(temp, user[i].username);
            temp[sizeof(user[i].username)-1] = '\n';
            temp[sizeof(user[i].username)] = '\0';  

            n = sprintf(buff, "\033[1;36m%d:\033[0m %s\n", i, temp);
            buff[n] = '\0';

            encrypt(buff, cfd);
        }

        if(i != me)
        {
            if(insist_write(client_fd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
            {
                perror("write omg message");
                exit(1);
            }
        }
    }
    return 0;
}

void needs_refresh(int serverfd, int all_connections, int cfd)
{
    int i=-1,n;
    char buff[DATA_SIZE];

    for(i=0; i<all_connections-1; i++)
    {
        if(user[i].speaks_with == serverfd && user[i].active == 1)
        {
            if(all_connections == 2)
            {
                n = sprintf(buff, "^%c[2K\r", 27);
                buff[n]='\0';
                
                encrypt(buff, cfd);

                if(insist_write(user[i].sockfd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
                {
                    perror("write disconnecting message");
                    exit(1);
                }
                get_online_users(all_connections, i, user[i].sockfd, cfd);
            }

            else if (all_connections > 2)
            {
                n = sprintf(buff, "\n\033[1;36m%d:\033[0m %s\n", all_connections-1, user[all_connections-1].username);
                buff[n] = '\0';

                encrypt(buff, cfd);

                if(insist_write(user[i].sockfd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
                {
                    perror("write disconnecting message");
                    exit(1);
                }
            }
            
        }
    }
}

int handleUsernameInput(int clientfd, int serverfd, int connection, int all_connections, int cfd)
{
    int n;
    char error[DATA_SIZE];
    char username[DATA_SIZE];

    memset(&username,0,sizeof(username));
    n = read(clientfd, username, sizeof(username));

    /* Username input error handling */
    if (n < 0) 
    {
        strncpy(error, USER_INERR, sizeof(error));
        error[sizeof(error) - 1] = '\0';

        encrypt(error, cfd);

        if(insist_write(clientfd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
        {
            perror("write username usage message");
            exit(1);
        }
    }
    else if (n == 0)
    {
        return -1;
    }
    else
    {
        //Fails for some fucking reason
        decrypt(username,cfd);
        
        username[strlen(username)-1]='\0';
        /* Save temp user to struct */
        strncpy(user[connection].username, username, sizeof(user[connection].username));
        //printf("\n\n\n\nSPEAKING WITH %s\n\n\n\n",user[connection].username);
        user[connection].speaks_with = serverfd;    //Set client ready to select user to communicate with
        get_online_users(all_connections,connection,clientfd,cfd);  //Display online user list
        
        needs_refresh(serverfd, all_connections, cfd);
        return 1;
    }
    return 0;
}

/* Disconnect user and make speaking user go to home screen */
void disconnectUser(int connection, int serverfd, int all_connections, int cfd)
{
    int n;
    char buff[DATA_SIZE];
    int speaking_to,i;

    memset(&buff,0,sizeof(buff));

    printf("User %s has disconnected...\n",user[connection].username);
    
    if(user[connection].speaks_with != serverfd)
    {
    
        n = sprintf(buff, "Sorry, %s has disconnected\n",user[connection].username);
        clear_buff(buff,n);

        encrypt(buff, cfd);
        /* Print disconnect message to other peer */
        if(insist_write(user[connection].speaks_with, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
        {
            perror("write disconnecting message");
            exit(1);
        }
        
        /* Find to who disconnecting user was speaking to */
        speaking_to = getUserFromfd(user[connection].speaks_with);
        if(speaking_to >= 0)
        {
            user[speaking_to].speaks_with = serverfd;
            get_online_users(all_connections, speaking_to, user[connection].speaks_with, cfd);
        }
    }
    
    close(user[connection].sockfd);

    user[connection].sockfd = 0;
    user[connection].active = 0;

    /* Rectify user struct */
    for(i=connection; i<MAX_USERS-1; i++)
    {
        if(user[i].sockfd == 0 && user[i+1].sockfd != 0)
        {
            user[i].sockfd = user[i+1].sockfd;
            user[i+1].sockfd = 0;
        }
    }
    
}

/* Function to display that connection is made to users */
void display_connection(int me, int cfd)
{
    char buff[DATA_SIZE];
    int n;
    clear_buff(buff,0);
    n = sprintf(buff, "You are speakin to: %s\n",user[me].username);
    clear_buff(buff,n-1);

    encrypt(buff, cfd);

    if(insist_write(user[me].speaks_with, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
    {
        perror("write disconnecting message");
        exit(1);
    }
}

/* Now chat */
int chat(int me, int cfd)
{
    int n = 0;
    char buff[DATA_SIZE];
    char wrbuff[DATA_SIZE+128];

    /* Clear buffers */
    memset(&buff,0,sizeof(buff));
    memset(&wrbuff,0,sizeof(buff));

    /* Read message */
    n = read(user[me].sockfd, buff, sizeof(buff));

    if(n == 0)
    {
        return -1;
    }

    decrypt(buff, cfd);

    /* Bug fix */
    if(buff[0] == '\0')
    {
        return 0;
    }

    /* Print message where it's supposed to */
    n = sprintf(wrbuff, "\033[1;32m%s:\033[0m %s\033[1;32mme:\033[0m", user[me].username, buff);
    clear_buff(wrbuff,n);
    
    encrypt(wrbuff, cfd);

    if(insist_write(user[me].speaks_with, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
    {
        perror("write disconnecting message");
        exit(1);
    }

    return n;
}


int main(void)
{
    char buff[DATA_SIZE],userr[DATA_SIZE];
    char addrstr[INET_ADDRSTRLEN];
    int serverfd, connection, maxfd, i,tempfd, event, selected_user;
    ssize_t n;
    socklen_t len;
    struct sockaddr_in sa;
    fd_set readfds;
    int cfd;
    
    /* Start communication with crypto driver */ 
    cfd = open("/dev/cryptodev0", O_RDWR, 0);

    /* Initialise client fd array */
    for(i=0; i<MAX_USERS; i++)
    {
        user[i].sockfd = 0;
        user[i].active = 0;
    }

    /* Ignore SIGPIPE signals */
    signal(SIGPIPE, SIG_IGN);

    /* Create TCP/IP socket */
    if ((serverfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");

    /* Bind to TCP well-known port */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(TCP_PORT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverfd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
        perror("bind");
        exit(1);
    }
    fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);

    /* Listen for incoming connections */
    if(listen(serverfd, BACKLOG) < 0)
    {
        perror("listen");
        exit(1);
    }

    /* Initialize connection number */
    connection = 0;
    fprintf(stderr, "Waiting for an incoming connection...\n");

    for(;;) 
    {
        /*  Remove all fds from set */
        FD_ZERO(&readfds);

        /* Add server socket to set */
        FD_SET(serverfd, &readfds);
        maxfd = serverfd;

        /* Add all active connections to set */
        for (i=0; i<MAX_USERS; i++)
        {
            tempfd = user[i].sockfd;

            /* If the client is online add it to the list */
            if(tempfd > 0)
                FD_SET(tempfd,&readfds);
            
            /* Update max fd */
            if(tempfd > maxfd)
            {
                maxfd = tempfd;
            }
        }
        
        /* Wait for some event to take place */
        event = select(maxfd+1, &readfds, NULL, NULL, NULL);

        if((event < 0) && (errno != EINTR))
        {
            printf("Something wrong with select\n");
        }


        /* Accept an incoming connection */
        if (FD_ISSET(serverfd, &readfds))
        {
            /* Get hello message in buffer */
            memset(&buff,0, sizeof(buff));
            strncpy(buff, HELLO, sizeof(buff));
	        buff[sizeof(HELLO)] = '\0';

            len = sizeof(struct sockaddr_in);
            if ((user[connection].sockfd = accept(serverfd, (struct sockaddr *)&sa, &len)) < 0)
            {
                perror("accept");
                exit(1);
            }
            /* Initialize user struct */
            user[connection].username[0] = '\0';
            user[connection].speaks_with = 0-serverfd;  //Choose username
            user[connection].active = 1;    //Make active

            /* Print message for incoming connection */
            if(!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr)))
            {
                perror("could not format IP address");
                exit(1);
            }
            fprintf(stderr, "Incoming connection from %s:%d\n", 
                addrstr, ntohs(sa.sin_port));
            
            /* Request username */
            encrypt(buff, cfd);

            if(insist_write(user[connection].sockfd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
            {
                perror("write hello message");
                exit(1);
            }

            /* Increment number of connections */
            connection++;
        }
        
        /* Find which client has requested IO */
        for(i=0; i<connection; i++)
        {
            tempfd = user[i].sockfd;

            if (FD_ISSET(tempfd, &readfds))
            {
                /* Input username */
                
                if (user[i].speaks_with == 0-serverfd)
                {
                    if((handleUsernameInput(tempfd, serverfd, i, connection, cfd)) < 0)
                    {
                        disconnectUser(i, serverfd, connection, cfd);
                    /* Fix to check all users if someone disconnects */
                        i--;
                        connection--; //Decrement number of connections
                    }

                    /* By now we have gotten username or user has disconnected */
                }
                
                /* Choose user to speak to */
                else if (user[i].speaks_with == serverfd)
                {
                    n = read(user[i].sockfd, userr, sizeof(userr));

                    decrypt(userr, cfd);

                    if(n == 0)
                    {
                        disconnectUser(i,serverfd,connection,cfd);
                        i--;
                        connection--;
                    }
                    else
                    {
                        if(isNumber(userr)>0)
                        {
                            selected_user = atoi(userr);
                        }
                        else
                        {
                            printf("I AM HERE ALSO\n");
                            selected_user = MAX_USERS + 1;
                        }
                        
                        if(selected_user > connection || user[selected_user].active == 0)
                        {
                            /* If input is wrong print try again message */
                            printf("I AM HERE\n");
                            memset(&buff,0, sizeof(buff));
                            strncpy(buff, TRY_IT_AGAIN, sizeof(buff));
                            buff[sizeof(TRY_IT_AGAIN)] = '\0';

                            encrypt(buff, cfd);

                            if(insist_write(user[i].sockfd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted))
                            {
                                perror("write try again message");
                                exit(1);
                            }
                        }
                        else
                        {
                            /* Start connection */
                            user[i].speaks_with = user[selected_user].sockfd;
                            user[selected_user].speaks_with = user[i].sockfd;
                            /* Display connection */
                            display_connection(i, cfd);
                            display_connection(selected_user, cfd);
                        }
                    }
                }

                /* Chat with ur friends */
                else
                {
                    if(chat(i, cfd)<0)
                    {
                        disconnectUser(i,serverfd,connection,cfd);
                        i--;
                        connection--;
                    }
                } 
            }
        }
    }

    close(cfd);
    return 0;
}