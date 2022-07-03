#include "rsocket.h"

#define PORT1 50059
#define PORT2 50060

int main() {
    int sockfd, len;
    struct sockaddr_in srcaddr, destaddr;
    memset(&srcaddr, 0, sizeof(srcaddr));
    memset(&destaddr, 0, sizeof(destaddr));

    sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(0);
    }

    srcaddr.sin_family = AF_INET;
    srcaddr.sin_port = htons(PORT1);
    srcaddr.sin_addr.s_addr = INADDR_ANY;

    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(PORT2);
    destaddr.sin_addr.s_addr = INADDR_ANY;

    if (r_bind(sockfd, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) < 0) {
        perror("Bind failed");
        exit(0);
    }

    char str[102], buf[2];
    buf[1] = '\0';
    printf("Enter a string: ");
    fgets(str, 102, stdin);
    str[strlen(str) - 1] = '\0';

    int i = 0;
    while (1) {
        buf[0] = str[i++];
        if (buf[0] == '\0') break;
        r_sendto(sockfd, (char *)buf, strlen(buf) + 1, 0, (struct sockaddr *)&destaddr,
                 sizeof(destaddr));
        if(i >= strlen(str))
            break;
    }
    r_close(sockfd);
    printf("%.3lf\n", getPerformance());

    return 0;
}
