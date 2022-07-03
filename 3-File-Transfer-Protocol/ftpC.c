#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int validate(char * cname) {
    if(strcmp(cname, "open") && strcmp(cname, "user") && strcmp(cname, "pass") &&
       strcmp(cname, "cd") && strcmp(cname, "lcd") && strcmp(cname, "dir") && strcmp(cname, "get") &&
       strcmp(cname, "put") && strcmp(cname, "mget") && strcmp(cname, "mput") && strcmp(cname, "quit"))
       return 0;
    return 1;
}

void getfunc(char * DestName, int C1_sockfd, char * cmd)
{
    char * tmp;
    char * NULL1 = "\0";
    char FileName[MAX_SIZE], buffer[MAX_SIZE];
    char net_len_bit[MAX_SIZE];
    int i, File_Desc, header, last, recv_bytes;
    short int net_len, len, e;
    char rsp[10];
    tmp = strtok(DestName, "/");
    sprintf(FileName, "%s", tmp);
    while (tmp = strtok(NULL, "/")) {
        sprintf(FileName, "%s", tmp);
    }
    if ((File_Desc = creat(FileName, 0666)) < 0) {
        fprintf(stderr, RED "%s : " WHITE, FileName);
        perror("");
        exit(EXIT_FAILURE);
    }
    
    send(C1_sockfd, cmd, strlen(cmd) + 1, 0);

    recv(C1_sockfd, rsp, sizeof(rsp), 0);
    sscanf(rsp, "%hd", &e);
    if(e == 200)
        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "File will be transferred");
    else
        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "File could not be transferred");

    header = len = last = 0;
    while (1) {
        bzero(buffer, MAX_SIZE);
        recv_bytes = recv(C1_sockfd, buffer, MAX_SIZE, 0);
        if (recv_bytes < 0) {
            perror(RED "Get Failed " WHITE);
            close(File_Desc);
            exit(EXIT_FAILURE);
        }
        if (recv_bytes == 0) {
            write(File_Desc, & NULL1, 1);
            close(File_Desc);
            break;
        }

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
            }
            else {
                write(File_Desc, buffer + i, 1);
                len--;
            }
        }
        if(last == 1)
            break;
    }
    return;
}

void putfunc(char * PathName, int C1_sockfd, char * cmd)
{
    char FileName[MAX_SIZE], buffer[MAX_SIZE], Block[MAX_SIZE];
    short int read_bytes1, read_bytes2, net_read_bytes, e;
    char rsp[10];
    int File_Desc;
    sprintf(FileName, "%s", PathName);
    if ((File_Desc = open(FileName, O_RDONLY)) < 0) {
        fprintf(stderr, RED "\'%s\' : " WHITE, FileName);
        perror("");
        exit(EXIT_FAILURE);
    }
    
    send(C1_sockfd, cmd, strlen(cmd) + 1, 0);

    recv(C1_sockfd, rsp, sizeof(rsp), 0);
    sscanf(rsp, "%hd", &e);
    if(e == 200)
        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "File will be transferred");
    else
        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "File could not be transferred");
    
    bzero(buffer, MAX_SIZE);
    if ((read_bytes1 = read(File_Desc, buffer, BUFF_SIZE)) < 0) {
        perror(RED "Read Failure " WHITE);
        close(File_Desc);
        exit(EXIT_FAILURE);
    }
    if (read_bytes1 == 0) {
        bzero(Block, MAX_SIZE);
        sprintf(Block, "%c%c%c", 'L', 0, 0);
        send(C1_sockfd, Block, header_size + read_bytes1 + 1, 0);
        close(File_Desc);
        return;
    }
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
            return;
        }
        if (read_bytes2 == 0) {
            Block[0] = 'L';
            send(C1_sockfd, Block, header_size + read_bytes1 + 1, 0);
            break;
        }
        send(C1_sockfd, Block, header_size + read_bytes1, 0);
        read_bytes1 = read_bytes2;
    }
    return;
}

void dirfunc(int C1_sockfd, char * cmd)
{
    char buffer[MAX_SIZE];
    int recv_bytes;
    send(C1_sockfd, cmd, strlen(cmd) + 1, 0);
    while(1)
    {
        bzero(buffer, MAX_SIZE);
        recv_bytes = recv(C1_sockfd, buffer, MAX_SIZE, 0);
        if(!strcmp(buffer, ""))
            break;
        if (recv_bytes < 0) {
            perror(RED "Get Failed " WHITE);
            exit(EXIT_FAILURE);
        }
        if (recv_bytes == 0) {
            break;
        }
        printf("\t%s\n", buffer);
    }
}

void lcdfunc(char * dname)
{
    if (chdir(dname) < 0)
        perror(RED "501 : Could not change client directory " WHITE);
    else
        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "Client Directory Change Successful.");
    return;
}

int main() {
    short int read_bytes1, read_bytes2, net_read_bytes, net_len, len;
    short int e;
    int i, header, last;
    int recv_bytes, enable, status, process, FORK, PORT_Y;
    int CC_serv_len, SD_serv_len, CD_cli_len;
    int C1_sockfd, C2_sockfd, File_Desc;
    int chk = 0;
    struct sockaddr_in CC_serv_addr, CD_cli_addr, SD_serv_addr;
    char Server_IP[MAX_SIZE], net_len_bit[MAX_SIZE];
    char buffer[MAX_SIZE], temp[MAX_SIZE];
    char cmd[MAX_SIZE], FileName[MAX_SIZE], Block[MAX_SIZE];
    char * cname, * dname, * fname, * PathName, * DestName, * tmp, * IP_Y;
    char rsp[10];
    char * fnames[MAX_SIZE];
    int ab = 0;
    char * NULL1 = "\0";

    while(chk != 1)
    {
        printf(GREEN "myFTP>" WHITE);
        bzero(buffer, MAX_SIZE);
        fgets(buffer, MAX_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        bzero(cmd, MAX_SIZE);
        sprintf(cmd, "%s", buffer);
        cname = strtok(buffer, " ");
        if (!cname) continue;
        if (strcmp("open", cname) != 0 && chk == 0)
        {
            printf("Error. First command entered should be `open`.\n");
            continue;
        }
        if (strcmp("open", cname) == 0)
        {
            IP_Y = strtok(NULL, " ");
            tmp = strtok(NULL, " ");
            if(!IP_Y || !tmp)
            {
                printf("Error. Arguments expectd for IP Address and Port.\n");
                continue;
            }
            chk = 1;
            PORT_Y = atoi(tmp);
        }
    }

    if(!(PORT_Y>=20000 && PORT_Y<=65535))
    {
        printf(RED "Invalid Port Number.\n" WHITE);
        exit(EXIT_FAILURE);
    }

    if ((C1_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(RED "Control Socket Creation Failure " WHITE);
        exit(EXIT_FAILURE);
    }

    memset( & CC_serv_addr, '\0', sizeof(CC_serv_addr));
    CC_serv_addr.sin_family = AF_INET;
    inet_aton(IP_Y, & CC_serv_addr.sin_addr);
    CC_serv_addr.sin_port = htons(PORT_Y);
    CC_serv_len = sizeof(CC_serv_addr);

    if (connect(C1_sockfd, (struct sockaddr * ) & CC_serv_addr, CC_serv_len) < 0) {
        perror(RED "Client unable to connect to Server " WHITE);
        close(C1_sockfd);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "Server Connected Successfully");

    while (1) {
        printf(GREEN "myFTP>" WHITE);
        bzero(buffer, MAX_SIZE);
        fgets(buffer, MAX_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        bzero(cmd, MAX_SIZE);
        sprintf(cmd, "%s", buffer);

        cname = strtok(buffer, " ");
        if (!cname) continue;
        if(!validate(cname))
        {
            fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "Unrecognisable command not implemented");
            continue;
        }
        FORK = 0;
        if (!strcmp(cname, "get")) {
            FORK = 1;
            PathName = strtok(NULL, " ");
            DestName = strtok(NULL, " ");
        }
        if (!strcmp(cname, "put")) {
            FORK = 2;
            PathName = strtok(NULL, " ");
            DestName = strtok(NULL, " ");
        }
        if (!strcmp(cname, "dir")) {
            FORK = 3;
        }
        if (!strcmp(cname, "mget")) {
            FORK = 4;
        }
        if (!strcmp(cname, "mput")) {
            FORK = 5;
        }
        if (!FORK) // cd, lcd, user, pass, quit
        {
            if(!strcmp(cname, "lcd"))
            {
                dname = strtok(NULL, " ");
                if (!dname || strtok(NULL, " "))
                    fprintf(stderr, RED "%hi : %s%s%s\n", 501, "Invalid Argument - ", WHITE, "Waiting for next command.");
                lcdfunc(dname);
                continue;
            }
            if(!strcmp(cname, "quit"))
            {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 421, "Closing Connections and Quitting....");
                close(C1_sockfd);
                exit(EXIT_SUCCESS);
            }
            if (send(C1_sockfd, cmd, strlen(cmd) + 1, 0) < 0) {
                fprintf(stderr, RED "Server Closed Connection - " WHITE "Closing Connections and Quitting....");
                close(C1_sockfd);
                exit(EXIT_FAILURE);
            }
            if (recv(C1_sockfd, rsp, sizeof(rsp), 0) < 0) {
                fprintf(stderr, RED "Server Closed Connection - " WHITE "Closing Connections and Quitting....");
                kill(process, SIGKILL);
                close(C1_sockfd);
                exit(EXIT_FAILURE);
            }

            sscanf(rsp, "%hd", &e);
            switch (e) {
                case 200:
                    if (!strcmp(cname, "user")) {
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "User Received Successfully");
                        break;
                    }
                    if (!strcmp(cname, "pass")) {
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "Password Received Successfully");
                        break;
                    }
                    if (!strcmp(cname, "cd")) {
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "Directory Changed Successfully");
                        break;
                    }

                case 500:
                    if (!strcmp(cname, "user")){
                        fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "User not found");
                        break;
                    }
                    if (!strcmp(cname, "pass")){
                        fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "Incorrect Username or Password");
                        break;
                    }
                    if (!strcmp(cname, "cd")){
                        fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "Directory Change Unsuccessful");
                        break;
                    }
                    if (!strcmp(cname, "get")){
                        fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "Could not fetch file(s)");
                        break;
                    }
                    if (!strcmp(cname, "put")){
                        fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "Could not fetch file(s)");
                        break;
                    }
                    if (!strcmp(cname, "dir")){
                        fprintf(stderr, RED "%hi : %s\n" WHITE, 500, "Could not fetch contents of current directory");
                        break;
                    }

                case 421:
                    fprintf(stderr, CYAN "%hi : %s\n" WHITE, 421, "Closing Connections and Quitting....");
                    close(C1_sockfd);
                    exit(EXIT_SUCCESS);

                case 501:
                    fprintf(stderr, RED "%hi : %s\n" WHITE, 501, "Invalid Arguments, Please Try Again");
                    break;

                case 502:
                    fprintf(stderr, RED "%hi : %s\n" WHITE, 502, "Unrecognisable command not implemented");
                    break;

                case 550:
                    fprintf(stderr, RED "%hi : %s\n" WHITE, 550, "FATAL ERROR - Incorrect Port Number!");
                    close(C1_sockfd);
                    exit(EXIT_FAILURE);

                case 600:
                    fprintf(stderr, RED "%hi : %s\n" WHITE, 600, "Bad sequence of commands ");
                    fprintf(stderr,  "\nFirst command should be \'open <IP address> <PORT>\'");
                    fprintf(stderr,  "\nSecond command should be \'user <username>\'" WHITE);
                    fprintf(stderr,  "\nThird command should be \'pass <password>\'\n" WHITE);
                    break;

                default:
                    fprintf(stderr, RED "%s" WHITE "%s\n", "Some Error Occured on Server Side - ", "Closing Connections and Quitting....");
                    close(C1_sockfd);
                    exit(EXIT_FAILURE);
            }
            continue;
        }
        bzero(FileName, MAX_SIZE);

        switch (FORK) {
            case 1:
                getfunc(DestName, C1_sockfd, cmd);
                break;

            case 2:
                putfunc(PathName, C1_sockfd, cmd);
                break;

            case 3:
                dirfunc(C1_sockfd, cmd);
                break;
            
            case 4:
                ab = 0;
                while(1)
                {
                    char * fname2 = strtok(NULL, ", ");
                    if(!fname2)
                        break;
                    fnames[ab] = fname2;
                    ab++;
                }
                for(i=0; i<ab; i++)
                {
                    char cmd2[MAX_SIZE];
                    strcpy(cmd2, "get ");
                    strcat(cmd2, fnames[i]);
                    strcat(cmd2, " ");
                    strcat(cmd2, fnames[i]);
                    getfunc(fnames[i], C1_sockfd, cmd2);
                    sleep(1);
                }
                break;
            case 5:
                ab = 0;
                while(1)
                {
                    char * fname2 = strtok(NULL, ", ");
                    if(!fname2)
                        break;
                    fnames[ab] = fname2;
                    ab++;
                }
                for(i=0; i<ab; i++)
                {
                    char cmd2[MAX_SIZE];
                    strcpy(cmd2, "put ");
                    strcat(cmd2, fnames[i]);
                    strcat(cmd2, " ");
                    strcat(cmd2, fnames[i]);
                    putfunc(fnames[i], C1_sockfd, cmd2);
                    sleep(1);
                }
                break;
            default:
                break;
        }
    }
    return 0;
}