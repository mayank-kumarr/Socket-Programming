// CS39006: Networks Lab
// Assignment-5: Option-1 - Traceroute
// Mayank Kumar (19CS30029)
// my_traceroute.c

#include <arpa/inet.h>
#include <errno.h>
#include <linux/udp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define MAX 60
#define STARTID 10058
#define PAYLOAD 52
#define UDPHDRLEN 8
#define IPHDRLEN 20
#define ICMPHDRLEN 28
#define DESTPORT 32164
#define MAXHOPCOUNT 16
#define SEC 1
#define USEC 0

// UDP Checksum Calculation Codes

uint16_t check_udp_sum(uint8_t *buffer, int len)  // Standard UDP Checksum Code
{
    unsigned long sum = 0;
    struct iphdr *tempI = (struct iphdr *)(buffer);
    struct udphdr *tempH = (struct udphdr *)(buffer + IPHDRLEN);

    uint16_t *usBuff = (uint16_t *)&tempI->saddr;
    unsigned int cksum = 0;
    int isize = 8;
    for (; isize > 1; isize -= 2) {
        cksum += *usBuff++;
    }
    if (isize == 1) {
        cksum += *(uint16_t *)usBuff;
    }

    sum = cksum;

    usBuff = (uint16_t *)&tempH;
    isize = len;
    cksum = 0;
    for (; isize > 1; isize -= 2) {
        cksum += *usBuff++;
    }
    if (isize == 1) {
        cksum += *(uint16_t *)usBuff;
    }

    sum = sum + cksum;

    sum += ntohs(IPPROTO_UDP + len);

    sum = (sum >> len) + (sum & 0x0000ffff);
    sum += (sum >> len);
    sum = ~(sum & 0) >> len;

    return ~sum;
}

int main(int argc, char *argv[]) {
    if (argc < 2)  // Destination address should be given
    {
        printf("Argument needed\n");
        return 0;
    }
    char host[MAX], ip[15];
    strcpy(host, argv[1]);  // Get the destination hostname

    int ttl = 1;  // set TTL to 1
    int i;
    fd_set readfs;  // Variables for select call and calculating response time
    float response_time;
    int ndfs, r;
    int count;
    struct timeval timeout, sendt, recvt;

    char buf[UDPHDRLEN + IPHDRLEN + PAYLOAD],
        checkbuf[UDPHDRLEN + IPHDRLEN +
                 PAYLOAD];  // Final string to be sent, checkbuf is for creating
                            // checksum
    char msg[PAYLOAD];

    struct hostent *he = gethostbyname(host);  // get host by name
    if (he == NULL) {
        printf("%s: Name or service not known\n",
               host);  // If service is not known
        return 0;
    }

    struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
    strcpy(ip, inet_ntoa(*addr_list[0]));  // else pick up the IP Address of the
                                           // first entry of the address list

    printf("mytraceroute to %s (%s), %d hops max, %d byte packets\n", host, ip,
           MAXHOPCOUNT, PAYLOAD);
    int S1, S2;
    struct sockaddr_in saddr, dest_addr, raddr;
    int saddrlen, raddr_len, msglen;

    S1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);  // Create UDP Raw Socket S1
    if (S1 < 0) {
        perror("Cannot create raw UDP socket");
        exit(0);
    }

    S2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);  // Create ICMP Raw Socket S2
    if (S2 < 0) {
        perror("Cannot create raw ICMP socket");
        exit(0);
    }

    struct iphdr send_hdrIP, recv_ip_hdr;  // Create various header structures
    struct udphdr send_hdrUDP;
    struct icmp recv_icmp_hdr;

    dest_addr.sin_family = AF_INET;  // Destination Address Parameters
    inet_aton(ip, &dest_addr.sin_addr);
    dest_addr.sin_port = htons(DESTPORT);

    saddr.sin_family = AF_INET;  // Source Address Parameters
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddrlen = sizeof(saddr);

    if (setsockopt(S1, IPPROTO_IP, IP_HDRINCL, &(int){1}, sizeof(int)) <
        0)  // Setsockopt to include IPHDR_INCL
    {
        printf("Sockopt failed");
        exit(0);
    }
    if (bind(S1, (const struct sockaddr *)&saddr, saddrlen) <
        0)  // Bind S1 to source address
    {
        perror("Cannot bind raw UDP socket");
        exit(0);
    }

    if (bind(S2, (const struct sockaddr *)&saddr, saddrlen) <
        0)  // Bind S2 to source address
    {
        perror("Cannot bind raw ICMP socket");
        exit(0);
    }

    send_hdrIP.ihl = 5;  // Set ihl to 5, version to 5, tos to 0
    send_hdrIP.version = 4;
    send_hdrIP.tos = 0;
    send_hdrIP.tot_len = htons(IPHDRLEN + UDPHDRLEN + PAYLOAD);  // total len
    send_hdrIP.frag_off = 0;
    send_hdrIP.protocol = IPPROTO_UDP;
    send_hdrIP.saddr = saddr.sin_addr.s_addr;      // source address
    send_hdrIP.daddr = dest_addr.sin_addr.s_addr;  // destination address
    send_hdrUDP.source = htons(PORT);              // source port
    send_hdrUDP.dest = htons(DESTPORT);            // destination port
    send_hdrUDP.len = htons(UDPHDRLEN + PAYLOAD);  // length

    while (ttl <= MAXHOPCOUNT)  // Go up to max MAXHOPCOUNT hops
    {
        count = 0;  // for the first TTL, count is 0
        timeout.tv_sec = SEC;
        timeout.tv_usec = USEC;  // Set the timeout initially
        send_hdrIP.ttl = ttl;    // set ttl value
        while (count < 3) {
            FD_ZERO(&readfs);  // Set the fd_set properly
            FD_SET(S2, &readfs);
            ndfs = S2 + 1;
            send_hdrIP.id =
                htonl(ttl * 10 + count);  // Give an initial id to the msg
            for (i = 0; i < PAYLOAD; i++) msg[i] = rand() % 100;  // Send msg
            memcpy(checkbuf, &send_hdrIP, IPHDRLEN);  // Use the checkbuffers
            memcpy(checkbuf + IPHDRLEN, &send_hdrUDP, UDPHDRLEN);
            memcpy(checkbuf + IPHDRLEN + UDPHDRLEN, msg, PAYLOAD);

            send_hdrUDP.check =
                check_udp_sum(checkbuf, (IPHDRLEN + UDPHDRLEN + PAYLOAD) /
                                            2);  // Get the checksum

            memcpy(buf, &send_hdrIP, IPHDRLEN);  // Use the buffers
            memcpy(buf + IPHDRLEN, &send_hdrUDP, UDPHDRLEN);
            memcpy(buf + IPHDRLEN + UDPHDRLEN, msg, PAYLOAD);

            sendto(S1, buf, IPHDRLEN + UDPHDRLEN + PAYLOAD, 0,
                   (const struct sockaddr *)&dest_addr,
                   sizeof(dest_addr));   // Send the UDP packet
            gettimeofday(&sendt, NULL);  // get the current time

            r = select(ndfs, &readfs, 0, 0, &timeout);  // select call
            if (r == -1) {
                perror("Select Error\n");
                exit(0);
            }
            if (FD_ISSET(S2, &readfs))  // if ICMP Socket is triggered
            {
                raddr_len = sizeof(raddr);
                msglen = recvfrom(S2, buf, ICMPHDRLEN + IPHDRLEN, 0,
                                  (struct sockaddr *)&raddr,
                                  &raddr_len);  // recv the ICMP msg
                if (msglen < 0) {
                    printf("Cannot receive\n");
                    exit(0);
                }

                gettimeofday(&recvt, NULL);  // get the new time
                recv_icmp_hdr =
                    *((struct icmp *)(buf + IPHDRLEN));  // get the recv header
                                                         // from the same

                if (recv_icmp_hdr.icmp_type ==
                    11)  // if it is of type TIME LIMIT EXCEEDED
                {
                    response_time = (recvt.tv_sec - sendt.tv_sec) +
                                    (recvt.tv_usec - sendt.tv_usec) / 1000000.0;
                    printf("Hop_Count(%d)\t%s\t%.5fs\n", ttl,
                           inet_ntoa(raddr.sin_addr),
                           response_time);  // Print the response time and the
                                            // hop address
                    ttl++;
                    timeout.tv_sec = SEC;
                    timeout.tv_usec =
                        USEC;  // Update the timeout values and break from the
                               // (count<3) loop i.e go to next TTL value
                    break;
                } else if (recv_icmp_hdr.icmp_type == 3) {
                    if (strcmp(ip, inet_ntoa(raddr.sin_addr)) ==
                        0)  // if it is of type DESTINATION UNREACHABLE and the
                            // ip of the recved msg is same as the expected one
                    {
                        response_time =
                            (recvt.tv_sec - sendt.tv_sec) +
                            (recvt.tv_usec - sendt.tv_usec) / 1000000.0;
                        printf("Hop_Count(%d)\t%s\t%.5fs\n", ttl, ip,
                               response_time);  // Print the response time and
                                                // the final destination ip
                        close(S1);
                        close(S2);  // Close the sockets and exit
                        return 0;
                    }
                } else
                    ;  // do nothing for spurious packet
            }

            else  // On timeout
            {
                count++;
                timeout.tv_sec = SEC;  // Update count, timeout values
                timeout.tv_usec = USEC;

                if (count == 3)  // After three trails print *	*
                {
                    printf("Hop_Count(%d)\t*\t\t*\n", ttl);
                    ttl++;  // increase TTL value
                }
            }
        }
    }

    close(S1);  // Close the sockets and exit after 30 hop counts
    close(S2);
    return 0;
}