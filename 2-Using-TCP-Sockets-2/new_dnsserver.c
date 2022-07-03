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
#include <netdb.h>
#define SIZE 1024

int main()
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int e;

    int tcp_sockfd, udp_sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[SIZE];

    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_sockfd < 0)
    {
        perror("[-]Error in TCP Socket");
        exit(1);
    }
    printf("[+]TCP Socket created successfully.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&new_addr, 0, sizeof(new_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    e = bind(tcp_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e < 0)
    {
        perror("[-]TCP Binding Failed");
        exit(1);
    }
    printf("[+]TCP Binding successfull.\n");

    listen(tcp_sockfd, 10);
    
    printf("[+]Server Running...\n");

    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sockfd < 0)
    {
        perror("[-]Error in UDP Socket");
        exit(1);
    }
    printf("[+]UDP Socket created successfully.\n");

    e = bind(udp_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e < 0)
    {
        perror("[-]UDP Binding Failed");
        exit(1);
    }
    printf("[+]UDP Binding successfull.\n");
    
    addr_size = sizeof(new_addr);
    
    fd_set fds;
    FD_ZERO(&fds);

    int max_fd = ((tcp_sockfd>udp_sockfd)?tcp_sockfd:udp_sockfd) + 1;

    while (1)
    {
        FD_SET(tcp_sockfd, &fds);
        FD_SET(udp_sockfd, &fds);
        int nfd = select(max_fd, &fds, NULL, NULL, NULL);

        if(FD_ISSET(tcp_sockfd, &fds))
        {
            addr_size = sizeof(new_addr);
            new_sock = accept(tcp_sockfd, (struct sockaddr*)&new_addr, &addr_size);
            if(fork() == 0)
            {
                close(tcp_sockfd);

                int n;
                char buffer[SIZE];

                n = read(new_sock, buffer, SIZE);
                buffer[n]='\0';
                printf("\n[+]Recieved DNS Name from TCP: %s\n", buffer);

                struct hostent *ips = gethostbyname(buffer);

                if(ips == NULL)
                {
                    char err[] = "0.0.0.0";
                    send(new_sock, err, sizeof(err), 0);
                    printf("[-]Invalid Hostname.\n");
                    printf("[+]TCP Connection closed by client.\n");
                    close(new_sock);
                    exit(0);
                    continue;
                }

                int cnt = 0;
                char arr[SIZE];
                do
                {
                    if(!(ips->h_addr_list[cnt]))
                    {
                        char end[] = "\0";
                        send(new_sock, end, sizeof(end), 0);
                        printf("[+]End of sending.\n");
                        break;
                    }
                    strcpy(arr, inet_ntoa(*((struct in_addr*) ips->h_addr_list[cnt]))); 
                    if(!arr)
                        break;
                    cnt++;
                    printf("[+]IP Address %d Sent: %s\n", cnt, arr);
                    send(new_sock, (const char *)arr, sizeof(arr), 0);
                }
                while(arr != NULL);

                printf("[+]TCP Connection closed by client.\n");
                close(new_sock);
                exit(0);
            }
            close(new_sock);
        }

        if(FD_ISSET(udp_sockfd, &fds))
        {
            addr_size = sizeof(new_addr);

            if(fork() == 0)
            {
                int n;
                char buffer[SIZE];

                n = recvfrom(udp_sockfd, buffer, SIZE, 0, (struct sockaddr *) &new_addr, &addr_size);
                buffer[n]='\0';
                printf("\n[+]Recieved DNS Name from UDP: %s\n", buffer);

                struct hostent *ips = gethostbyname(buffer);

                if(ips == NULL)
                {
                    char err[] = "0.0.0.0";
                    sendto(udp_sockfd, err, sizeof(err), 0,(struct sockaddr*)&new_addr, sizeof(new_addr));
                    printf("[-]Invalid Hostname.\n");
                    printf("[+]UDP Connection closed by client.\n");
                    close(udp_sockfd);
                    exit(0);
                    continue;
                }

                int cnt = 0;
                char arr[SIZE];
                do
                {
                    if(!(ips->h_addr_list[cnt]))
                    {
                        char end[] = "\0";
                        sendto(udp_sockfd, end, sizeof(end), 0,(struct sockaddr*)&new_addr, sizeof(new_addr));
                        printf("[+]End of sending.\n");
                        break;
                    }
                    strcpy(arr, inet_ntoa(*((struct in_addr*) ips->h_addr_list[cnt]))); 
                    if(!arr)
                        break;
                    cnt++;
                    printf("[+]IP Address %d Sent: %s\n", cnt, arr);
                    sendto(udp_sockfd, (const char *)arr, sizeof(arr), 0, (const struct sockaddr *) &new_addr, sizeof(new_addr));
                }
                while(arr != NULL);

                printf("[+]UDP Connection closed by client\n");
                close(udp_sockfd);
                exit(0);
            }
        }
    }
}