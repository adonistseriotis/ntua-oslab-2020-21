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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>


#include "irc_server.h"

void clear_buff(char *buff,int n)
{
    int i;
    for(i=n; i<sizeof(buff); i++)
    {
        buff[i] = '\0';
    }
}
/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;
	
	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

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

int get_online_users(int connections, int me, int client_fd)
{
    char buff[100];
    int i;
    /* If no other users are available */
    if(connections == 1)
    {
        clear_buff(buff,0);
        strncpy(buff, NO_ONLINE_USERS, sizeof(buff));
        buff[sizeof(buff) - 1] = '\0';

        if((insist_write(client_fd, buff, sizeof(buff))) != sizeof(buff))
        {
            perror("write hello message");
            exit(1);
        }
        return -1;
    }

    for(i=-1; i<connections-1; i++)
    {
        /* Print initial message */
        if(i == -1)
        {
            clear_buff(buff,0);
            strncpy(buff, ONLINE_USER, sizeof(buff));
            buff[sizeof(buff) - 1] = '\0';      
        }

        /* Print all other users */
        if(i != me && i != -1)
        {
            clear_buff(buff,0);
            strncpy(buff, user[i].username, sizeof(buff));
            buff[sizeof(user[i].username)-1] = '\n';
            buff[sizeof(buff) - 1] = '\0';  
        }

        if(insist_write(client_fd, buff, sizeof(buff)) != sizeof(buff))
        {
            perror("write hello message");
            exit(1);
        }
    }
    return 0;
}

int handleUsernameInput(int clientfd, int serverfd, int connection, int all_connections)
{
    int n;
    char error[100];
    char username[20];

    clear_buff(username,0);
    n = read(clientfd, username, sizeof(username));
            /* Username input error handling */
    if (n < 0) 
    {
        strncpy(error, USER_INERR, sizeof(error));
        error[sizeof(error) - 1] = '\0';
        if(insist_write(clientfd, error, sizeof(error)) != sizeof(error))
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
        /* Save temp user to struct */
        username[n-2]='\0';
        strncpy(user[connection].username, username, n-1);
        //printf("\n\n\n\nSPEAKING WITH %s\n\n\n\n",user[connection].username);
        user[connection].speaks_with = serverfd;                                //Set client ready to select user to communicate with
        get_online_users(all_connections, connection,user[connection].sockfd);  //Display online user list
        return 1;
    }
    return 0;
}

/* Disconnect user and make speaking user go to home screen */
void disconnectUser(int connection, int serverfd, int all_connections)
{
    int n;
    char buff[100];
    int speaking_to,i;

    clear_buff(buff,0);
    n = sprintf(buff, "Sorry, %s has disconnected",user[connection].username);
    clear_buff(buff,n);
    /* Print disconnect message to other peer */
    if(insist_write(user[connection].speaks_with, buff, sizeof(buff)) != sizeof(buff))
    {
        perror("write disconnecting message");
        exit(1);
    }
    
    /* Find to who disconnecting user was speaking to */
    if((speaking_to = getUserFromfd(user[connection].speaks_with)) > 0)
    {
        user[speaking_to].speaks_with = serverfd;
        get_online_users(all_connections, connection, user[connection].speaks_with);
    }
    
    close(user[connection].sockfd);

    user[connection].sockfd = 0;
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
void display_connection(int me)
{
    char buff[80];
    int n;
    clear_buff(buff,0);
    n = sprintf(buff, "You are speakin to: %s\n",user[me].username);
    clear_buff(buff,n-1);

    if(insist_write(user[me].speaks_with, buff, n+1) != n+1)
    {
        perror("write disconnecting message");
        exit(1);
    }
}

/* Now chat */
int chat(int me)
{
    int n = 0;
    char buff[100];
    char wrbuff[124];
    /* Read message */
    clear_buff(buff,0);
    clear_buff(wrbuff,0);
    n = read(user[me].sockfd, buff, sizeof(buff));
    if(buff[0] == '\0')
    {
        return 0;
    }
    clear_buff(buff, n);
    if(n == 0)
    {
        return -1;
    }
    /* Print message where it's supposed to */
    printf("User %d with username: %s:",me, user[me].username);
    n = sprintf(wrbuff, "%s: %s", user[me].username, buff);
    clear_buff(wrbuff,n);

    if(insist_write(user[me].speaks_with, wrbuff, n+1) != n+1)
    {
        perror("write disconnecting message");
        exit(1);
    }

    clear_buff(buff,0);
    clear_buff(wrbuff,0);
    return n;
}



int main(void)
{
    char buff[100],userr[2];
    char addrstr[INET_ADDRSTRLEN];
    int serverfd, connection, maxfd, i,tempfd, event, selected_user;
    ssize_t n;
    socklen_t len;
    struct sockaddr_in sa;
    fd_set readfds;

    /* Initialise client fd array */
    for(i=0; i<MAX_USERS; i++)
    {
        user[i].sockfd = 0;
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


    /* Get hello message in buffer */
    clear_buff(buff,0);
    strncpy(buff, HELLO, sizeof(buff));
	buff[sizeof(buff) - 1] = '\0';

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
            len = sizeof(struct sockaddr_in);
            if ((user[connection].sockfd = accept(serverfd, (struct sockaddr *)&sa, &len)) < 0)
            {
                perror("accept");
                exit(1);
            }
            /* Initialize user struct */
            user[connection].username[0] = '\0';
            user[connection].speaks_with = 0-serverfd;  //Choose username

            /* Print message for incoming connection */
            if(!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr)))
            {
                perror("could not format IP address");
                exit(1);
            }
            fprintf(stderr, "Incoming connection from %s:%d\n", 
                addrstr, ntohs(sa.sin_port));
            
            /* Request username */
            if(insist_write(user[connection].sockfd, buff, sizeof(buff)) != sizeof(buff))
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
                    if((handleUsernameInput(tempfd, serverfd, i, connection)) < 0)
                    {
                        disconnectUser(i, serverfd, connection);
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
                    if(n == 0)
                    {
                        disconnectUser(i,serverfd,connection);
                        i--;
                        connection--;
                    }
                    else
                    {
                        selected_user = atoi(userr);
                        /* Start connection */
                        user[i].speaks_with = user[selected_user].sockfd;
                        user[selected_user].speaks_with = user[i].sockfd;
                        /* Display connection */
                        display_connection(i);
                        display_connection(selected_user);
                    }
                }

                /* Chat with ur friends */
                else
                {
                    if(chat(i)<0)
                    {
                        disconnectUser(i,serverfd,connection);
                        i--;
                        connection--;
                    }
                } 
            }
        }
    }
    return 0;
}