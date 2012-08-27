#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>

#define PORT_TIME       13              /* "time" (not available on RedHat) */
#define MY_PORT        9999              /* FTP connection port */
#define SERVER_ADDR     "127.0.0.1"     /* localhost */
#define MAXBUF          1024

int main()
{   int sockfd;
    struct sockaddr_in dest;
    char buffer[MAXBUF];

    /*---Open socket for streaming---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("Socket");
        exit(errno);
    }

    /*---Initialize server address/port struct---*/
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(MY_PORT);
    if ( inet_aton(SERVER_ADDR, &dest.sin_addr.s_addr) == 0 )
    {
        perror(SERVER_ADDR);
        exit(errno);
    }

    /*---Connect to server---*/
    if ( connect(sockfd, &dest, sizeof(dest)) != 0 )
    {
        perror("Connect ");
        exit(errno);
    }

    /*---Get "Hello?"---*/
    bzero(buffer, MAXBUF);
    recv(sockfd, buffer, sizeof(buffer), 0);
    printf("%s", buffer);

    /*---Clean up---*/
    close(sockfd);
    return 0;
}
