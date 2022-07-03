// rsocket.h file

#ifndef RSOCKET_H
#define RSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#include <stdbool.h>

#define T 2
#define T_NSEC 0
#define P 0.2
#define SOCK_MRP 20
#define HASH 11
#define MAX 100

// Function prototypes
void *thread_R(void *a);
void *thread_S(void *a);
int r_socket(int domain, int type, int protocol);
int r_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int r_recvfrom(int sockfd, char *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int r_close(int sockfd);
int dropMessage(float p);
void RecvMsg();
void RecvACKMsg(char buf[103]);
void HandleDataMsgRecv(char buf[103],struct sockaddr *src_addr,socklen_t addrlen);
void HandleRetransmit();
double getPerformance();

// Data structures
struct recvmsg {
    char msg[100];                    // for recv buffer
    struct sockaddr addr;
};

struct unackmsg {                    // for unacknowledged msg
    int id;
    int flags;
    char msg[100];
    time_t stime;
    struct sockaddr addr;
    struct unackmsg *next;
};

#endif
