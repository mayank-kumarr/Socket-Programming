#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define MAXLINE 1024
#define TIMEOUT 2

int recvtimeout(int sockfd, char *buff2)
{
    fd_set fds;
    int n;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    n = select(sockfd+1, &fds, NULL, NULL, &tv);
    if(n == 0)
        return -2;
    if(n == -1)
        return -1;

    return read(sockfd, (char *)buff2, MAXLINE);
}

int main() 
{
    char buff1[MAXLINE];
    printf("Enter DNS name: ");
    scanf("%s", buff1);
    
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
	printf("[+]Connected to Server.\n");

    send(sockfd, buff1, sizeof(buff1), 0);
    printf("[+]Sent DNS Name: %s\n", buff1);

    int n, cnt = 0;
    char buff2[MAXLINE];
    socklen_t addr_size = sizeof(server_addr);
    while(1)
    {
        n = recvtimeout(sockfd, buff2);
        if(n == -2)
        {
            printf("[-]Timeout.\n");
            printf("[-]Exiting.");
            exit(1);
        }

        if(strcmp("\0", buff2) == 0)
        {
            printf("[+]End of receiving.\n");
            break;
        }
        if(strcmp("0.0.0.0", buff2) == 0)
        {
            printf("[-]Invalid Hostname.\n");
            printf("[-]IP Address Received: %s\n", buff2);
            break;
        }
        cnt++;
        printf("   [+]IP Address %d Received: %s\n", cnt, buff2);
    }

    printf("[+]Closing the connection.\n");
    close(sockfd);
    return 0;
}