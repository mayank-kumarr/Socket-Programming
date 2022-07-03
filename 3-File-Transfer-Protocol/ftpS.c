#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"
#define RESET "\033[0m"
#define PORT_X 50000
#define MAX_SIZE 500
#define BUFF_SIZE 100
#define COMM_SIZE 79
#define header_size 3

unsigned short PORT_Y;
char User[MAX_SIZE], Password[MAX_SIZE];
const char * IP_Y;
char * NULL1 = "\0";
char usernames[MAX_SIZE][MAX_SIZE];
char passwords[MAX_SIZE][MAX_SIZE];
int ids_len = 0, user_index = -1;

short int parse_first(char * cmd) {
    char * cname, * Argument;

    cname = strtok(cmd, " ");
    if (strcmp(cname, "user"))
        return (short int) 600;

    Argument = strtok(NULL, " ");
    if (!Argument || strtok(NULL, " "))
        return (short int) 501;

    if (!sscanf(Argument, "%s", User))
        return (short int) 501;
    
    for(int i=0; i<ids_len; i++)
    {
        if(!strcmp(usernames[i], User))
        {
            user_index = i;
            return (short int) 200;
        }
    }

    return (short int) 500;
}

short int parse_second(char * cmd) {
    char * cname, * Argument;

    cname = strtok(cmd, " ");
    if (strcmp(cname, "pass"))
        return (short int) 600;
    
    Argument = strtok(NULL, " ");
    if (!Argument || strtok(NULL, " "))
        return (short int) 501;
    
    if (!sscanf(Argument, "%s", Password))
        return (short int) 501;
    
    if(!strcmp(passwords[user_index], Password))
        return (short int) 200;
    else
        return (short int) 500;
}

int main() {
     
    char * IP_Y = "127.0.0.1";

    short int read_bytes1, read_bytes2, net_read_bytes, net_len, len;
    short int e, success;
    char rsp[10];
    int i, header, last, recv_bytes, total, stop, next, process, status, enable;
    int sockfd1, new_sockfd, sockfd2, File_Desc;
    int addr_size, cli_len, new_len;
    char net_len_bit[MAX_SIZE];
    char buffer[MAX_SIZE], buffer1[MAX_SIZE];
    char cmd[MAX_SIZE], FileName[MAX_SIZE];
    char Header[MAX_SIZE], Block[MAX_SIZE];
    char * cname, * PathName, * DestName, * dname, * Argument, * tmp;
    struct sockaddr_in server_addr, cli_addr, new_addr;

   FILE* ptr = fopen("user.txt", "r");
    if (NULL == ptr) {
        printf("File can't be opened.\n");
    }
    else
    {
        char line[500];
        ssize_t read;
        size_t len=0;
        ids_len = 0;
        while (fgets(line, sizeof(line), ptr) != NULL)
        {
            sscanf(line, "%s" "%s", usernames[ids_len], passwords[ids_len] );
            ids_len++;
        }
    }
    fclose(ptr);


    if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(RED "Control Socket Creation Failure " WHITE);
        exit(EXIT_FAILURE);
    }
    enable = 1;
    if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, & enable, sizeof(int)) < 0) {
        perror(RED "setsockopt(SO_REUSEADDR) Failed " WHITE);
    }

    memset( & server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP_Y);
    server_addr.sin_port = htons(PORT_X);
    addr_size = sizeof(server_addr);

    if (bind(sockfd1, (struct sockaddr * ) & server_addr, addr_size) < 0) {
        perror(RED "Unable to Bind Control Socket to Local Address " WHITE);
        close(sockfd1);
        exit(EXIT_FAILURE);
    }
    listen(sockfd1, 10);

    while (1) {
        printf(GREEN "\nServer Running.... \n\n" WHITE);
        
        cli_len = sizeof(cli_addr);
        if ((new_sockfd = accept(sockfd1, (struct sockaddr * ) & cli_addr, & cli_len)) < 0) {
            perror(RED "Server Control Unable to Accept Incoming Connection " WHITE);
            continue;
        }
        int cnt = 0;
        while (1)
        {
            if ((recv_bytes = recv(new_sockfd, buffer, MAX_SIZE, 0)) < 0) {
                perror(RED "Command Receive Error " WHITE);
                continue;
            }
            if (recv_bytes == 0) {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "SUCCESS - Closing Connection..");
                stop = 1;
                break;
            }
            sprintf(cmd, "%s", buffer);
            if(cnt==0) e = parse_first(buffer);
            else if(cnt==1) e = parse_second(buffer);

            sprintf(rsp, "%hd", e);
            send(new_sockfd, rsp, sizeof(rsp), 0);

            if (e == 200) {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, e, "Command Executed Successfully");
                stop = 0;
                cnt++;
            }
            if (e == 421) {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, e, "SUCCESS - Closing Connection..");
                stop = 1;
                break;
            }
            if (e == 500) {
                fprintf(stderr, RED "%hi : %s%s%s\n", 500, "Invalid Username or Password - ", WHITE, "Please enter both of them again.");
                cnt = 0;
                continue;
            }
            if (e == 501 || e == 502) {
                fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Command - ", WHITE, "Waiting for next command.");
                continue;
            }
            if (e == 550) {
                fprintf(stderr, RED "%hi : %s\n" WHITE, e, "FAILURE - Closing Connection.");
                stop = 1;
                break;
            }
            if (e == 600) {
                fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Sequence of Commands - ", WHITE, "First two commands should be `user` and `pass`.");
                continue;
            }
            if(cnt==2)
                break;
        }
        
        while (1)
        {
            if ((recv_bytes = recv(new_sockfd, buffer, MAX_SIZE, 0)) < 0) {
                perror(RED "Command Receive Error " WHITE);
                continue;
            }
            if (recv_bytes == 0) {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "SUCCESS - Closing Connection..");
                stop = 1;
                break;
            }
            sprintf(cmd, "%s", buffer);
            printf("Command received: %s\n", cmd);

            cname = strtok(buffer, " ");

            if (!strcmp(cname, "open") || !strcmp(cname, "user") || !strcmp(cname, "pass")) {
                e = 502;
                fprintf(stderr, RED "%hi : %s%s%s\n", 502, "Command Received Again - ", WHITE, "Waiting for next command.");
                sprintf(rsp, "%hd", e);
                send(new_sockfd, rsp, sizeof(rsp), 0);
                continue;
            }
            if (!strcmp(cname, "cd")) {
                dname = strtok(NULL, " ");
                if (!dname || strtok(NULL, " ")) {
                    e = 501;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Argument - ", WHITE, "Waiting for next command.");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                } else {
                    if (chdir(dname) < 0) {
                        perror(RED "500 : Could not change directory " WHITE);
                        e = 500;
                    } else {
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE, e, "Directory Change Successful.");
                        e = 200;
                    }
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                }
                continue;
            }
            if (!strcmp(cname, "get")) {
                success = 0;
                PathName = strtok(NULL, " ");
                DestName = strtok(NULL, " ");
                if (!PathName || !DestName || strtok(NULL, " ")) {
                    e = 501;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Argument - ", WHITE, "Waiting for next command.");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }

                if((strlen(PathName) >= 2 && PathName[0] == '.' && PathName[1] == '/') || (strlen(DestName) >= 2 && DestName[0] == '.' && DestName[1] == '/')) {
                    e = 501;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Path - ", WHITE, "file name cannot be relative to the current  directory  or  parent  directory.");
                    perror("");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }

                if ((File_Desc = open(PathName, O_RDONLY)) < 0) {
                    e = 500;
                    fprintf(stderr, RED "%hi : \'%s\' " WHITE, e, PathName);
                    perror("");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }

                e = 200;
                sprintf(rsp, "%hd", e);
                send(new_sockfd, rsp, sizeof(rsp), 0);

                bzero(buffer, MAX_SIZE);
                read_bytes1 = read(File_Desc, buffer, BUFF_SIZE);
                if (read_bytes1 < 0) {
                    perror(RED "Read Failure " WHITE);
                    close(File_Desc);
                    exit(EXIT_FAILURE);
                }

                if (read_bytes1 == 0) {
                    bzero(Block, MAX_SIZE);
                    sprintf(Block, "%c%c%c", 'L', 0, 0);
                    send(new_sockfd, Block, header_size + read_bytes1 + 1, 0);
                    close(File_Desc);
                    success = 1;
                }
                else {
                    while (1) {
                        bzero(Block, MAX_SIZE);
                        net_read_bytes = htons(read_bytes1);
                        Block[0] = 'M';
                        memcpy(Block + 1, & net_read_bytes, 2);
                        memcpy(Block + 3, buffer, read_bytes1);

                        bzero(buffer, MAX_SIZE);
                        if ((read_bytes2 = read(File_Desc, buffer, BUFF_SIZE)) < 0) {
                            perror(RED "Read Failure " WHITE);
                            close(File_Desc);
                            exit(EXIT_FAILURE);
                        }
                        if (read_bytes2 == 0) {
                            Block[0] = 'L';
                            send(new_sockfd, Block, header_size + read_bytes1 + 1, 0);
                            success = 1;
                            break;
                        }
                        send(new_sockfd, Block, header_size + read_bytes1, 0);
                        read_bytes1 = read_bytes2;
                    }
                }
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "File Transfer Successful");
                close(File_Desc);
                continue;
            }

            if (!strcmp(cname, "put")) {
                success = 0;
                PathName = strtok(NULL, " ");
                DestName = strtok(NULL, " ");
                if (!PathName || !DestName || strtok(NULL, " ")) {
                    e = 500;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 500, "Invalid Argument - ", WHITE, "Waiting for next command.");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }

                if((strlen(PathName) >= 2 && PathName[0] == '.' && PathName[1] == '/') || (strlen(DestName) >= 2 && DestName[0] == '.' && DestName[1] == '/')) {
                    e = 501;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Path - ", WHITE, "File name cannot be relative to the current  directory  or  parent  directory.");
                    perror("");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }

                if((strlen(PathName) >= 3 && PathName[0] == '.' && PathName[1] == '.' && PathName[2] == '/') || (strlen(DestName) >= 3 && DestName[0] == '.' && DestName[1] == '/' && DestName[2] == '/')) {
                    e = 501;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Path - ", WHITE, "File name cannot be relative to the current  directory  or  parent  directory.");
                    perror("");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }

                tmp = strtok(DestName, "/");
                sprintf(FileName, "%s", tmp);
                if (tmp = strtok(NULL, "/")) {
                    e = 500;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 500, "Invalid Argument, no Absolute Paths Allowed - ", WHITE, "Waiting for next command.");
                    sprintf(rsp, "%hd", e);
                    send(new_sockfd, rsp, sizeof(rsp), 0);
                    continue;
                }
                if ((File_Desc = creat(FileName, 0666)) < 0) {
                    perror(RED "Unable to Create Copy File " WHITE);
                    close(new_sockfd);
                    exit(EXIT_FAILURE);
                }

                e = 200;
                sprintf(rsp, "%hd", e);
                send(new_sockfd, rsp, sizeof(rsp), 0);

                total = 0;
                header = len = last = 0;
                while (1) {
                    bzero(buffer, MAX_SIZE);
                    recv_bytes = recv(new_sockfd, buffer, MAX_SIZE, 0);
                    if (recv_bytes < 0) {
                        perror(RED "get Failed ");
                        close(new_sockfd);
                        close(File_Desc);
                        exit(EXIT_FAILURE);
                    }
                    if (recv_bytes == 0) {
                        if (total == 0) {
                            close(new_sockfd);
                            exit(EXIT_FAILURE);
                        }
                        write(File_Desc, & NULL1, 1);
                        close(File_Desc);
                        success = 1;
                        break;
                    }
                    else {
                        total += recv_bytes;
                        for (i = 0; i < recv_bytes; i++) {
                            if (len == 0) {
                                if (last == 1 && header == 0) {
                                    break;
                                }
                                switch (header) {
                                    case 0:
                                        if (buffer[i] == 'L') last = 1;
                                        break;
                                    case 1:
                                        net_len_bit[0] = buffer[i];
                                        break;
                                    case 2:
                                        net_len_bit[1] = buffer[i];
                                        memcpy( & net_len, net_len_bit, 2);
                                        len = ntohs(net_len);
                                        break;
                                    default:
                                        break;
                                }
                                header = (header + 1) % 3;
                            } else {
                                write(File_Desc, buffer + i, 1);
                                len--;
                            }
                        }
                        if(last == 1)
                        {
                            success = 1;
                            break;
                        }
                    }
                }
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "File Transfer Successful");
                close(File_Desc);
                continue;
            }
            if (!strcmp(cname, "dir")) {
                success = 0;
                if (strtok(NULL, " ")) {
                    e = 500;
                    fprintf(stderr, RED "%hi : %s%s%s\n", 500, "Invalid Argument - ", WHITE, "Waiting for next command.");
                    send(new_sockfd, "", 1, 0);
                    continue;
                }
                DIR *d;
                struct dirent *dir;
                if (d = opendir(".")) {
                    while ((dir = readdir(d)) != NULL) {
                        sprintf(buffer1, "%s", dir->d_name);
                        send(new_sockfd, buffer1, sizeof(buffer1), 0);
                    }
                    closedir(d);
                }
                send(new_sockfd, "", 1, 0);
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "Contents Sent Successfully");
                continue;
            }
        }
    }
}