// Assignment - 1b
// my_udpclient.c
// Mayank Kumar
// 19CS30029

// THE CLIENT PROCESS

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#define SIZE 100
#define MAX_LIMIT 100

void send_file(int fd, int sockfd, struct sockaddr_in server_addr, socklen_t addr_size)
{
    int n;
    char data[SIZE] = {0};

    while(read(fd, data, SIZE) > 0)
    {
        if (sendto(sockfd, data, sizeof(data), 0, (struct sockaddr*)&server_addr, addr_size) == -1)
        {
            perror("[-]Error in sending file");
            exit(1);
        }
        bzero(data, SIZE);
    }
    memcpy(data, "..", 3);
    if (sendto(sockfd, data, sizeof(data), 0, (struct sockaddr*)&server_addr, addr_size) == -1)
    {
        perror("[-]Error in terminating");
        exit(1);
    }
    bzero(data, SIZE);
}

int main(int argc, char *argv[])
{
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
    server_addr.sin_port = htons(6000);
    inet_aton(ip, &server_addr.sin_addr);

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("[-]Unable to open file");
        printf("[-]Exiting.\n");
        exit(0);
    }

    socklen_t addr_size = sizeof(server_addr);
    send_file(fd, sockfd, server_addr, addr_size);
    printf("[+]File data sent successfully.\n");
    
    int ccnt = 0, wcnt = 0, scnt = 0, return_status;
    return_status = recvfrom(sockfd, &ccnt, sizeof(ccnt), 0, (struct sockaddr*)&server_addr, &addr_size);

    if(return_status > 0)
    {
        printf("[+]Message received from server.\n");
        printf("   [+]Character Count: %d\n", ccnt);
    }
    else
        printf("   [-]Error in reading character count\n");
    
    return_status = recvfrom(sockfd, &wcnt, sizeof(wcnt), 0, (struct sockaddr*)&server_addr, &addr_size);
    if(return_status > 0)
        printf("   [+]Word Count: %d\n", wcnt);
    else
        printf("   [-]Error in reading word count\n");
    
    return_status = recvfrom(sockfd, &scnt, sizeof(scnt), 0, (struct sockaddr*)&server_addr, &addr_size);
    if(return_status > 0)
        printf("   [+]Sentence Count: %d\n", scnt);
    else
        printf("   [-]Error in reading sentence count\n");
    
    printf("[+]Closing the connection.\n");
    close(sockfd);

    return 0;
}
