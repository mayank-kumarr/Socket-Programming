// rsocket.c file
#include "rsocket.h"

struct recvmsg* recvbuffer;    // Receive buffer
struct unackmsg** unackmsgtb;  // Unacknowledged msg table
bool* recvmsgidtb;             // Received msg id table

int next_unused_id;     // next_unused_id to assign ids
int size_unackmsgtb;    // size of tables
int N_retransmissions;  // number of retransmissions
int udpsockfd;          // udp socket
int rbuf_recvend;       // recv buffer pointers
int rbuf_addend;

pthread_mutex_t recvbufferlock;  // locks
pthread_mutex_t unackmsgtblock;

void RetransmitMsg()  // Handle retransmission upon timeout
{
    time_t currt;
    currt = time(NULL);  // get the current time
    int i;
    char buf[MAX + 3];
    struct sockaddr addr;
    struct unackmsg* temp;

    for (i = 0; i < HASH; i++)  // Traverse through the hash table
    {
        temp = unackmsgtb[i];
        while (temp != NULL) {
            if ((temp->stime + 2*T) <=
                currt)  // if the timeout for a certain msg has occured
            {
                memset(buf, '\0', MAX + 3);  // make buf null string
                buf[0] = 'D';
                buf[1] = (temp->id / 10) % 10 + '0';
                buf[2] = (temp->id) % 10 + '0';  // store M followed by id
                strcat(buf, temp->msg);          // concatenate the msg
                addr = temp->addr;
                sendto(udpsockfd, buf, strlen(buf), temp->flags,
                       (const struct sockaddr*)&addr, sizeof(addr));
                N_retransmissions++;  // number of retransmissions increase by 1
                pthread_mutex_lock(&unackmsgtblock);
                temp->stime = currt;  // new sending time set to current times
                pthread_mutex_unlock(&unackmsgtblock);
            }
            temp = temp->next;
        }
    }
    return;
}

void RecvDataMsg(char buf[MAX + 3], struct sockaddr* cliaddr, socklen_t clilen) {
    char ack[4];  // acknowledgement msg
    char msg[MAX];
    int i;
    int id = (buf[1] - '0') * 10 + (buf[2] - '0');  // get the id

    if (recvmsgidtb[id - 1] == true)  // if the id has already been received
    {
        ack[0] = 'A';
        ack[1] = buf[1];
        ack[2] = buf[2];
        ack[3] = '\0';
        sendto(udpsockfd, ack, strlen(ack), 0, cliaddr, clilen);
        return;
    }

    for (i = 0; buf[i + 3] != '\0'; i++)  // get the msg
    {
        msg[i] = buf[i + 3];
    }

    pthread_mutex_lock(&recvbufferlock);
    strcpy(recvbuffer[rbuf_addend].msg, msg);  // store it in the recv buffer
    recvbuffer[rbuf_addend].addr = *cliaddr;
    rbuf_addend = (rbuf_addend + 1) % MAX;
    pthread_mutex_unlock(&recvbufferlock);

    // send an ack
    ack[0] = 'A';
    ack[1] = buf[1];
    ack[2] = buf[2];
    ack[3] = '\0';
    sendto(udpsockfd, ack, strlen(ack), 0, cliaddr, clilen);  // send the ack
    recvmsgidtb[id - 1] = true;

    return;
}

void RecvACKMsg(char buf[MAX + 3]) {  // Handle ack
    int id = (buf[1] - '0') * 10 + (buf[2] - '0');  // get the id
    int hashindex = id % HASH;
    struct unackmsg* a =
        unackmsgtb[hashindex];  // go to the corresponding list in hash table
    struct unackmsg* temp;
    if (a != NULL &&
        a->id == id)  // if the first element of the list is acknowledged, simply move the
                      // list to next element and free the first element
    {
        pthread_mutex_lock(&unackmsgtblock);
        unackmsgtb[hashindex] = a->next;
        free(a);
        size_unackmsgtb--;  // size is reduced by 1
        pthread_mutex_unlock(&unackmsgtblock);
        return;
    }

    while (a->next != NULL)  // else traverse through the list and check
    {
        if (a->next->id ==
            id)  // if the msg is present in the unackmsg table at the next position
        {
            pthread_mutex_lock(&unackmsgtblock);
            temp = a->next;  // we need to free the next element and move to next->next
                             // element
            a->next = a->next->next;
            free(temp);
            size_unackmsgtb--;  // reduce the size
            pthread_mutex_unlock(&unackmsgtblock);
            return;
        }
        a = a->next;
    }
    return;
}

void RecvMsg() {  // Handle the received msg
    char buf[MAX + 3];  // buf stores the recved msg
    int r;
    struct sockaddr src_addr;  // Address parameters
    socklen_t addrlen;

    addrlen = sizeof(src_addr);
    for (r = 0; r < MAX + 3; r++) buf[r] = '\0';  // clear buf
    r = recvfrom(udpsockfd, buf, MAX + 3, 0, (struct sockaddr*)&src_addr,
                 &addrlen);  // receive msg
    if (r < 3) return;
    buf[r] = '\0';

    // if we drop the msg, we simply return otherwise we handle the msg
    int drop = dropMessage(P);
    if (drop == 1) return;

    if (buf[0] == 'D') {
        RecvDataMsg(buf, (struct sockaddr*)&src_addr, addrlen);  // Handle app msg
    } else if (buf[0] == 'A') {
        RecvACKMsg(buf);  // Handle ack msg
    }

    return;
}

void* thread_R(void* a) {
    while (1) {
        RecvMsg();
    }
}

void* thread_S(void* a) {
    struct timespec tim, tim2;
    tim.tv_sec = T;
    tim.tv_nsec = T_NSEC;
    if(nanosleep(&tim , &tim2) < 0) {
        perror("Error in nanosleep");
        exit(0);
    }
    while(1) {
        RetransmitMsg();
    }
}

int r_socket(int domain, int type, int protocol) {
    if (type != SOCK_MRP) {
        return -1;
    }
    udpsockfd = socket(domain, SOCK_DGRAM, protocol);  // create udp socket
    if (udpsockfd < 0) return -1;

    srand(time(0));

    // create locks and threads
    pthread_mutex_init(&recvbufferlock, NULL);
    pthread_mutex_init(&unackmsgtblock, NULL);
    pthread_t R, S;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&R, &attr, thread_R, NULL);
    pthread_create(&S, &attr, thread_S, NULL);

    // Size of buffers and other variables initialised to 0
    size_unackmsgtb = 0;
    next_unused_id = 0;
    N_retransmissions = 0;
    rbuf_recvend = 0;
    rbuf_addend = 0;

    // Allocate space to buffers
    recvbuffer = (struct recvmsg*)calloc(MAX, sizeof(struct recvmsg));
    unackmsgtb = (struct unackmsg**)calloc(HASH, sizeof(struct unackmsg*));
    recvmsgidtb = (bool*)calloc(MAX, sizeof(bool));

    for (int i = 0; i < MAX; i++) recvmsgidtb[i] = false;

    // return socket id
    return udpsockfd;
}

int r_sendto(int udpsockfd, const void* b, size_t len, int flags,
             const struct sockaddr* dest_addr, socklen_t addrlen) {
    char* buf = (char*)b;
    char msg[103];  // Msg of 103 bytes
    int r;

    int i;
    for (i = 0; i < len; i++) {
        msg[i + 3] = buf[i];  // store msg
    }

    msg[0] = 'D';
    msg[1] = (next_unused_id / 10) % 10 + '0';  // X is the tens place of msg id
    msg[2] = next_unused_id % 10 + '0';         // Y is the units place of msg id

    r = sendto(udpsockfd, msg, len + 3, flags, dest_addr,
               addrlen);  // Send the msg to the destination address
    if (r < 3) {
        return -1;  // if less than 3 bits are sent
    }

    pthread_mutex_lock(&unackmsgtblock);
    struct unackmsg* a =
        (struct unackmsg*)malloc(sizeof(struct unackmsg));  // create new node
    a->stime = time(NULL);    // current time is stored in stime
    a->addr = *dest_addr;     // IP-port for the msg
    a->id = next_unused_id;   // id is next_unused_id
    strcpy(a->msg, msg + 3);  // msg
    a->flags = flags;         // Flags
    int hashindex = a->id % HASH;
    a->next = unackmsgtb[hashindex];
    unackmsgtb[hashindex] = a;
    size_unackmsgtb++;  // increase the size of the unackmsgtb
    pthread_mutex_unlock(&unackmsgtblock);

    next_unused_id++;  // next_unused_id is incremented
    return 0;
}

int r_recvfrom(int udpsockfd, char* buf, size_t len, int flags, struct sockaddr* src_addr,
               socklen_t* addrlen) {
    int i;
    if (flags != MSG_DONTWAIT)  // if we have to wait for the msg
    {
        while (1) {
            if (rbuf_recvend == rbuf_addend) {
                sleep(1);  // sleep till the recv buffer is not having at least 1 element
            } else
                break;
        }
    } else {
        if (rbuf_recvend == rbuf_addend)  // If we use MSG_DONTWAIT flag and recv buffer is empty
        {
            src_addr = NULL;
            addrlen = NULL;
            memset(buf, '\0', len);
            return -1;  // make everything null and return -1
        }
    }
    int msglen = strlen(recvbuffer[rbuf_recvend].msg);
    int returnlen;

    if (msglen < len)  // if the msg is smaller than the requested bytes
    {
        pthread_mutex_lock(&recvbufferlock);
        strcpy(buf, recvbuffer[rbuf_recvend].msg);  // copy the msg
        *src_addr = recvbuffer[rbuf_recvend].addr;  // copy the src_addr
        *addrlen = sizeof(*src_addr);
        returnlen = strlen(buf);  // return len is the length of the msg
        rbuf_recvend = (rbuf_recvend + 1) % MAX;
        pthread_mutex_unlock(&recvbufferlock);
        return returnlen;  // return
    } else {
        pthread_mutex_lock(&recvbufferlock);
        for (i = 0; i < len; i++)
            buf[i] =
                recvbuffer[rbuf_recvend].msg[i];  // else copy the first len characters
        *src_addr = recvbuffer[rbuf_addend].addr;
        *addrlen = sizeof(*src_addr);
        rbuf_recvend = (rbuf_recvend + 1) % MAX;  // update the other variables
        returnlen = (int)len;
        pthread_mutex_unlock(&recvbufferlock);
        return returnlen;  // returnlen is simply len
    }

    return -1;
}

int r_close(int udpsockfd) {
    while (1) {
        if (size_unackmsgtb == 0)
            break;  // till the time the unackmsgtable is not empty, wait
        else
            sleep(1);
    }

    free(recvbuffer);  // free the buffers
    free(unackmsgtb);  // no need to free individual entries in the unackmsgtb hash table
                       // because the size is already 0
    free(recvmsgidtb);
    pthread_mutex_destroy(&recvbufferlock);  // destroy the locks
    pthread_mutex_destroy(&unackmsgtblock);
    printf("Closing Socket\n");
    int n = close(udpsockfd);  // close the socket
    if (n < 0) return -1;
    return N_retransmissions;
}

int r_bind(int udpsockfd, const struct sockaddr* addr, socklen_t addrlen) {
    // bind the udp socket to the given address as usual
    if (bind(udpsockfd, addr, addrlen) < 0)
        return -1;  // failure
    return 0;  // success
}

int dropMessage(float p)  // Drop a msg
{
    float r = (float)rand() / (float)RAND_MAX;  // randomly choose a value between 0 and 1
    if (r < p) return 1;                        // drop if the value is less than p
    return 0;                                   // else don't drop
}

double getPerformance() {
    return (double) N_retransmissions / next_unused_id;
}
