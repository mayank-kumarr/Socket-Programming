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

#define SIZE 1024
#define TIMEOUT 2

int recvtimeout(int sockfd, char *buff2, struct sockaddr_in server_addr, socklen_t addr_size)
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

    return recvfrom(sockfd, (char *)buff2, SIZE, 0, (struct sockaddr*)&server_addr, &addr_size);
}

int main() 
{
    char buff1[SIZE];
    printf("Enter DNS name: ");
    scanf("%s", buff1);
    
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton(ip, &server_addr.sin_addr);

    sendto(sockfd, buff1, sizeof(buff1), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
    printf("[+]Sent DNS Name: %s\n", buff1);

    int n, cnt = 0;
    char buff2[SIZE];
    socklen_t addr_size = sizeof(server_addr);
    while(1)
    {
        n = recvtimeout(sockfd, buff2, server_addr, addr_size);
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