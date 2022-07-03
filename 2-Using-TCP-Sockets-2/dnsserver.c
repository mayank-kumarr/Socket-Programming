#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#define SIZE 1024

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
    server_addr.sin_port = htons(port);
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

        int n;
        char buffer[SIZE];

        n = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr *) &new_addr, &addr_size);
        buffer[n]='\0';
        printf("[+]Recieved DNS Name: %s\n", buffer);
        
        struct hostent *ips = gethostbyname(buffer);
        
        if(ips == NULL)
        {
            char err[] = "0.0.0.0";
            sendto(sockfd, err, sizeof(err), 0,(struct sockaddr*)&new_addr, sizeof(new_addr));
            printf("[-]Invalid Hostname.\n");
            printf("[+]Connection closed by client\n");
            continue;
        }

        int cnt = 0;
        char arr[SIZE];
        do
        {
            if(!(ips->h_addr_list[cnt]))
            {
                char end[] = "\0";
                sendto(sockfd, end, sizeof(end), 0,(struct sockaddr*)&new_addr, sizeof(new_addr));
                printf("[+]End of sending.\n");
                break;
            }
            strcpy(arr, inet_ntoa(*((struct in_addr*) ips->h_addr_list[cnt]))); 
            if(!arr)
                break;
            cnt++;
            printf("[+]IP Address %d Sent: %s\n", cnt, arr);
            sendto(sockfd, (const char *)arr, strlen(arr), 0, (const struct sockaddr *) &new_addr, sizeof(new_addr));
        }
        while(arr != NULL);

        printf("[+]Connection closed by client\n");
    }
    
    printf("[+]Closing the connection.\n\n");
    close(sockfd);

    return 0;
}