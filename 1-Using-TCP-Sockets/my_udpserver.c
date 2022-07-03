// Assignment - 1b
// my_udpserver.c
// Mayank Kumar
// 19CS30029

// THE SERVER PROCESS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SIZE 100

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int sockfd;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&new_addr, 0, sizeof(new_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6000);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e < 0)
    {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    while(1)
    {
        printf("\n[+]Server Waiting...\n");

        addr_size = sizeof(new_addr);
        int return_status = 1;

        int n;
        char recvd_txt[1000] = "";
        char buffer[SIZE];

        char ch, pch = '@';
        int exit_status = 0;
        int ccnt = 0, wcnt = 0, scnt = 0;
        int i = 0;

        while(1)
        {
            n = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr *) &new_addr, &addr_size);
            if (n < 0)
            {
                exit_status = 0;
                perror("[-]Unable to receive data from socket\n");
                break;
            }
            else if (n == 0)
            {
                exit_status = 0;
                perror("[-]Connection closed by client\n");
                break;
            }
            else
            {
                exit_status = 0;
                if(strcmp(buffer, "..") == 0)
                {
                    exit_status = 1;
                }
                else
                {
                    for(i=0; i<n; i++)
                    {
                        ch = buffer[i];
                        if(ch == '\0')
                            break;
                        ccnt++;
                        if(ch=='.' && pch!='.')
                            scnt++;
                        if(!((pch>=65 && pch <= 90) || (pch>=97 && pch<=122) || (pch>=48 && pch<=57)) && ((ch>=65 && ch <= 90) || (ch>=97 && ch<=122) || (ch>=48 && ch<=57)))
                            wcnt++;
                        pch = ch;
                    }
                }
                if(exit_status == 1)
                    break;
            }
            bzero(buffer, SIZE);
        }

        if(exit_status == 1)
        {
            printf("[+]File data received successfully.\n");

            sendto(sockfd, &ccnt, sizeof(ccnt), 0, (struct sockaddr*)&new_addr, addr_size);
            printf("[+]Character count sent successfully.\n");

            sendto(sockfd, &wcnt, sizeof(wcnt), 0, (struct sockaddr*)&new_addr, addr_size);
            printf("[+]Word count sent successfully.\n");

            sendto(sockfd, &scnt, sizeof(scnt), 0, (struct sockaddr*)&new_addr, addr_size);
            printf("[+]Sentence count sent successfully.\n");
        }
        else
        {
            printf("[-]File data not received.\n");
        }
        printf("[+]Connection closed by client\n");
    }
    
    printf("[+]Closing the connection.\n\n");
    close(sockfd);

    return 0;
}
