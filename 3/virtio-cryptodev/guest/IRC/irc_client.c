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
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "cryptodev.h"
//#include <crypto/cryptodev.h>

#define CLIENT

#include "irc_server.h"

int main(int argc, char *argv[])
{
    int sockfd, port, event, maxfd, cfd;
    ssize_t n;
    char rbuff[DATA_SIZE],wrbuff[DATA_SIZE];
    char *hostname;
    struct hostent *hp;
    struct sockaddr_in sa;
    fd_set read_fds;

    cfd = open("/dev/cryptodev0", O_RDWR, 0);

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
            if (n == 0)
            {
                perror("Server disconnected.");
                exit(1);
            }
            
            decrypt(rbuff, cfd);
            fflush(stdout);
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            fflush(stdin);
            n = read(1,wrbuff, sizeof(wrbuff));
            wrbuff[n] = '\0'; // Add null character
            //printf("WRBUFF as read :%s -\n\n", wrbuff);

            encrypt(wrbuff, cfd);
            /*
            for(n =0; n < sizeof(data.encrypted); n++)
                printf("%c", data.encrypted[n]);
            printf("\n");*/
            if (insist_write(sockfd, data.encrypted, sizeof(data.encrypted)) != sizeof(data.encrypted)) 
            {
                perror("write");
                exit(1);
            }
        }
    }
    return 0;
}

