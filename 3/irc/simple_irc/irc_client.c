/* 
 * Simple IRC client that speaks to implemented server
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

int main(int argc, char *argv[])
{
    int sockfd, port, event, maxfd;
    ssize_t n;
    char rbuff[1024],wrbuff[1024];
    char *hostname;
    struct hostent *hp;
    struct sockaddr_in sa;
    fd_set read_fds;

    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s [hostname] [port]\n", argv[0]);
        exit(1);
    }

    hostname = argv[1];
    port = atoi(argv[2]);
    if(port > 65535 || port <= 0)
    {
        fprintf(stderr, "Usage: port must be between 1 and 65535");
        exit(1);
    }

    /* Create TCP/IP socket */
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");

    /* Look up remote hostname by DNS */
    if (!(hp = gethostbyname(hostname))) 
    {
		printf("DNS lookup failed for host %s\n", hostname);
		exit(1);
	}

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
    fprintf(stderr, "Connecting to remote host.... "); fflush(stderr);

    if (connect(sockfd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
        perror("connect");
        exit(1);
    }
    fprintf(stderr, "Connected.\n");

    for(;;)
    {
        FD_ZERO(&read_fds);

        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        
        maxfd = (sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO);

        event = select(maxfd+1, &read_fds, NULL, NULL, NULL);

        if((event < 0) && (errno != EINTR))
        {
            printf("Something wrong with select\n");
        }

        /* Accept message sent from server */
        if(FD_ISSET(sockfd, &read_fds))
        {
            n = read(sockfd, rbuff, sizeof(rbuff));
            if (n < 0) {
                perror("read");
                exit(1);
            }

            if (insist_write(0, rbuff, n) != n) {
                perror("write");
                exit(1);
            }
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            n = read(1,wrbuff, sizeof(wrbuff));
            clear_buff(wrbuff,n);
            wrbuff[n] = '\0';
            
            if (insist_write(sockfd, wrbuff, n+1) != n+1) 
            {
                perror("write");
                exit(1);
            }
        }
    }
    return 0;
}

